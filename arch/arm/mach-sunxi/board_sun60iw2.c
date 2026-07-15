/*
 * Allwinner Sun60iw2 do poweroff in uboot with arisc
 *
 * (C) Copyright 2021  <xinouyang@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <sunxi_board.h>
#include <smc.h>
#include <asm/arch/usb.h>
#include <asm/arch/watchdog.h>
#include <sprite.h>
#include <asm/arch/clock.h>
#include <private_uboot.h>

#ifdef CONFIG_SUNXI_POWER
#include <sunxi_power/sunxi_power.h>
#include <fdt_support.h>
#include <linux/bitops.h>
#endif

#include <asm/arch-sunxi/efuse.h>
#include <asm/arch/gpio.h>

#define SUNXI_SOC_VER_B		(0x1)
#define SUNXI_GPIO_VCCIO	(12)

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
	/* imporve later */
	ulong cfg_base = 0;

	if (work_mode != WORK_MODE_BOOT)
		return 0;

	/* startup cpus before shutdown */
	arm_svc_arisc_startup(cfg_base);

	if (status)
		arm_svc_poweroff_charge();
	else
		arm_svc_poweroff();

	while (1) {
		asm volatile ("wfi");
	}
	return 0;
}

#ifdef CONFIG_SUNXI_PMU_EXT
int update_pmu_ext_info_to_kernel(void)
{
	int nodeoffset, err;
	uint32_t phandle = 0;

	phandle = sunxi_pmu_ext_info_update();

	/* get cpu@4 node */
	nodeoffset = fdt_path_offset(working_fdt, "cpu-ext");
	if (nodeoffset < 0) {
		pr_err("## error: %s: L%d\n", __func__, __LINE__);
		return -1;
	}

	/* Change cpu-supply to ext dcdc*/
	err = fdt_setprop_u32(working_fdt, nodeoffset,
				"cpu-supply", phandle);
	if (err < 0) {
		pr_warn("WARNING: fdt_setprop can't set %s from node %s: %s\n",
			"compatible", "status", fdt_strerror(err));
		return -1;
	}

	return 0;
}
#endif

#define SRAM_CTRL_REG2 (SUNXI_SYSCTRL_BASE + 0x8)
int sunxi_set_sramc_mode(void)
{
	u32 reg_val;

	/* SRAM:set sram to npu, default boot mode */
	reg_val = readl(SRAM_CTRL_REG2);
	reg_val &= ~(0x1 << 1);
	writel(reg_val, SRAM_CTRL_REG2);
	debug("set sram to npu\n");

	reg_val = readl(SRAM_CTRL_REG2);
	if (reg_val & (0x1 << 1))
		pr_err("set sram to npu fail!\n");
	return 0;
}

int sunxi_get_active_boot0_id(void)
{
	uint32_t val = *(uint32_t *)(SUNXI_RTC_BASE + 0x318);
	if (val & (1 << 15)) {
		return (val >> 12) & 0x7;
	} else {
		return (val >> 28) & 0x7;
	}
}

void otg_phy_config(void)
{
	u32 reg_val;
	reg_val = readl((const volatile void __iomem *)(SUNXI_USBOTG_BASE +
							USBC_REG_o_PHYCTL));
	reg_val &= ~(0x01 << USBC_PHY_CTL_SIDDQ);
	reg_val |= 0x01 << USBC_PHY_CTL_VBUSVLDEXT;
	writel(reg_val, (volatile void __iomem *)(SUNXI_USBOTG_BASE +
						  USBC_REG_o_PHYCTL));
}

void sunxi_board_reset_cpu(ulong addr)
{
	static const struct sunxi_wdog *wdog =
		(struct sunxi_wdog *)SUNXI_WDT_BASE;

	writel(((WDT_CFG_KEY << 16) | 0x02), &wdog->ocfg);
	/* Set the watchdog for its shortest interval (.5s) and wait */
	writel(((WDT_CFG_KEY << 16) | WDT_MODE_EN), &wdog->srst);
	while (1) { }

}

#ifdef CONFIG_SUNXI_POWER
int update_bmu_info_to_kernel(void)
{
	sunxi_bmu_info_update();

	return 0;
}
#endif

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

int get_group_bit_offset(enum pin_e port_group)
{
	switch (port_group) {
	case GPIO_GROUP_B:
		return SUNXI_GPIO_VCCIO;
		break;
	case GPIO_GROUP_C:
	case GPIO_GROUP_D:
	case GPIO_GROUP_E:
	case GPIO_GROUP_F:
	case GPIO_GROUP_G:
	case GPIO_GROUP_H:
	case GPIO_GROUP_I:
	case GPIO_GROUP_J:
	case GPIO_GROUP_K:
		return port_group;
		break;
	default:
		return -1;
	}
	return -1;
}

enum io_pow_mode_e io_get_volt_val(enum pin_e port_group)
{
	uint32_t reg;
	uint8_t group_bit_offset = get_group_bit_offset(port_group);
	int ver;

	if ((group_bit_offset < 0) || ((group_bit_offset * 2) >= 32)) {
		pr_err("get port %d volt error:port error\n", port_group);
		return IO_MODE_DEFAULT;
	}

	reg = readl(PIOC_REG_POW_VAL);
	ver = sunxi_efuse_get_soc_ver();
	if ((ver == SUNXI_SOC_VER_B)
			&& (port_group != GPIO_GROUP_C)
			&& (port_group != GPIO_GROUP_E)
			&& (port_group != GPIO_GROUP_K))
		/* (except bank C/E/K): 0 -- 3.3v  1 -- 1.8v */
		return (!(reg & (1 << (group_bit_offset * 2))) != 0) ?
			IO_MODE_3_3_V :
			IO_MODE_1_8_V;
	else
		/* (only bank C/E/K): 0 -- 1.8v  1 -- 3.3v */
		return ((reg & (1 << (group_bit_offset * 2))) != 0) ?
			IO_MODE_3_3_V :
			IO_MODE_1_8_V;
}

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
