/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Allwinner SoCs hdmi driver.
 *
 * Copyright (C) 2025 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/

#include "shdmi_base.h"
#include "shdmi_edid.h"

#if IS_ENABLED(CONFIG_AW_DRM_HDMI20)
#include "dw-hdmi/dw_dev.h"
#include "dw-hdmi/dw_i2cm.h"
#include "dw-hdmi/dw_mc.h"
#include "dw-hdmi/dw_avp.h"
#include "dw-hdmi/dw_fc.h"
#include "dw-hdmi/dw_phy.h"
#include "dw-hdmi/dw_edid.h"
#include "dw-hdmi/phy_aw.h"
#include "dw-hdmi/phy_inno.h"
#include "dw-hdmi/phy_snps.h"
#include "dw-hdmi/phy_top.h"
#endif
