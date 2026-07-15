/*
 * Allwinner Sun60iw2 do poweroff in uboot with arisc
 *
 * (C) Copyright 2021  <xinouyang@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <sunxi_board.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/efuse.h>
#include <private_uboot.h>
#include <fdt_support.h>
#include <sunxi_power/sunxi_power.h>
#include <smc.h>
#include <malloc.h>
#include <string.h>

#ifdef CONFIG_SUNXI_POWER
#include <sunxi_power/sunxi_power.h>
#include <fdt_support.h>
#endif
#include <sunxi_gpioled.h>

#define UARTLCR	0xC
#define UARTHALT 0xA4
#define UARTLCR_DLAB	0x80 /* bit7 */
#define UARTDLL 0x0
#define UARTDLLM 0x4
#define UART_MEM_SIZE	0x1000
#define UARTHALT_AT_BUSY 0x2

int sunxi_platform_power_off(int status)
{
	int work_mode = get_boot_work_mode();

	if (work_mode != WORK_MODE_BOOT)
		return 0;

	/* flush cache to dram */
	flush_dcache_range(
		(ulong)ALIGN_DOWN(((ulong)working_fdt), CONFIG_SYS_CACHELINE_SIZE),
		(ulong)(ALIGN(((ulong)working_fdt + working_fdt->totalsize),
			      CONFIG_SYS_CACHELINE_SIZE)));

	/* startup cpus before shutdown */
	arm_svc_arisc_startup((ulong)working_fdt);

	if (status)
		arm_svc_poweroff_charge();
	else
		arm_svc_poweroff();

	while (1) {
		asm volatile ("wfi");
	}
	return 0;
}

#ifdef CONFIG_SUNXI_POWER
int update_bmu_info_to_kernel(void)
{
	sunxi_bmu_info_update();

	return 0;
}
#endif

int sunxi_baud_rate_get(void)
{
	uint32_t val;
	static int32_t sclk = -1;
	static int32_t baudrate = -1;
	uint32_t  uart_base = 0;
	uint32_t clk_src = 0, factor_m = 0, divisor = 0;

	uart_base = (SUNXI_UART0_BASE + uboot_spare_head.boot_data.uart_port * UART_MEM_SIZE);

	val = readl(uart_base + UARTHALT);
	val |= UARTHALT_AT_BUSY;
	writel(val, uart_base + UARTHALT);

	/*adaptive select sclk and baudrate*/
	val = readl(SUNXI_UART_CLK_REG);
	clk_src = (val >> 24) & 0x7;
	factor_m = (val & 0x1f) + 1;

	val = readl(uart_base + UARTLCR);
	val |= UARTLCR_DLAB;
	writel(val, uart_base + UARTLCR);

	divisor = (readl(uart_base + UARTDLL) & 0xFF) | (readl(uart_base + UARTDLLM) & 0xFF) << 8;
	val &= ~UARTLCR_DLAB;
	writel(val, uart_base + UARTLCR);

	val = readl(uart_base + UARTHALT);
	val &= ~UARTHALT_AT_BUSY;
	writel(val, uart_base + UARTHALT);

	if (clk_src == SUNXI_UART_CLK_SRC_SEL_HOSC) {
		sclk = 24000000 / factor_m;
		baudrate = sclk / (16 * divisor);
		if (baudrate == 115384)
			baudrate = 115200;
	} else if (clk_src == SUNXI_UART_CLK_SRC_PERI0_600M) {
		sclk = 600000000 / factor_m;
		baudrate = sclk / (16 * divisor);
	} else if (clk_src == SUNXI_UART_CLK_SRC_PERI0_480M) {
		sclk = 480000000 / factor_m;
		baudrate = sclk / (16 * divisor);
	}

	return baudrate;
}

#ifdef CONFIG_SUNXI_SET_EFUSE_POWER
int set_efuse_voltage(int status)
{
	int nodeoffset, len;
	int vol;
	const char *power_supply;
	const char *vol_value = "voltage";
	const char *power_name = "power_supply";

	nodeoffset = fdt_path_offset(working_fdt, "/soc/sid");
	if (nodeoffset < 0) {
		printf("libfdt fdt_path_offset() returned %s\n",
				fdt_strerror(nodeoffset));
		return -1;
	}
	vol = fdt_getprop_u32_default_node(working_fdt, nodeoffset, 0, vol_value, -1);
	power_supply = fdt_getprop(working_fdt, nodeoffset, power_name, &len);

	if (status) {
		pmu_set_voltage((char *)power_supply, vol, 1);
		mdelay(20);
	} else {
		mdelay(20);
		pmu_set_voltage((char *)power_supply, 0, 0);
	}

	return 0;
}
#endif

