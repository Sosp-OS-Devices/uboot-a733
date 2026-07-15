/*
 * disp2/disp/disp_sys_intf.c
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/libfdt.h>
#include <drm/drm_print.h>
#include <drm/drm_logo.h>
#include <malloc.h>
#include "sunxi_drm_updata_param.h"
#include "sunxi_drm_parameter.h"
#include "sunxi_disp_param_from_ini.h"
#include "sunxi_drm_time_trace.h"

#define INI_FILE_NAME		"disp_config.ini"

#define MAX_INI_KEY_COUNT	64

#define MAX_CRTC_COUNT		2
#define MAX_CONNECTOR_COUNT	8
#define MAX_VO_COUNT		2
#define MAX_TCON_COUNT		5
#define MAX_TCON_LCD_COUNT	3	/* FIXME */

/* FIXME: uboot-2018 does not support update fdt! (Except u32) */
#define UBOOT_UPDATE_SUPPORT	0

struct ini_private {
	char file_name[128];
	char part_name[128];
	const char *data;
	bool dirty;
	struct key_info *info;
	bool uboot;
};

struct type_name {
	uint32_t type;
	char name[64];
};

struct type_name type_names[] = {
	/* { DRM_MODE_CONNECTOR_Unknown,     "unknow"    }, */
	{ DRM_MODE_CONNECTOR_VGA,         "vga"       },
	/* { DRM_MODE_CONNECTOR_DVII,        "dvii"      }, */
	/* { DRM_MODE_CONNECTOR_DVID,        "dvid"      }, */
	/* { DRM_MODE_CONNECTOR_DVIA,        "dvia"      }, */
	/* { DRM_MODE_CONNECTOR_Composite,   "composite" }, */
	/* { DRM_MODE_CONNECTOR_SVIDEO,      "svideo"    }, */
	{ DRM_MODE_CONNECTOR_LVDS,        "lvds"      },
	/* { DRM_MODE_CONNECTOR_Component,   "component" }, */
	/* { DRM_MODE_CONNECTOR_9PinDIN,     "9pindin"   }, */
	{ DRM_MODE_CONNECTOR_DisplayPort, "dp"        },
	{ DRM_MODE_CONNECTOR_HDMIA,       "hdmi"      },
	/* { DRM_MODE_CONNECTOR_HDMIB,       "hdmi"      }, */
	{ DRM_MODE_CONNECTOR_TV,          "tv"        },
	{ DRM_MODE_CONNECTOR_eDP,         "edp"       },
	/* { DRM_MODE_CONNECTOR_VIRTUAL,     "virtual"   }, */
	{ DRM_MODE_CONNECTOR_DSI,         "dsi"       },
	{ DRM_MODE_CONNECTOR_DPI,         "rgb"       },
	/* { DRM_MODE_CONNECTOR_WRITEBACK,   "writeback" }, */
	/* { DRM_MODE_CONNECTOR_SPI,         "spi"       }, */
	/* { DRM_MODE_CONNECTOR_USB,         "usb"       }, */
};

enum key_type {
	KEY_TYPE_STRING,
	KEY_TYPE_U32,
	KEY_TYPE_ARRAY,
};

struct ini_update_key {
	const char *name;
	enum key_type type;
	bool need_dirty;	/* only setup this key when dirty=1 */
	int cell_size;		/* optional: only KEY_TYPE_ARRAY use */
	bool skip_uboot;	/* optional: only update kernel fdt */
};

