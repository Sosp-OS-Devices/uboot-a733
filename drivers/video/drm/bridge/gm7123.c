/* SPDX-License-Identifier: GPL-2.0-only
 *
 * file: drm/bridge/gm7123.c
 *
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
 * Author: zhangxuswc <zhangxuswc@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#include <asm/gpio.h>
#include <dm.h>
#include <drm/drm_edid.h>

#include "../sunxi_drm_helper_funcs.h"
#include "include.h"

static int gm7123_get_mode(struct sunxi_drm_bridge *bridge, struct drm_display_mode *mode)
{
	ofnode bridge_of_node, dt, timing;
	struct ofnode_phandle_args args;
	u32 bus_flags = 0, value = 0;
	int ret;
	dt = ofnode_find_subnode(bridge->dev->node, "display-timings");
	if (ofnode_valid(dt)) {
		ret = ofnode_parse_phandle_with_args(dt, "native-mode", NULL, 0, 0, &args);
		if (ret) {
			printk("%s[%d] err !\n", __FUNCTION__, __LINE__);
			return 0;
		}
		timing = args.node;
	} else {
		timing = ofnode_find_subnode(bridge_of_node, "panel-timing");
	}
	if (!ofnode_valid(timing)) {
		DRM_ERROR("failed to get display timings from DT\n");
		return 0;
	}
	sunxi_ofnode_get_display_mode(timing, mode, &bus_flags);
	if (!ofnode_read_u32(bridge_of_node, "width-mm", &value))
		mode->width_mm = value;
	if (!ofnode_read_u32(bridge_of_node, "height-mm", &value))
		mode->height_mm = value;
	mode->type |= DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	return 0;
}

static int gm7123_init(struct udevice *dev)
{
	struct sunxi_bridge_priv *priv = dev_get_priv(dev);
	int i = 0;
	char gpio_name[40] = {0};
	ulong ret;

	for (i = 0; i < 1; i++) {
		snprintf(gpio_name, 40, "enable%d-gpios", i);
		ret = sunxi_drm_gpio_node_request(dev->node, gpio_name);
		if (ret < 0) {
			DRM_ERROR("[%s]: get %s ret = %d\n", __func__, gpio_name, ret);
			continue;
		}
		priv->enable_gpio[i] = ret;
		sunxi_drm_gpio_set_value(priv->enable_gpio[i], 1);
	}
	DRM_ERROR("%s done1\n", __func__);
	return 0;
}

const struct sunxi_bridge_info _gm7123_info = {
	.name = "corpro,gm7123",
	.max_rate = 297000, // pixel clock, Unit: KHz
	.bridge_id = 0,
	.connector_type = DRM_MODE_CONNECTOR_VGA,
	.drv_funs.drv_init = gm7123_init,
	.drv_funs.drv_get_mode = gm7123_get_mode,
};
