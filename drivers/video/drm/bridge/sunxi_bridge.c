/* SPDX-License-Identifier: GPL-2.0-only
 *
 * file: drm/bridge/sunxi_bridge.c
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
#include "include.h"
#include "../sunxi_drm_connector.h"

#if IS_ENABLED(CONFIG_AW_DRM_BRIDGE_GM7123)
extern const struct sunxi_bridge_info _gm7123_info;
#endif

#if IS_ENABLED(CONFIG_AW_DRM_BRIDGE_LT8711)
extern const struct sunxi_bridge_info _lt8711_info;
#endif

static const struct udevice_id sunxi_bridge_match[] = {
#if IS_ENABLED(CONFIG_AW_DRM_BRIDGE_GM7123)
	{.compatible = "corpro,gm7123", .data = (ulong)&_gm7123_info},
#endif

#if IS_ENABLED(CONFIG_AW_DRM_BRIDGE_LT8711)
	{.compatible = "lontium,lt8711", .data = (ulong)&_lt8711_info},
#endif
	{},
};

static void _sunxi_bridge_enable(struct sunxi_drm_bridge *bridge)
{
	struct sunxi_bridge_priv *priv = dev_get_priv(bridge->dev);
	int ret = 0;

	if (!priv) {
		BRIDGE_ERR("%s get priv failed!\n", __func__);
		return;
	}

	if (priv->info->drv_funs.drv_enable) {
		ret = priv->info->drv_funs.drv_enable(bridge);
		if (ret != 0) {
			BRIDGE_ERR("bridge enable failed. ret = %d\n", ret);
			return;
		}
	}
}

static void _sunxi_bridge_disable(struct sunxi_drm_bridge *bridge)
{
	struct sunxi_bridge_priv *priv = dev_get_priv(bridge->dev);
	int ret = 0;

	if (!priv) {
		BRIDGE_ERR("%s get priv failed!\n", __func__);
		return;
	}

	if (priv->info->drv_funs.drv_disable) {
		ret = priv->info->drv_funs.drv_disable(bridge);
		if (ret != 0) {
			BRIDGE_ERR("bridge disable failed. ret = %d\n", ret);
			return;
		}
	}
}

static void _sunxi_bridge_get_mode(struct sunxi_drm_bridge *bridge,
		struct drm_display_mode *mode)
{
	struct sunxi_bridge_priv *priv = dev_get_priv(bridge->dev);
	int ret = 0;

	if (!priv) {
		BRIDGE_ERR("%s get priv failed!\n", __func__);
		return;
	}

	if (priv->info->drv_funs.drv_get_mode) {
		ret = priv->info->drv_funs.drv_get_mode(bridge, mode);
		if (ret != 0) {
			BRIDGE_ERR("get mode failed. ret = %d\n", ret);
			return;
		}
	}

	BRIDGE_INFO("get mode %dx%d\n", mode->hdisplay, mode->vdisplay);
}

static const struct sunxi_drm_bridge_funcs _sunxi_bridge_drv_funcs = {
	.enable	  = _sunxi_bridge_enable,
	.disable  = _sunxi_bridge_disable,
	.get_mode = _sunxi_bridge_get_mode,
};

static int _sunxi_bridge_drv_init(struct udevice *dev)
{
	struct sunxi_bridge_priv *priv = dev_get_priv(dev);
	int ret = 0;

	if (priv->info->drv_funs.drv_init)
		ret = priv->info->drv_funs.drv_init(dev);

	return ret;
}

static int sunxi_bridge_probe(struct udevice *dev)
{
	struct sunxi_bridge_priv *priv = dev_get_priv(dev);

	BRIDGE_INFO("%s start\n", __func__);

	priv->dev = dev;
	priv->info = (struct sunxi_bridge_info *)dev_get_driver_data(dev);
	priv->bridge.dev = dev;
	priv->bridge.funcs = &_sunxi_bridge_drv_funcs;
	dev->driver_data = (ulong)&priv->bridge;

	sunxi_drm_connector_bind(&priv->connector, dev, priv->info->bridge_id,
			NULL, NULL, priv->info->connector_type);
	priv->bridge.conn = &(priv->connector);
	return _sunxi_bridge_drv_init(dev);
}

U_BOOT_DRIVER(sunxi_bridge_drv) = {
	.name = "sunxi_bridge_drv",
	.id	  = UCLASS_DISPLAY,
	.of_match = sunxi_bridge_match,
	.probe	  = sunxi_bridge_probe,
	.flags	  = DM_FLAG_PRE_RELOC,
	.priv_auto_alloc_size = sizeof(struct sunxi_bridge_priv),
};