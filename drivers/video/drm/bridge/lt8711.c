/* SPDX-License-Identifier: GPL-2.0-only
 *
 * file: drm/bridge/lt8711.c
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

#include <dm.h>
#include <asm/gpio.h>
#include <drm/drm_edid.h>

#include "include.h"
#include "../sunxi_drm_helper_funcs.h"

const struct drm_display_mode _lt8711_default_mode[1] = {
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500,
		1920, 2008, 2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC)},
};

static int _lt8711_get_mode(struct sunxi_drm_bridge *bridge,
		struct drm_display_mode *mode)
{
	struct sunxi_bridge_priv *priv = dev_get_priv(bridge->dev);
	struct edid *edid = NULL;
	struct drm_display_mode *temp_mode;
	int mode_num = 0;

	if (!priv) {
		DRM_ERROR("%s get priv failed!\n", __func__);
		return 0;
	}

	if (priv->get_edid)
		edid = priv->get_edid(priv->parent_dev);

	if (!edid) {
		DRM_INFO("[%s] get edid is null\n", __func__);
		goto use_default;
	}

	mode_num += drm_add_edid_modes(bridge->conn, edid);
	if (!mode_num) {
		DRM_INFO("[%s] get edid mode num is null\n", __func__);
		goto use_default;
	}

	list_for_each_entry(temp_mode, &bridge->conn->probed_modes, head) {
		if ((temp_mode->type & DRM_MODE_TYPE_PREFERRED) &&
				(temp_mode->clock < priv->info->max_rate)) {
			memcpy(mode, temp_mode, sizeof(struct drm_display_mode));
			DRM_INFO("bridge select mode1: %dx%d\n", mode->hdisplay, mode->vdisplay);
			return 0;
		}
	}

	list_for_each_entry(temp_mode, &bridge->conn->probed_modes, head) {
		if (temp_mode->clock < priv->info->max_rate) {
			memcpy(mode, temp_mode, sizeof(struct drm_display_mode));
			DRM_INFO("bridge select mode2: %dx%d\n", mode->hdisplay, mode->vdisplay);
			return 0;
		}
	}

use_default:
	memcpy(mode, &_lt8711_default_mode[0], sizeof(struct drm_display_mode));
	return 0;
}

static int _lt8711_init(struct udevice *dev)
{
	struct sunxi_bridge_priv *priv = dev_get_priv(dev);
	int i = 0;
	char gpio_name[40] = {0};
	char power_name[40] = {0};
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
	DRM_ERROR("%s %d\n", __func__, __LINE__);

	for (i = 0; i < 1; i++) {
		snprintf(power_name, 32, "power%d-supply", i);
		if (dev_read_bool(dev, power_name)) {
#ifdef DRM_USE_DM_POWER
			ret = uclass_get_device_by_phandle(UCLASS_REGULATOR, dev,
							   power_name, &priv->power_supply[i]);
			if (IS_ERR(priv->power_supply[i]) || ret) {
				DRM_ERROR("failed to request regulator(%s) for priv, ret:%d\n",
					power_name, ret);
				priv->power_supply[i] = NULL;
			}
			if (priv->power_supply[i])
				ret = regulator_set_enable(priv->power_supply[i], true);
#else
			ret = dev_read_u32(dev, power_name, &priv->power_supply[i]);
			if (!priv->power_supply[i] || ret) {
				DRM_ERROR("failed to request regulator(%s) for priv, ret:%d\n",
					power_name, ret);
				priv->power_supply[i] = 0;
			}
			if (priv->power_supply[i])
				ret = sunxi_drm_power_enable(priv->power_supply[i], 0);
#endif
		}
	}

	DRM_ERROR("%s done1\n", __func__);
	return 0;
}

const struct sunxi_bridge_info _lt8711_info = {
	.name = "LT8711",
	.max_rate  = 297000, // pixel clock, Unit: KHz
	.bridge_id = 0,
	.connector_type = DRM_MODE_CONNECTOR_HDMIA,
	.drv_funs.drv_init     = _lt8711_init,
	.drv_funs.drv_get_mode = _lt8711_get_mode,
};
