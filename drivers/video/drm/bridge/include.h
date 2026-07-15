/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved.
 * lt8711.c
 *
 * Copyright (C) 2023 Allwinnertech Co.Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#ifndef _BRIDGE_INCLUDE_
#define _BRIDGE_INCLUDE_

#include <drm/drm_print.h>
#include <drm/drm_modes.h>
#include "../sunxi_drm_drv.h"
#include "../sunxi_drm_bridge.h"
#include "../sunxi_drm_connector.h"

#ifdef BRIDGE_INFO
#undef BRIDGE_INFO
#endif
#define BRIDGE_INFO(fmt, args...)               \
	do {                                        \
		DRM_INFO("[bridge][info] "fmt, ##args); \
	} while (0)

#ifdef BRIDGE_ERR
#undef BRIDGE_ERR
#endif
#define BRIDGE_ERR(fmt, args...)                  \
	do {                                          \
		DRM_ERROR("[bridge][error] "fmt, ##args); \
	} while (0)

struct sunxi_bridge_drv_funcs {
	int (*drv_init)(struct udevice *dev);
	int (*drv_get_mode)(struct sunxi_drm_bridge *bridge,
						struct drm_display_mode *mode);
	int (*drv_enable)(struct sunxi_drm_bridge *bridge);
	int (*drv_disable)(struct sunxi_drm_bridge *bridge);
};

struct sunxi_bridge_info {
	char name[32];
	unsigned int bridge_id;
	unsigned int connector_type;
	unsigned int max_rate;
	unsigned int gpio_cnt;
	unsigned int power_cnt;

	struct sunxi_bridge_drv_funcs drv_funs;
};

/**
 * @desc: sunxi bridge private data
 * You can add other private data to adapt your bridge device
 */
struct sunxi_bridge_priv {
	struct udevice *dev;
	struct udevice *parent_dev;
	struct sunxi_bridge_info *info;
	struct sunxi_drm_bridge bridge;
	struct sunxi_drm_connector connector;

	unsigned int power_num;
	unsigned int gpio_num;
	unsigned int power_delay;
#ifdef DRM_USE_DM_POWER
	struct udevice *power_supply[5];
#else
	uint32_t power_supply[5];
#endif
	ulong enable_gpio[5];
	ulong reset_gpio;

	bool (*get_hpd)(struct udevice *dev);
	struct edid* (*get_edid)(struct udevice *dev);
};

#endif /* _BRIDGE_INCLUDE_ */