int update_board_dt_config_to_kernel(void)
{
	const void *fdt;
	int offset, ret;
	u32 cpu_val = 0;

	/* Read cpu_status from efuse using arm_svc_efuse_read */
	arm_svc_efuse_read("cpu_status", &cpu_val);

	/* Use gd->new_ext_fdt as the device tree pointer with enhanced error checking */
	fdt = gd->new_ext_fdt;
	if (!fdt) {
		pr_err("%s:%d: gd->new_ext_fdt is NULL\n", __func__, __LINE__);
		return -1;
	}

	/* Check if the device tree has valid magic number */
	uint32_t magic = fdt_magic(fdt);
	if (magic != FDT_MAGIC) {
		pr_err("%s:%d: Invalid device tree magic: 0x%08x (expected: 0x%08x)\n",
		       __func__, __LINE__, magic, FDT_MAGIC);
		pr_err("Device tree address: 0x%p\n", fdt);
		pr_err("Device tree size: %d bytes\n", fdt_totalsize(fdt));
		return -1;
	}

	/* Check if cpu_val & 0x10140 == 0x10140 (A333), otherwise A537 */
	if ((cpu_val & 0x10140) == 0x10140) {
		pr_debug("%s:%d: CPU_STATUS indicates A333 (0x%08x), performing device tree modifications\n",
			__func__, __LINE__, cpu_val);

		/* Find /cpus node using the same method as fdt list command */
	offset = fdt_path_offset(fdt, "/cpus");
	if (offset < 0) {
		pr_err("%s:%d: Could not find /cpus node in device tree: %s\n",
			__func__, __LINE__, fdt_strerror(offset));
		return offset;
	}

		/* Delete cpu nodes under cpus - using fdt rm command style error handling */
		const char *cpu_nodes[] = {"cpu@2", "cpu@3", "cpu@201"};
		int i;

		for (i = 0; i < sizeof(cpu_nodes) / sizeof(cpu_nodes[0]); i++) {
			int cpu_offset = fdt_subnode_offset(fdt, offset, cpu_nodes[i]);
			if (cpu_offset >= 0) {
				ret = fdt_del_node((void *)fdt, cpu_offset);
				if (ret < 0) {
					pr_err("%s:%d: Failed to delete %s node: %s\n",
					       __func__, __LINE__, cpu_nodes[i], fdt_strerror(ret));
				} else {
					pr_info("%s:%d: Successfully deleted %s node\n",
						__func__, __LINE__, cpu_nodes[i]);
				}
			} else if (cpu_offset != -FDT_ERR_NOTFOUND) {
				pr_err("%s:%d: Could not find %s node: %s\n",
					__func__, __LINE__, cpu_nodes[i], fdt_strerror(cpu_offset));
			}
		}

		/* Find cpu-map node under cpus */
		offset = fdt_subnode_offset(fdt, offset, "cpu-map");
		if (offset < 0) {
			pr_err("%s:%d: Could not find /cpus/cpu-map node in device tree: %s\n",
			       __func__, __LINE__, fdt_strerror(offset));
			return 0;
		}

		/* Process cluster0 nodes */
		int cluster0_offset = fdt_subnode_offset(fdt, offset, "cluster0");
		if (cluster0_offset >= 0) {
			const char *core_nodes[] = {"core2", "core3"};

			for (i = 0; i < sizeof(core_nodes) / sizeof(core_nodes[0]); i++) {
				int core_offset = fdt_subnode_offset(fdt, cluster0_offset, core_nodes[i]);
				if (core_offset >= 0) {
					ret = fdt_del_node((void *)fdt, core_offset);
					if (ret < 0) {
						pr_err("%s:%d: Failed to delete %s node: %s\n",
						       __func__, __LINE__, core_nodes[i], fdt_strerror(ret));
					} else {
						pr_info("%s:%d: Successfully deleted %s node\n",
							__func__, __LINE__, core_nodes[i]);
					}
				} else if (core_offset != -FDT_ERR_NOTFOUND) {
					pr_err("%s:%d: Could not find %s node: %s\n",
						__func__, __LINE__, core_nodes[i], fdt_strerror(core_offset));
				}
			}
		}

		/* Process cluster2 nodes */
		int cluster2_offset = fdt_subnode_offset(fdt, offset, "cluster2");
		if (cluster2_offset >= 0) {
			int core_offset = fdt_subnode_offset(fdt, cluster2_offset, "core1");
			if (core_offset >= 0) {
				ret = fdt_del_node((void *)fdt, core_offset);
				if (ret < 0) {
					pr_err("%s:%d: Failed to delete core1 node: %s\n",
					       __func__, __LINE__, fdt_strerror(ret));
				} else {
					pr_info("%s:%d: Successfully deleted core1 node\n",
						__func__, __LINE__);
				}
			} else if (core_offset != -FDT_ERR_NOTFOUND) {
				pr_err("%s:%d: Could not find core1 node: %s\n",
					__func__, __LINE__, fdt_strerror(core_offset));
			}
		}

		/* thermal-zones node */
		offset = fdt_path_offset(fdt, "/thermal-zones");
		if (offset >= 0) {
			int zone_offset = fdt_subnode_offset(fdt, offset, "cpul_idle_zone");
			if (zone_offset >= 0) {
				ret = fdt_del_node((void *)fdt, zone_offset);
				if (ret < 0) {
					pr_err("%s:%d: Failed to delete cpul_idle_zone node: %s\n",
					       __func__, __LINE__, fdt_strerror(ret));
				} else {
					pr_info("%s:%d: Successfully deleted cpul_idle_zone node\n",
						__func__, __LINE__);
				}
			} else if (zone_offset != -FDT_ERR_NOTFOUND) {
				pr_err("%s:%d: Could not find cpul_idle_zone node: %s\n",
					__func__, __LINE__, fdt_strerror(zone_offset));
			}

			/* cpul_thermal_zone node */
			int cpul_thermal_offset = fdt_subnode_offset(fdt, offset, "cpul_thermal_zone");
			if (cpul_thermal_offset >= 0) {
				int len;
				const void *sustainable_power_a333_data = fdt_getprop((void *)fdt, cpul_thermal_offset, "sustainable-power-a333", &len);
				if (sustainable_power_a333_data) {
					uint32_t *sustainable_power_copy = malloc(len);
					if (sustainable_power_copy) {
						memcpy(sustainable_power_copy, sustainable_power_a333_data, len);

						ret = fdt_setprop((void *)fdt, cpul_thermal_offset, "sustainable-power", sustainable_power_copy, len);
						free(sustainable_power_copy);

						if (ret < 0) {
							pr_err("%s:%d: Failed to set sustainable-power from sustainable-power-a333: %s\n",
								__func__, __LINE__, fdt_strerror(ret));
						} else {
							pr_info("%s:%d: Successfully set sustainable-power from sustainable-power-a333\n",
								__func__, __LINE__);
						}
					}
				}
			} else {
				pr_err("%s:%d: Could not find cpul_thermal_zone node: %s\n",
				       __func__, __LINE__, fdt_strerror(cpul_thermal_offset));
			}

			/* cpub_thermal_zone node */
			int cpub_thermal_offset = fdt_subnode_offset(fdt, offset, "cpub_thermal_zone");
			if (cpub_thermal_offset >= 0) {
				int len;
				const void *sustainable_power_a333_data = fdt_getprop((void *)fdt, cpub_thermal_offset, "sustainable-power-a333", &len);
				if (sustainable_power_a333_data) {
					uint32_t *sustainable_power_copy = malloc(len);
					if (sustainable_power_copy) {
						memcpy(sustainable_power_copy, sustainable_power_a333_data, len);

						ret = fdt_setprop((void *)fdt, cpub_thermal_offset, "sustainable-power", sustainable_power_copy, len);
						free(sustainable_power_copy);

						if (ret < 0) {
							pr_err("%s:%d: Failed to set sustainable-power from sustainable-power-a333: %s\n",
								__func__, __LINE__, fdt_strerror(ret));
						} else {
							pr_info("%s:%d: Successfully set sustainable-power from sustainable-power-a333\n",
								__func__, __LINE__);
						}
					}
				}
			} else {
				pr_err("%s:%d: Could not find cpub_thermal_zone node: %s\n",
				       __func__, __LINE__, fdt_strerror(cpub_thermal_offset));
			}

			/* cpub_idle_zone node with cooling-maps */
			int cpub_idle_offset = fdt_subnode_offset(fdt, offset, "cpub_idle_zone");
			if (cpub_idle_offset >= 0) {
				/* cooling-maps subnode */
				int cooling_maps_offset = fdt_subnode_offset(fdt, cpub_idle_offset, "cooling-maps");
				if (cooling_maps_offset >= 0) {
					/* map0 subnode */
					int map0_offset = fdt_subnode_offset(fdt, cooling_maps_offset, "map0");
					if (map0_offset >= 0) {
						int len;
						const void *cooling_device_a333_data = fdt_getprop((void *)fdt, map0_offset, "cooling-device-a333", &len);
						if (cooling_device_a333_data) {
							uint32_t *cooling_device_copy = malloc(len);
							if (cooling_device_copy) {
								memcpy(cooling_device_copy, cooling_device_a333_data, len);

								ret = fdt_setprop((void *)fdt, map0_offset, "cooling-device", cooling_device_copy, len);
								free(cooling_device_copy);

								if (ret < 0) {
									pr_err("%s:%d: Failed to set cooling-device from cooling-device-a333: %s\n",
									       __func__, __LINE__, fdt_strerror(ret));
								} else {
									pr_info("%s:%d: Successfully set cooling-device from cooling-device-a333\n",
										__func__, __LINE__);
								}
							}
						}
					} else {
						pr_err("%s:%d: Could not find map0 node: %s\n",
						       __func__, __LINE__, fdt_strerror(map0_offset));
					}
				} else {
					pr_err("%s:%d: Could not find cooling-maps node: %s\n",
					       __func__, __LINE__, fdt_strerror(cooling_maps_offset));
				}
			} else {
				pr_err("%s:%d: Could not find cpub_idle_zone node: %s\n",
				       __func__, __LINE__, fdt_strerror(cpub_idle_offset));
			}
		}

		/* Add arm_pmu node under soc */
		offset = fdt_path_offset(fdt, "/soc");
		if (offset >= 0) {
			int arm_pmu_offset = fdt_subnode_offset(fdt, offset, "arm_pmu");
			if (arm_pmu_offset >= 0) {
				/* Set interrupts property */
				uint32_t interrupts[] = {
					cpu_to_fdt32(0x13c), /* GIC_SPI 316 */
					cpu_to_fdt32(0x4),   /* IRQ_TYPE_LEVEL_HIGH */
					cpu_to_fdt32(0x13d), /* GIC_SPI 317 */
					cpu_to_fdt32(0x4),   /* IRQ_TYPE_LEVEL_HIGH */
					cpu_to_fdt32(0x15c), /* GIC_SPI 348 */
					cpu_to_fdt32(0x4),   /* IRQ_TYPE_LEVEL_HIGH */
					cpu_to_fdt32(0x15d), /* GIC_SPI 349 */
					cpu_to_fdt32(0x4),   /* IRQ_TYPE_LEVEL_HIGH */
					cpu_to_fdt32(0x176), /* GIC_SPI 374 */
					cpu_to_fdt32(0x4)    /* IRQ_TYPE_LEVEL_HIGH */
				};
				ret = fdt_setprop((void *)fdt, arm_pmu_offset, "interrupts", interrupts, sizeof(interrupts));
				if (ret < 0) {
					pr_err("%s:%d: Failed to set interrupts for arm_pmu: %s\n",
					       __func__, __LINE__, fdt_strerror(ret));
				}

				/* Set interrupt-affinity property */
				uint32_t interrupt_affinity[] = {
					fdt_get_phandle(fdt, fdt_path_offset(fdt, "/cpus/cpu@0")),
					fdt_get_phandle(fdt, fdt_path_offset(fdt, "/cpus/cpu@1")),
					fdt_get_phandle(fdt, fdt_path_offset(fdt, "/cpus/cpu@100")),
					fdt_get_phandle(fdt, fdt_path_offset(fdt, "/cpus/cpu@101")),
					fdt_get_phandle(fdt, fdt_path_offset(fdt, "/cpus/cpu@200"))
				};
				ret = fdt_setprop((void *)fdt, arm_pmu_offset, "interrupt-affinity", interrupt_affinity, sizeof(interrupt_affinity));
				if (ret < 0) {
					pr_err("%s:%d: Failed to set interrupt-affinity for arm_pmu: %s\n",
					       __func__, __LINE__, fdt_strerror(ret));
				} else {
					pr_info("%s:%d: Successfully modified arm_pmu node under soc\n",
						__func__, __LINE__);
				}
			} else {
				pr_err("%s:%d: Could not find arm_pmu node: %s\n",
				       __func__, __LINE__, fdt_strerror(arm_pmu_offset));
			}
		}

		/* Modify a55_rproc node core-id property */
		offset = fdt_path_offset(fdt, "/soc");
		if (offset >= 0) {
			int a55_rproc_offset = fdt_subnode_offset(fdt, offset, "a55_rproc@0");
			if (a55_rproc_offset >= 0) {
				/* Change core-id 1 */
				uint32_t new_core_id = cpu_to_fdt32(1);
				ret = fdt_setprop((void *)fdt, a55_rproc_offset, "core-id", &new_core_id, sizeof(new_core_id));
				if (ret < 0) {
					pr_err("%s:%d: Failed to modify a55_rproc core-id property: %s\n",
				       __func__, __LINE__, fdt_strerror(ret));
				} else {
					pr_info("%s:%d: Successfully modified a55_rproc core-id from 7 to 1\n",
						__func__, __LINE__);
				}
			} else {
				pr_err("%s:%d: Could not find a55_rproc@0 node under /soc: %s\n",
				       __func__, __LINE__, fdt_strerror(a55_rproc_offset));
			}
		}
	} else {
		pr_debug("%s:%d: CPU_STATUS indicates 0x%08x , no device tree modifications needed\n",
			__func__, __LINE__, cpu_val);
	}

	return 0;
}

void sunxi_boot_init_gpioled(void)
{
	sunxi_gpioled_ctrl(GPIOLED_ON, FDT_PATH_CARD_BOOT);
}
