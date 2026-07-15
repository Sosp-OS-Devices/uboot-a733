/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Allwinner SoCs hdmi2.1 driver.
 *
 * Copyright (C) 2025 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#ifndef _SHDMI_EDID_H_
#define _SHDMI_EDID_H_

#include "shdmi_base.h"

int sunxi_edid_reset(struct shdmi_sink_s *sink);

int sunxi_edid_parse(u8 *raw_edid, struct shdmi_sink_s *sink);

ssize_t sunxi_sink_dump(char *buf, struct shdmi_sink_s *sink);

int sunxi_sink_support_hdmi2x(struct shdmi_sink_s *sink);

bool sunxi_sink_support_hdmivic(struct shdmi_sink_s *sink, u32 code);

bool sunxi_sink_support_ceavic(struct shdmi_sink_s *sink, u32 code);

bool sunxi_sink_support_scdc(struct shdmi_sink_s *sink);

bool sunxi_sink_support_yuv420_limit(struct shdmi_sink_s *sink, u32 code);

bool sunxi_sink_support_yuv420(struct shdmi_sink_s *sink, u32 code);

bool sunxi_sink_support_yuv422(struct shdmi_sink_s *sink);

bool sunxi_sink_support_yuv444(struct shdmi_sink_s *sink);

bool sunxi_sink_support_rgb_dc(struct shdmi_sink_s *sink, u8 bits);

bool sunxi_sink_support_yuv444_dc(struct shdmi_sink_s *sink, u8 bits);

bool sunxi_sink_support_yuv422_dc(struct shdmi_sink_s *sink, u8 bits);

bool sunxi_sink_support_yuv420_dc(struct shdmi_sink_s *sink, u8 bits);

bool sunxi_sink_support_sdr(struct shdmi_sink_s *sink);

bool sunxi_sink_support_hdr10(struct shdmi_sink_s *sink);

bool sunxi_sink_support_hlg(struct shdmi_sink_s *sink);

bool sunxi_sink_support_max_tmdsclk(struct shdmi_sink_s *sink, u32 clk);

bool sunxi_sink_support_isrc(struct shdmi_sink_s *sink);

#endif /* SHDMI_EDID_H_ */