static int ini_update_param(struct ini_private ini, int node, const char *primary_key,
			    const char *group_name, struct ini_update_key *keys, int size)
{
#if IS_ENABLED(CONFIG_AIOT_DISP_PARAM_UPDATE)
	int i, j;

	if (!ini.data || !ini.info)
		return -1;

	memset(ini.info, 0, sizeof(*ini.info) * MAX_INI_KEY_COUNT);

	if (ini_new_get_group_info((char *)group_name, ini.data, ini.info)) {
		DRM_ERROR("ini_new_get_group_info %s failed!\n", group_name);
		return -1;
	}

	if (!strlen(ini.info[0].key_name)) {
#if !UBOOT_UPDATE_SUPPORT
		if (ini.uboot)
			return -1;
#endif

		/* disabled dts status if ini not config */
		for (i = 0; i < size; i++) {
			if (!strcmp(keys[i].name, "status")) {
				fdt_setprop_string(working_fdt, node, "status", "disabled");
				DRM_INFO("[ini] set %s status disabled\n", primary_key);
				return -1;
			}
		}
		return -1;
	}

	for (i = 0; i < size; i++) {
		if (keys[i].need_dirty && !ini.dirty)
			continue;

		for (j = 0; j < MAX_INI_KEY_COUNT; j++) {
			if (!strlen(ini.info[j].key_name))
				break;

			if (!strcmp(ini.info[j].key_name, keys[i].name) && ini.info[j].key_value) {
				if (ini.uboot && keys[i].skip_uboot)
					continue;

				switch (keys[i].type) {
				case KEY_TYPE_STRING:
#if !UBOOT_UPDATE_SUPPORT
					if (ini.uboot)
						break;
#endif
					fdt_setprop_string(working_fdt, node, ini.info[j].key_name,
							   ini.info[j].key_value);
					DRM_INFO("[ini] set %s %s %s\n", primary_key,
						 ini.info[j].key_name, ini.info[j].key_value);
					break;
				case KEY_TYPE_U32:
					fdt_setprop_u32(working_fdt, node, ini.info[j].key_name,
							simple_strtol(ini.info[j].key_value,
								      NULL, 10));
					DRM_INFO("[ini] set %s %s %d\n", primary_key,
						 ini.info[j].key_name,
						 simple_strtol(ini.info[j].key_value, NULL, 10));
					break;
				case KEY_TYPE_ARRAY:
#if !UBOOT_UPDATE_SUPPORT
					if (ini.uboot)
						break;
#endif
					fdt_setprop(working_fdt, node, ini.info[j].key_name,
						    ini.info[j].key_value, keys[i].cell_size);
					DRM_INFO("[ini] set %s %s %d\n", primary_key,
						 ini.info[j].key_name, keys[i].cell_size);
					break;
				default:
					DRM_ERROR("Something wrong: key type: %d\n", keys[i].type);
				}
			}
		}
	}
#endif
	return 0;
}

static int lcd_panel_timing_update(struct ini_private ini, int node, char *primary_key,
				   const char *group_name)
{
	int len;
	const fdt32_t *native_mode;

	node = fdt_subnode_offset(working_fdt, node, "display-timings");
	if (node < 0) {
		DRM_ERROR("%s offset display-timings failed!\n", primary_key);
		return -1;
	}

	native_mode = fdt_getprop(working_fdt, node, "native-mode", &len);
	if (!native_mode) {
		DRM_ERROR("%s getprop native_mode!\n", primary_key);
		return -1;
	}

	node = fdt_node_offset_by_phandle(working_fdt, fdt32_to_cpu(*native_mode));
	if (node < 0) {
		DRM_ERROR("%s offset_by_phandle failed! (native_mode)\n", primary_key);
		return -1;
	}

	struct ini_update_key keys_timing[] = {
		{
			.name = "clock-frequency",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "hback-porch",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "hactive",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "hfront-porch",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "hsync-len",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "vback-porch",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "vactive",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "vfront-porch",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "vsync-len",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
	};

	ini_update_param(ini, node, primary_key, group_name, keys_timing, ARRAY_SIZE(keys_timing));

	return 0;
}

static int lcd_lvds_panel_update(struct ini_private ini)
{
	int i;
	int node;
	char primary_key[64];
	char group_name[64];
	struct ScreenParameter *sparam = NULL;

	for (i = 0; i < MAX_CONNECTOR_COUNT; i++) {
		snprintf(primary_key, sizeof(primary_key) - 1, "/lvds%d_panel", i);

		node = fdt_path_offset(working_fdt, primary_key);
		if (node < 0) {
			DRM_INFO("Node: %s not found\n", primary_key);
			break;
		}

		snprintf(group_name, sizeof(group_name) - 1, "lvds%d_panel", i);

		if (!sunxi_drm_parameter_get_screen_param(DRM_MODE_CONNECTOR_LVDS, i, &sparam)) {
			if (!ini.dirty && !ini.uboot) {
				fdt_setprop_u32(working_fdt, node, "rotation",
						sparam->connector.orientation * 90);
				DRM_INFO("[param] set %s rotation %d\n", primary_key,
					 sparam->connector.orientation * 90);
			}
		}

		struct ini_update_key keys[] = {
			{
				.name = "status",
				.type = KEY_TYPE_STRING,
				.need_dirty = false,
			},
			{
				.name = "power-delay-ms",
				.type = KEY_TYPE_U32,
				.need_dirty = false,
			},
			{
				.name = "bus-format",
				.type = KEY_TYPE_U32,
				.need_dirty = false,
			},
			{
				.name = "rotation",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
			},
		};
		ini_update_param(ini, node, primary_key, group_name, keys, ARRAY_SIZE(keys));

		lcd_panel_timing_update(ini, node, primary_key, group_name);
	}

	return 0;
}

static int lcd_rgb_panel_update(struct ini_private ini)
{
	int i;
	int node;
	char primary_key[64];
	char group_name[64];
	struct ScreenParameter *sparam = NULL;

	for (i = 0; i < MAX_CONNECTOR_COUNT; i++) {
		snprintf(primary_key, sizeof(primary_key) - 1, "/rgb%d_panel", i);

		node = fdt_path_offset(working_fdt, primary_key);
		if (node < 0) {
			DRM_INFO("Node: %s not found\n", primary_key);
			break;
		}

		snprintf(group_name, sizeof(group_name) - 1, "rgb%d_panel", i);

		if (!sunxi_drm_parameter_get_screen_param(DRM_MODE_CONNECTOR_DPI, i, &sparam)) {
			if (!ini.dirty && !ini.uboot) {
				fdt_setprop_u32(working_fdt, node, "rotation",
						sparam->connector.orientation * 90);
				DRM_INFO("[param] set %s rotation %d\n", primary_key,
					 sparam->connector.orientation * 90);
			}
		}

		struct ini_update_key keys[] = {
			{
				.name = "status",
				.type = KEY_TYPE_STRING,
				.need_dirty = false,
			},
			{
				.name = "power-delay-ms",
				.type = KEY_TYPE_U32,
				.need_dirty = false,
			},
			{
				.name = "rotation",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
			},
		};
		ini_update_param(ini, node, primary_key, group_name, keys, ARRAY_SIZE(keys));

		lcd_panel_timing_update(ini, node, primary_key, group_name);
	}

	return 0;
}

static int __lcd_dsi_panel_update(struct ini_private ini, int node, char *primary_key)
{
	char group_name[64];

	/* primary_key + 1: remove '/'. (/dsi0_panel -> dsi0_panel) */
	snprintf(group_name, sizeof(group_name) - 1, "%s", primary_key + 1);

	struct ini_update_key keys[] = {
		{
			.name = "status",
			.type = KEY_TYPE_STRING,
			.need_dirty = false,
		},
		{
			.name = "power-delay-ms",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "reset-off-sequence",
			.type = KEY_TYPE_ARRAY,
			.need_dirty = false,
			.cell_size = 13,
		},
		{
			.name = "reset-on-sequence",
			.type = KEY_TYPE_ARRAY,
			.need_dirty = false,
			.cell_size = 13,
		},
		{
			.name = "dsi,flags",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "dsi,format",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "panel-init-sequence",
			.type = KEY_TYPE_ARRAY,
			.need_dirty = false,
			.cell_size = 13,
		},
		{
			.name = "panel-exit-sequence",
			.type = KEY_TYPE_ARRAY,
			.need_dirty = false,
			.cell_size = 13,
		},
		{
			.name = "width-mm",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "height-mm",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "rotation",
			.type = KEY_TYPE_U32,
			.need_dirty = true,
		},
	};
	ini_update_param(ini, node, primary_key, group_name, keys, ARRAY_SIZE(keys));

	lcd_panel_timing_update(ini, node, primary_key, group_name);

	return 0;
}

static int lcd_dsi_panel_update(struct ini_private ini)
{
	int i;
	int node;
	char primary_key[64];
	struct ScreenParameter *sparam = NULL;

	for (i = 0; i < MAX_CONNECTOR_COUNT; i++) {
		snprintf(primary_key, sizeof(primary_key) - 1, "/dsi%d_panel", i);

		node = fdt_path_offset(working_fdt, primary_key);
		if (node < 0) {
			DRM_INFO("Node: %s not found\n", primary_key);
			break;
		}

		if (!sunxi_drm_parameter_get_screen_param(DRM_MODE_CONNECTOR_DSI, i, &sparam)) {
			if (!ini.dirty && !ini.uboot) {
				fdt_setprop_u32(working_fdt, node, "rotation",
						sparam->connector.orientation * 90);
				DRM_INFO("[param] set %s rotation %d\n", primary_key,
					 sparam->connector.orientation * 90);
			}
		}

		__lcd_dsi_panel_update(ini, node, primary_key);
	}

	/* dual_dsi_panel */
	snprintf(primary_key, sizeof(primary_key) - 1, "/dual_dsi_panel");
	node = fdt_path_offset(working_fdt, primary_key);
	if (node < 0)
		DRM_INFO("Node: %s not found\n", primary_key);
	else
		__lcd_dsi_panel_update(ini, node, primary_key);

	return 0;
}

static int lcd_update(struct ini_private ini)
{
	int i;
	int node;
	char primary_key[64];
	char group_name[64];
	struct ScreenParameter *sparam = NULL;

	for (i = 0; i < MAX_CONNECTOR_COUNT; i++) {
		snprintf(primary_key, sizeof(primary_key) - 1, "/soc/dsi%d", i);

		node = fdt_path_offset(working_fdt, primary_key);
		if (node < 0) {
			DRM_INFO("Node: %s not found\n", primary_key);
			break;
		}

		if (!sunxi_drm_parameter_get_screen_param(DRM_MODE_CONNECTOR_DSI, i, &sparam)) {
			if (!ini.dirty && !ini.uboot) {
				fdt_setprop_u32(working_fdt, node, "priority",
						sparam->connector.priority);
				DRM_INFO("[param] set %s priority %d\n", primary_key,
					 sparam->connector.priority);

				fdt_setprop_u32(working_fdt, node, "best_crtc",
						sparam->connector.best_crtc);
				DRM_INFO("[param] set %s best_crtc %d\n", primary_key,
					 sparam->connector.best_crtc);
			}
		}

		snprintf(group_name, sizeof(group_name), "dsi%d", i);

		struct ini_update_key keys[] = {
			{
				.name = "status",
				.type = KEY_TYPE_STRING,
				.need_dirty = false,
			},
			{
				.name = "priority",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
				.skip_uboot = true,
			},
			{
				.name = "best_crtc",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
				.skip_uboot = true,
			},
		};
		ini_update_param(ini, node, primary_key, group_name, keys, ARRAY_SIZE(keys));
	}

	for (i = 0; i < MAX_CONNECTOR_COUNT; i++) {
		snprintf(primary_key, sizeof(primary_key) - 1, "/soc/lvds%d", i);

		node = fdt_path_offset(working_fdt, primary_key);
		if (node < 0) {
			DRM_INFO("Node: %s not found\n", primary_key);
			break;
		}

		if (!sunxi_drm_parameter_get_screen_param(DRM_MODE_CONNECTOR_LVDS, i, &sparam)) {
			if (!ini.dirty && !ini.uboot) {
				fdt_setprop_u32(working_fdt, node, "priority",
						sparam->connector.priority);
				DRM_INFO("[param] set %s priority %d\n", primary_key,
					 sparam->connector.priority);

				fdt_setprop_u32(working_fdt, node, "best_crtc",
						sparam->connector.best_crtc);
				DRM_INFO("[param] set %s best_crtc %d\n", primary_key,
					 sparam->connector.best_crtc);
			}
		}

		snprintf(group_name, sizeof(group_name) - 1, "lvds%d", i);

		struct ini_update_key keys[] = {
			{
				.name = "status",
				.type = KEY_TYPE_STRING,
				.need_dirty = false,
			},
			{
				.name = "dual-channel",
				.type = KEY_TYPE_U32,
				.need_dirty = false,
			},
			{
				.name = "priority",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
				.skip_uboot = true,
			},
			{
				.name = "best_crtc",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
				.skip_uboot = true,
			},
		};
		ini_update_param(ini, node, primary_key, group_name, keys, ARRAY_SIZE(keys));
	}

	for (i = 0; i < MAX_CONNECTOR_COUNT; i++) {
		snprintf(primary_key, sizeof(primary_key) - 1, "/soc/rgb%d", i);
		node = fdt_path_offset(working_fdt, primary_key);
		if (node < 0) {
			DRM_INFO("Node: %s not found\n", primary_key);
			continue;
		}

		if (!sunxi_drm_parameter_get_screen_param(DRM_MODE_CONNECTOR_DPI, i, &sparam)) {
			if (!ini.dirty && !ini.uboot) {
				fdt_setprop_u32(working_fdt, node, "priority",
						sparam->connector.priority);
				DRM_INFO("[param] set %s priority %d\n", primary_key,
					 sparam->connector.priority);

				fdt_setprop_u32(working_fdt, node, "best_crtc",
						sparam->connector.best_crtc);
				DRM_INFO("[param] set %s best_crtc %d\n", primary_key,
					 sparam->connector.best_crtc);
			}
		}

		snprintf(group_name, sizeof(group_name) - 1, "rgb%d", i);

		struct ini_update_key keys[] = {
			{
				.name = "status",
				.type = KEY_TYPE_STRING,
				.need_dirty = false,
			},
			{
				.name = "priority",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
				.skip_uboot = true,
			},
			{
				.name = "best_crtc",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
				.skip_uboot = true,
			},
		};
		ini_update_param(ini, node, primary_key, group_name, keys, ARRAY_SIZE(keys));
	}

	lcd_dsi_panel_update(ini);

	lcd_lvds_panel_update(ini);

	lcd_rgb_panel_update(ini);

	return 0;
}

static int edp_panel_update(struct ini_private ini)
{
	/* int i; */
	int node;
	char primary_key[64];
	char group_name[64];
	struct ScreenParameter *sparam = NULL;

	snprintf(primary_key, sizeof(primary_key) - 1, "/edp_panel");
	node = fdt_path_offset(working_fdt, primary_key);
	if (node < 0) {
		DRM_INFO("Node: %s not found\n", primary_key);
		return -1;
	}

	snprintf(group_name, sizeof(group_name) - 1, "edp_panel");

	if (!sunxi_drm_parameter_get_screen_param(DRM_MODE_CONNECTOR_eDP, 0, &sparam)) {
		if (!ini.dirty && !ini.uboot) {
			fdt_setprop_u32(working_fdt, node, "rotation",
					sparam->connector.orientation * 90);
			DRM_INFO("[param] set %s rotation %d\n", primary_key,
				 sparam->connector.orientation * 90);
		}
	}

	struct ini_update_key keys[] = {
		{
			.name = "status",
			.type = KEY_TYPE_STRING,
			.need_dirty = false,
		},
		{
			.name = "power-delay-ms",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "rotation",
			.type = KEY_TYPE_U32,
			.need_dirty = true,
		},
	};
	ini_update_param(ini, node, primary_key, group_name, keys, ARRAY_SIZE(keys));

	node = fdt_subnode_offset(working_fdt, node, "panel-timing");

	struct ini_update_key keys_timing[] = {
		{
			.name = "clock-frequency",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "hback-porch",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "hactive",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "hfront-porch",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "hsync-len",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "vback-porch",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "vactive",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "vfront-porch",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "vsync-len",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "hsync-active",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
		{
			.name = "vsync-active",
			.type = KEY_TYPE_U32,
			.need_dirty = false,
		},
	};
	ini_update_param(ini, node, primary_key, group_name, keys_timing, ARRAY_SIZE(keys_timing));

	return 0;
}

static int edp_update(struct ini_private ini)
{
	int i;
	int node;
	char primary_key[64];
	char group_name[64];
	struct ScreenParameter *sparam = NULL;

	for (i = 0; i < MAX_CONNECTOR_COUNT; i++) {
		snprintf(primary_key, sizeof(primary_key) - 1, "/soc/edp%d", i);

		node = fdt_path_offset(working_fdt, primary_key);
		if (node < 0) {
			DRM_INFO("Node: %s not found\n", primary_key);
			break;
		}

		if (!sunxi_drm_parameter_get_screen_param(DRM_MODE_CONNECTOR_eDP, i, &sparam)) {
			if (!ini.dirty && !ini.uboot) {
				fdt_setprop_u32(working_fdt, node, "priority",
						sparam->connector.priority);
				DRM_INFO("[param] set %s priority %d\n", primary_key,
					 sparam->connector.priority);

				fdt_setprop_u32(working_fdt, node, "best_crtc",
						sparam->connector.best_crtc);
				DRM_INFO("[param] set %s best_crtc %d\n", primary_key,
					 sparam->connector.best_crtc);

				fdt_setprop_u32(working_fdt, node, "rotation",
						sparam->connector.orientation * 90);
				DRM_INFO("[param] set %s rotation %d\n", primary_key,
					 sparam->connector.orientation * 90);
			}
		}

		/* FIXME: DP and EDP only choose one for now. */
		if (!sunxi_drm_parameter_get_screen_param(DRM_MODE_CONNECTOR_DisplayPort,
							  i, &sparam)) {
			if (!ini.dirty && !ini.uboot) {
				fdt_setprop_u32(working_fdt, node, "priority",
						sparam->connector.priority);
				DRM_INFO("[param] set %s priority %d\n", primary_key,
					 sparam->connector.priority);

				fdt_setprop_u32(working_fdt, node, "best_crtc",
						sparam->connector.best_crtc);
				DRM_INFO("[param] set %s best_crtc %d\n", primary_key,
					 sparam->connector.best_crtc);

				fdt_setprop_u32(working_fdt, node, "rotation",
						sparam->connector.orientation * 90);
				DRM_INFO("[param] set %s rotation %d\n", primary_key,
					 sparam->connector.orientation * 90);
			}
		}

		snprintf(group_name, sizeof(group_name) - 1, "edp%d", i);

		struct ini_update_key keys[] = {
			{
				.name = "status",
				.type = KEY_TYPE_STRING,
				.need_dirty = false,
			},
			{
				.name = "edp_color_depth",
				.type = KEY_TYPE_U32,
				.need_dirty = false,
			},
			{
				.name = "edp_color_fmt",
				.type = KEY_TYPE_U32,
				.need_dirty = false,
			},
			{
				.name = "priority",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
				.skip_uboot = true,
			},
			{
				.name = "best_crtc",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
				.skip_uboot = true,
			},
			{
				.name = "rotation",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
			},
		};

		ini_update_param(ini, node, primary_key, group_name, keys, ARRAY_SIZE(keys));
	}

	edp_panel_update(ini);

	return 0;
}

static int hdmi_update(struct ini_private ini)
{
	int i;
	int node;
	char primary_key[64];
	char group_name[64];
	struct ScreenParameter *sparam = NULL;
	int ret;

	for (i = 0; i < MAX_CONNECTOR_COUNT; i++) {
		ret = sunxi_drm_parameter_get_screen_param(DRM_MODE_CONNECTOR_HDMIA, i, &sparam);

		if (!ret && strlen((const char *)sparam->connector.dtsNodePath)) {
			node = fdt_path_offset(working_fdt,
					       (const char *)sparam->connector.dtsNodePath);
			if (node < 0) {
				DRM_INFO("Node: %s not found\n", sparam->connector.dtsNodePath);
				break;
			}
		} else {
			snprintf(primary_key, sizeof(primary_key) - 1, "/soc/hdmi%d", i);

			node = fdt_path_offset(working_fdt, primary_key);
			if (node < 0) {
				DRM_INFO("Node: %s not found\n", primary_key);
				break;
			}
		}

		if (!ret && !ini.dirty && !ini.uboot) {
			fdt_setprop_u32(working_fdt, node, "priority",
					sparam->connector.priority);
			DRM_INFO("[param] set %s priority %d\n", primary_key,
				 sparam->connector.priority);

			fdt_setprop_u32(working_fdt, node, "best_crtc",
					sparam->connector.best_crtc);
			DRM_INFO("[param] set %s best_crtc %d\n", primary_key,
				 sparam->connector.best_crtc);

			fdt_setprop_u32(working_fdt, node, "rotation",
					sparam->connector.orientation * 90);
			DRM_INFO("[param] set %s rotation %d\n", primary_key,
				 sparam->connector.orientation * 90);
		}

		snprintf(group_name, sizeof(group_name) - 1, "hdmi%d", i);

		struct ini_update_key keys[] = {
			{
				.name = "status",
				.type = KEY_TYPE_STRING,
				.need_dirty = false,
			},
			{
				.name = "uhdmi_fast_output",
				.type = KEY_TYPE_U32,
				.need_dirty = false,
			},
			{
				.name = "uhdmi_resistor_select",
				.type = KEY_TYPE_U32,
				.need_dirty = false,
			},
			{
				.name = "priority",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
				.skip_uboot = true,
			},
			{
				.name = "best_crtc",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
				.skip_uboot = true,
			},
			{
				.name = "rotation",
				.type = KEY_TYPE_U32,
				.need_dirty = true,
			},
		};

		ini_update_param(ini, node, primary_key, group_name, keys, ARRAY_SIZE(keys));
	}

	return 0;
}

static int tcon_update(struct ini_private ini)
{
	int i;
	int node;
	char primary_key[64];
	char group_name[64];

	for (i = 0; i < MAX_TCON_COUNT; i++) {
		snprintf(primary_key, sizeof(primary_key) - 1, "/soc/tcon%d", i);

		node = fdt_path_offset(working_fdt, primary_key);
		if (node < 0) {
			DRM_INFO("Node: %s not found\n", primary_key);
			break;
		}

		if (i < MAX_TCON_LCD_COUNT)
			snprintf(group_name, sizeof(group_name) - 1, "lcd%d", i);
		else
			snprintf(group_name, sizeof(group_name) - 1, "tv%d",
				 i - MAX_TCON_LCD_COUNT);

		struct ini_update_key keys[] = {
			{
				.name = "status",
				.type = KEY_TYPE_STRING,
				.need_dirty = false,
			},
		};

		ini_update_param(ini, node, primary_key, group_name, keys, ARRAY_SIZE(keys));
	}

	lcd_update(ini);

	edp_update(ini);

	hdmi_update(ini);

	return 0;
}

static int video_out_update(struct ini_private ini)
{
	int i;
	int node;
	char primary_key[64];
	char group_name[64];

	for (i = 0; i < MAX_VO_COUNT; i++) {
		snprintf(primary_key, sizeof(primary_key) - 1, "/soc/vo%d", i);

		node = fdt_path_offset(working_fdt, primary_key);
		if (node < 0) {
			DRM_INFO("Node: %s not found\n", primary_key);
			break;
		}

		snprintf(group_name, sizeof(group_name) - 1, "vo%d", i);

		struct ini_update_key keys[] = {
			{
				.name = "status",
				.type = KEY_TYPE_STRING,
				.need_dirty = false,
			},
		};

		ini_update_param(ini, node, primary_key, group_name, keys, ARRAY_SIZE(keys));
	}

	tcon_update(ini);

	return 0;
}

static int ini_update_route(struct ini_private ini, int node, const char *route_name)
{
#if IS_ENABLED(CONFIG_AIOT_DISP_PARAM_UPDATE)
	int i;

	if (!ini.data || !ini.info)
		return -1;

	memset(ini.info, 0, sizeof(*ini.info) * MAX_INI_KEY_COUNT);

	if (ini_new_get_group_info((char *)route_name, ini.data, ini.info)) {
		DRM_ERROR("ini_new_get_group_info %s failed!\n", route_name);
		return -1;
	}

	for (i = 0; i < MAX_INI_KEY_COUNT; i++) {
		if (!strlen(ini.info[i].key_name))
			break;

		if (ini.dirty && !strcmp(ini.info[i].key_name, "status") &&
		    ini.info[i].key_value) {
			fdt_setprop_u32(working_fdt, node, "enable", 1);
			DRM_WARN("[ini] enable %s\n", route_name);
		}

#if UBOOT_UPDATE_SUPPORT
		if (!strcmp(ini.info[i].key_name, "logo,uboot") && ini.info[i].key_value) {
			fdt_setprop_string(working_fdt, node, "logo,uboot", ini.info[i].key_value);
			DRM_WARN("[ini] set %s logo,uboot %s\n",
				 route_name, ini.info[i].key_value);
		}
#endif
	}
#endif
	return 0;
}

static int sunxi_drm_update(struct ini_private ini)
{
	int i, j, k;
	int root_node, node;
	char route_name[64];
	struct ScreenParameter *sparam = NULL;

	root_node = fdt_path_offset(working_fdt, "/soc/sunxi-drm");
	if (root_node < 0) {
		DRM_ERROR("Can't found /soc/sunxi-drm!\n");
		return -1;
	}

	/* /soc/sunxi-drm/route */
	root_node = fdt_subnode_offset(working_fdt, root_node, "route");
	for (i = 0; i < MAX_CRTC_COUNT; i++) {
		for (j = 0; j < ARRAY_SIZE(type_names); j++) {
			for (k = 0; k < MAX_CONNECTOR_COUNT; k++) {
				snprintf(route_name, sizeof(route_name) - 1, "disp%d_%s%d",
					 i, type_names[j].name, k);

				node = fdt_subnode_offset(working_fdt, root_node, route_name);
				if (node < 0) {
					DRM_INFO("Can't find route: %s\n", route_name);
					break;
				}

				fdt_setprop_u32(working_fdt, node, "enable", 0);

				if (!sunxi_drm_parameter_get_screen_param(type_names[j].type,
									  k, &sparam)) {
					if (!ini.dirty && sparam->enable &&
					    sparam->connector.de_id == i) {
						fdt_setprop_u32(working_fdt, node, "enable", 1);
						DRM_WARN("[param] enable %s\n", route_name);
					}
				}

				ini_update_route(ini, node, route_name);
			}
		}
	}

	return 0;
}

static int ini_initialize(struct ini_private *ini)
{
#if IS_ENABLED(CONFIG_AIOT_DISP_PARAM_UPDATE)
	int i;

	ini->data = get_file_from_partiton(INI_FILE_NAME, "bootloader");
	if (!ini->data) {
		ini->data = get_file_from_partiton(INI_FILE_NAME, "boot-resource");
		if (ini->data) {
			snprintf(ini->file_name, sizeof(ini->file_name) - 1, INI_FILE_NAME);
			snprintf(ini->part_name, sizeof(ini->part_name) - 1, "boot-resource");
		}
	} else {
		snprintf(ini->file_name, sizeof(ini->file_name) - 1, INI_FILE_NAME);
		snprintf(ini->part_name, sizeof(ini->part_name) - 1, "bootloader");
	}

	if (!ini->data)
		return -1;

	if (ini->info) {
		free(ini->info);
		ini->info = NULL;
	}

	ini->info = malloc(sizeof(*ini->info) * MAX_INI_KEY_COUNT);
	if (!ini->info) {
		DRM_ERROR("malloc ini.info failed!\n");
		return -1;
	}

	memset(ini->info, 0, sizeof(*ini->info) * MAX_INI_KEY_COUNT);

	/* get ini dirty flag */
	if (!ini_new_get_group_info("update", ini->data, ini->info)) {
		for (i = 0; i < MAX_INI_KEY_COUNT; i++) {
			if (!strlen(ini->info[i].key_name))
				break;

			if (!strcmp(ini->info[i].key_name, "dirty") && ini->info[i].key_value) {
				ini->dirty = ini->info[i].key_value[0] == '1' ? true : false;
				DRM_WARN("[ini] dirty: %d\n", ini->dirty);
				break;
			}
		}
	}

	return 0;
#else
	return -1;
#endif
}

static int ini_deinitialize(struct ini_private ini, bool uboot)
{
#if IS_ENABLED(CONFIG_AIOT_DISP_PARAM_UPDATE)
	/* after updating the kernel dts, clear the ini dirty flag. */
	if (ini.dirty && !uboot) {
		struct ini_option input = {
			.file_name = ini.file_name,
			.part_name = ini.part_name,
			.group_name = "update",
			.key = "dirty",
			.value = "0",
		};
		set_file_to_partiton(&input);
	}

	if (ini.info) {
		free(ini.info);
		ini.info = NULL;
	}
#endif
	return 0;
}

static int ini_fixup_dirty_control(struct ini_private *ini)
{
	if (!ini->data)
		return 0;

	if (!ini->dirty) {
		/* if no parameter file, use ini config by default. */
		if (!sunxi_drm_parameter_is_valid()) {
			ini->dirty = true;
			DRM_WARN("no parameter file, force set ini dirty=1\n");
			return 0;
		}

		/* if kernel startup failed, use ini config by default. */
		if (!sunxi_drm_parameter_has_enabled_connector()) {
			ini->dirty = true;
			DRM_WARN("no connector enable, force set ini dirty=1\n");
			return 0;
		}
	} else {
		/* use ini config and wait until hwc configures new parameter file.
		 * If startup fails, hwc fails to update the parameters,
		 * and the next startup should continue to use the ini configuration.
		 **/
		sunxi_drm_parameter_disable_all_connector();
		sunxi_drm_parameter_flush_data();
	}

	return 0;
}

int sunxi_drm_updata_param(bool uboot)
{
	int ret;
	struct ini_private ini;

	SUNXI_DRM_TIME_TRACE_START("Update Param", SUNXI_DRM_TRACE_LEVEL0);
	memset(&ini, 0, sizeof(ini));

	if (ini_initialize(&ini) && !sunxi_drm_parameter_is_valid()) {
		SUNXI_DRM_TIME_TRACE_END("Update Param");
		return -1;
	}

	ini_fixup_dirty_control(&ini);

	if (uboot) {
		ret = sunxi_drm_update(ini);
		if (ret)
			goto exit;
	}

	ini.uboot = uboot;
	video_out_update(ini);

exit:
	ini_deinitialize(ini, uboot);

	SUNXI_DRM_TIME_TRACE_END("Update Param");
	return 0;
}
