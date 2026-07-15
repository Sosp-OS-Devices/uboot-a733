/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#include <asm/io.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/bitops.h>

#include "lowlevel_hdmi/shdmi_edid.h"
#include "sunxi_hdmi.h"

u8 shdmi_log_leve;

#define func_not_register()  hdmi_trace("func: %s not has cb\n", __func__)

struct sunxi_hdmi_vic_mode {
	char name[25];
	int  vic_code;
};

struct sunxi_hdmi_plat_s sun8i_hdmi = {
	.version  = eHDMI_SUN8IW20P1,
	.hdmi_ver = SUNXI_HDMI20,
};

struct sunxi_hdmi_plat_s sun50i_hdmi = {
	.version  = eHDMI_SUN50IW9P1,
	.hdmi_ver = SUNXI_HDMI20,
};

struct sunxi_hdmi_plat_s sun55iw3_hdmi = {
	.version  = eHDMI_SUN55IW3P1,
	.hdmi_ver = SUNXI_HDMI20,
};

struct sunxi_hdmi_plat_s sun60i_hdmi = {
	.version  = eHDMI_SUN60IW2P1,
	.hdmi_ver = SUNXI_HDMI20,
	.use_top_phy = 1,

	/* config sun60i res cal and register and bitmask */
	.need_res_cal        = SUNXI_HDMI_ENABLE,
	.rescal_regs.rescal_ctrl_pa   = 0x03000160,
	.rescal_regs.bit_hdmi_res_sel = (u32)BIT(12),
	.rescal_regs.bit_rescal_mode  = (u32)BIT(2),
	.rescal_regs.bit_cal_ana_en   = (u32)BIT(1),
	.rescal_regs.bit_cal_en       = (u32)BIT(0),
	.rescal_regs.res0_ctrl_pa        = 0x03000164,
	.rescal_regs.res0_ctrl_bitmask   = (u32)GENMASK(31, 24),
};

struct sunxi_hdmi_plat_s sun55iw7_hdmi = {
	.version     = eHDMI_SUN55IW7P1,
	.hdmi_ver    = SUNXI_HDMI21,
	.use_top_phy = 0,
	.pixel_mode  = 4,
	.support_vrr = 1,
};

struct sunxi_hdmi_s      *sunxi_hdmi;

static int __maybe_unused _shdmi_is_use_hfvsif(u32 vic)
{
	switch (vic) {
	case HDMI_VIC_3840x2160P50:
	case HDMI_VIC_3840x2160P60:
	case HDMI_VIC_4096x2160P50:
	case HDMI_VIC_4096x2160P60:
		return 1;
	default:
		return 0;
	}
}

char *shdmi_color_fmt_string(enum disp_csc_type format)
{
	char *string[] = {"rgb", "yuv444", "yuv422", "yuv420"};

	return string[format];
}

char *shdmi_color_bits_string(enum disp_data_bits bits)
{
	char *string[] = {"8bits", "10bits", "12bits", "16bits"};

	return string[bits];
}

static struct shdmi_funcs_s __maybe_unused *_get_funcs(void)
{
#if IS_HDMI21
	if (sunxi_hdmi->dwqp_hdmi)
		return &sunxi_hdmi->dwqp_hdmi->funcs;
#else
	if (sunxi_hdmi->dw_hdmi)
		return &sunxi_hdmi->dw_hdmi->funcs;
#endif
	return NULL;
}

static struct shdmi_sink_s __maybe_unused *_get_sink(void)
{
#if IS_HDMI21
	if (sunxi_hdmi->dwqp_hdmi)
		return sunxi_hdmi->dwqp_hdmi->sink;
#else
	if (sunxi_hdmi->dw_hdmi)
		return sunxi_hdmi->dw_hdmi->sink_info;
#endif
	return NULL;
}

/*******************************************************************************
 * sunxi hdmi core register write and read function
 ******************************************************************************/
void sunxi_hdmi_ctrl_write(uintptr_t addr, u32 data)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->ctrl_write)
		funcs->ctrl_write(addr, data);
	else
		func_not_register();
}

u32 sunxi_hdmi_ctrl_read(uintptr_t addr)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->ctrl_read)
		return funcs->ctrl_read(addr);

	func_not_register();
	return 0;
}

/*******************************************************************************
 * sunxi hdmi core scdc write and read function
 ******************************************************************************/
void sunxi_hdmi_scdc_write(u8 addr, u8 data)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->scdc_write)
		funcs->scdc_write(addr, data);
	else
		func_not_register();

}

u8 sunxi_hdmi_scdc_read(u8 addr)
{
	struct shdmi_funcs_s *funcs = _get_funcs();
	u8 data = 0x0;

	if (funcs && funcs->scdc_read)
		funcs->scdc_read(addr, &data);
	else
		func_not_register();
	return data;
}
/*******************************************************************************
 * sunxi hdmi core phy function
 ******************************************************************************/
int sunxi_hdmi_phy_write(u8 addr, u32 data)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->phy_funcs->phy_write)
		return funcs->phy_funcs->phy_write(addr, (void *)&data);

	func_not_register();
	return 0;
}

int sunxi_hdmi_phy_read(u8 addr, u32 *data)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->phy_funcs->phy_read)
		return funcs->phy_funcs->phy_read(addr, (void *)data);

	func_not_register();
	return 0;
}

int sunxi_hdmi_phy_config(void)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->phy_funcs->phy_config)
		return funcs->phy_funcs->phy_config();

	func_not_register();
	return 0;
}

void sunxi_hdmi_phy_reset(void)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->phy_funcs->phy_reset)
		funcs->phy_funcs->phy_reset();
}

int sunxi_hdmi_phy_resume(void)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->phy_funcs->phy_resume)
		return funcs->phy_funcs->phy_resume();

	return 0;
}

int sunxi_hdmi_i2cm_set_ddc(u32 mode, u32 rate)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->i2cm_set_rate)
		return funcs->i2cm_set_rate(mode, rate);

	return 0;
}

int sunxi_hdmi_i2cm_xfer(struct i2c_msg *msgs, int num)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->i2cm_xfer)
		return funcs->i2cm_xfer(msgs, num);

	return 0;
}

int sunxi_hdmi_edid_parse(u8 *buffer)
{
	struct shdmi_sink_s *sink = _get_sink();

	sunxi_edid_reset(sink);

	sunxi_edid_parse(buffer, sink);

	return 0;
}

/*******************************************************************************
 * sunxi hdmi core audio function
 ******************************************************************************/
int sunxi_hdmi_audio_set_info(hdmi_audio_t *audio)
{
	struct shdmi_audio_info  info;
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (IS_ERR_OR_NULL(audio)) {
		shdmi_err(audio);
		return -1;
	}

	info.intf   = audio->hw_intf;
	info.coding = audio->data_raw;
	info.sample_freq = audio->sample_rate;
	info.sample_size = audio->sample_bit;
	info.channel_alloc = audio->ca;
	info.channel_num   = audio->channel_num;

	/* work, if set info, we will need reset fifo and reset i2s */
	if (funcs && funcs->audio_set)
		return funcs->audio_set(info);

	func_not_register();
	return -1;
}

int sunxi_hdmi_audio_enable(void)
{
	int ret = 0;
	struct shdmi_funcs_s *funcs = _get_funcs();

	mutex_lock(&sunxi_hdmi->lock_config);
	if (funcs && funcs->audio_on)
		ret = funcs->audio_on();
	else
		func_not_register();
	mutex_unlock(&sunxi_hdmi->lock_config);

	return ret;
}
/*******************************************************************************
 * sunxi hdmi core video info function
 ******************************************************************************/
u32 sunxi_hdmi_get_support_hdr_mode(void)
{
	u32 mode = 0;
	struct shdmi_sink_s *sink = _get_sink();

	if (sunxi_sink_support_sdr(sink))
		mode |= BIT(SHDMI_SDR);
	if (sunxi_sink_support_hdr10(sink))
		mode |= BIT(SHDMI_HDR10);
	if (sunxi_sink_support_hlg(sink))
		mode |= BIT(SHDMI_HLG);

	return mode;
}

int sunxi_hdmi_disp_select_eotf(struct disp_device_config *info)
{
	struct shdmi_sink_s *sink = _get_sink();

	if (IS_ERR_OR_NULL(info)) {
		shdmi_err(info);
		return -1;
	}

	if (info->eotf == DISP_EOTF_GAMMA22) {
		hdmi_trace("hdmi check continue use sdr eotf\n");
		return 0;
	}

	if (info->eotf == DISP_EOTF_SMPTE2084) {
		if (sunxi_sink_support_hdr10(sink)) {
			hdmi_trace("hdmi check continue use hdr10 eotf\n");
			return 0;
		}
		hdmi_inf("hdmi check sink unsupport hdr10. switch to sdr\n");
		goto switch_sdr;
	}

	if (info->eotf == DISP_EOTF_ARIB_STD_B67) {
		if (sunxi_sink_support_hlg(sink)) {
			hdmi_trace("hdmi check continue use hlg eotf\n");
			return 0;
		}
		hdmi_inf("hdmi check sink unsupport hlg. switch to sdr\n");
		goto switch_sdr;
	}

	hdmi_inf("hdmi check drv unsupport eotf %d. default use sdr\n", info->eotf);
switch_sdr:
	info->eotf = DISP_EOTF_GAMMA22;
	return 1;
}

int sunxi_hdmi_disp_select_space(struct disp_device_config *info, u32 vic_code)
{
	if ((info->eotf == DISP_EOTF_SMPTE2084) ||
			(info->eotf == DISP_EOTF_ARIB_STD_B67)) {
		info->cs = DISP_BT2020NC;
		hdmi_trace("hdmi check is hdr or hlg. prefer select BT2020NC space\n");
		return 0;
	}

	if (info->eotf == DISP_EOTF_GAMMA22) {
		switch (vic_code) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 6:
		case 7:
		case 17:
		case 18:
			info->cs = DISP_BT601;
			hdmi_trace("hdmi check use BT601\n");
			return 0;
		default:
			info->cs = DISP_BT709;
			hdmi_trace("hdmi check use BT709\n");
			return 0;
		}
	}

	hdmi_inf("hdmi check eotf %d unsupport, default use BT709\n", info->eotf);
	info->cs = DISP_BT709;
	return 1;
}

int sunxi_hdmi_disp_select_format(struct disp_device_config *info, u32 vic_code)
{
	struct shdmi_sink_s *sink = _get_sink();

	/* if this timing only support 420, we need select 420-8bit */
	if (sunxi_sink_support_yuv420_limit(sink, vic_code)) {
		info->format = DISP_CSC_TYPE_YUV420;
		if (info->bits != DISP_DATA_8BITS) {
			if (!sunxi_sink_support_yuv420_dc(sink, info->bits)) {
				hdmi_inf("hdmi check vic %d yuv420-%s unsupport and switch to yuv420-8bits\n",
						vic_code, shdmi_color_bits_string(info->bits));
				info->bits = DISP_DATA_8BITS;
			}
		}

		hdmi_inf("hdmi check vic %d only support 420-%s\n",
				vic_code, shdmi_color_bits_string(info->bits));
		return 0;
	}

	/* if hdr mode. perfer use yuv422-10bits */
	if ((info->eotf == DISP_EOTF_SMPTE2084) ||
			(info->eotf == DISP_EOTF_ARIB_STD_B67)) {
		info->format = DISP_CSC_TYPE_YUV422;
		info->bits   = DISP_DATA_10BITS;
		hdmi_inf("hdmi check is hdr mode. default use yuv422-10bits\n");
		return 0;
	}

	/* if current format support, not-change return. */
	switch (info->format) {
	case DISP_CSC_TYPE_YUV420:
		if (sunxi_sink_support_yuv420(sink, vic_code)) {
			if (info->bits == DISP_DATA_8BITS || sunxi_sink_support_yuv420_dc(sink, info->bits))
				hdmi_trace("hdmi check continue use yuv420-%s\n",
					shdmi_color_bits_string(info->bits));
				return 0;
		}
		break;
	case DISP_CSC_TYPE_YUV422:
		if (sunxi_sink_support_yuv422(sink)) {
			if (info->bits == DISP_DATA_8BITS || sunxi_sink_support_yuv422_dc(sink, info->bits)) {
				hdmi_trace("hdmi check continue use yuv422-%s\n",
					shdmi_color_bits_string(info->bits));
				return 0;
			}
		}
		break;
	case DISP_CSC_TYPE_YUV444:
		if (sunxi_sink_support_yuv444(sink)) {
			if (info->bits == DISP_DATA_8BITS || sunxi_sink_support_yuv444_dc(sink, info->bits)) {
				hdmi_trace("hdmi check continue use yuv444-%s\n",
					shdmi_color_bits_string(info->bits));
				return 0;
			}
		}
		break;
	default:
		info->format = DISP_CSC_TYPE_RGB;
		if (info->bits == DISP_DATA_8BITS || sunxi_sink_support_rgb_dc(sink, info->bits)) {
			hdmi_trace("hdmi check continue use rgb-%s\n",
					shdmi_color_bits_string(info->bits));
			return 0;
		}
		break;
	}

	/* if format unsupport, select perfer format */
	info->format = sunxi_sink_support_yuv444(sink) ? DISP_CSC_TYPE_YUV444 : DISP_CSC_TYPE_RGB;
	info->bits = DISP_DATA_8BITS;
	hdmi_inf("hdmi check switch use %s-8bits\n", shdmi_color_fmt_string(info->format));

	return 1;
}

int sunxi_hdmi_video_check_tmds_clock(u8 format, u8 bits, u32 pixel_clk)
{
	struct shdmi_sink_s *sink = _get_sink();
	u32 tmds_clk = pixel_clk, ref_clk = pixel_clk;

	if (format == DISP_CSC_TYPE_YUV422)
		goto check_clk;

	if (format == DISP_CSC_TYPE_YUV420)
		ref_clk /= 2;

	switch (bits) {
	case DISP_DATA_10BITS:
		tmds_clk = (ref_clk * 125 / 100);
		break;
	case DISP_DATA_12BITS:
		tmds_clk = (ref_clk * 3 / 2);
		break;
	default:
		tmds_clk = ref_clk;
		break;
	}

check_clk:
	return sunxi_sink_support_max_tmdsclk(sink, tmds_clk);
}

struct disp_device_config *sunxi_hdmi_get_disp_info(void)
{
	struct disp_device_config *info = &sunxi_hdmi->disp_info;

	return info;
}

void sunxi_hdmi_save_disp_info(struct disp_device_config *info)
{
	memcpy(&sunxi_hdmi->disp_info, info, sizeof(struct disp_device_config));
}

void sunxi_hdmi_reset_disp_info(void)
{
	memset(&sunxi_hdmi->disp_info, 0x0, sizeof(struct disp_device_config));
}

int sunxi_hdmi_set_disp_info(struct disp_device_config *info)
{
	int ret = 0;
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->video_set_info) {
		ret = funcs->video_set_info((disp_cfg_t *)info);
		sunxi_hdmi_save_disp_info(info);
		return ret;
	}

	func_not_register();
	return 0;
}

void sunxi_hdmi_video_set_pattern(u8 bit, u32 value)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->video_set_pattern)
		funcs->video_set_pattern(bit, value);
}

u32 sunxi_hdmi_get_color_capality(u32 vic)
{
	struct shdmi_sink_s *sink = _get_sink();
	u32 value = 0x0;

	/* RGB-Bits default support */
	value |= BIT(SHDMI_RGB888_8BITS);

	/* Check RGB DeepColor */
	if (sunxi_sink_support_rgb_dc(sink, DISP_DATA_10BITS))
		value |= BIT(SHDMI_RGB888_10BITS);
	if (sunxi_sink_support_rgb_dc(sink, DISP_DATA_12BITS))
		value |= BIT(SHDMI_RGB888_12BITS);
	if (sunxi_sink_support_rgb_dc(sink, DISP_DATA_16BITS))
		value |= BIT(SHDMI_RGB888_16BITS);

	/* Check YUV444 DeepColor */
	if (sunxi_sink_support_yuv444(sink)) {
		value |= BIT(SHDMI_YUV444_8BITS);
		if (sunxi_sink_support_yuv444_dc(sink, DISP_DATA_10BITS))
			value |= BIT(SHDMI_YUV444_10BITS);
		if (sunxi_sink_support_yuv444_dc(sink, DISP_DATA_12BITS))
			value |= BIT(SHDMI_YUV444_12BITS);
		if (sunxi_sink_support_yuv444_dc(sink, DISP_DATA_16BITS))
			value |= BIT(SHDMI_YUV444_16BITS);
	}

	/* check yuv422 format and bits */
	if (sunxi_sink_support_yuv422(sink)) {
		value |= BIT(SHDMI_YUV422_8BITS);
		if (sunxi_sink_support_yuv422_dc(sink, DISP_DATA_10BITS))
			value |= BIT(SHDMI_YUV422_10BITS);
		if (sunxi_sink_support_yuv422_dc(sink, DISP_DATA_12BITS))
			value |= BIT(SHDMI_YUV422_12BITS);
		if (sunxi_sink_support_yuv422_dc(sink, DISP_DATA_16BITS))
			value |= BIT(SHDMI_YUV422_16BITS);
	}

	/* check yuv420 format and bits */
	if (sunxi_sink_support_yuv420_limit(sink, vic)) {
		value = BIT(SHDMI_YUV420_8BITS);
		if (sunxi_sink_support_yuv420_dc(sink, DISP_DATA_10BITS))
			value |= BIT(SHDMI_YUV420_10BITS);
		if (sunxi_sink_support_yuv420_dc(sink, DISP_DATA_12BITS))
			value |= BIT(SHDMI_YUV420_12BITS);
		if (sunxi_sink_support_yuv420_dc(sink, DISP_DATA_16BITS))
			value |= BIT(SHDMI_YUV420_16BITS);
		goto exit_update;
	}

	if (sunxi_sink_support_yuv420(sink, vic)) {
		value = BIT(SHDMI_YUV420_8BITS);
		if (sunxi_sink_support_yuv420_dc(sink, DISP_DATA_10BITS))
			value |= BIT(SHDMI_YUV420_10BITS);
		if (sunxi_sink_support_yuv420_dc(sink, DISP_DATA_12BITS))
			value |= BIT(SHDMI_YUV420_12BITS);
		if (sunxi_sink_support_yuv420_dc(sink, DISP_DATA_16BITS))
			value |= BIT(SHDMI_YUV420_16BITS);
	}

exit_update:
	hdmi_trace("sunxi hdmi get color capality: 0x%x\n", value);
	return value;
}

int sunxi_hdmi_select_output_packets(u8 flags)
{
	struct shdmi_funcs_s *funcs = _get_funcs();
	bool is_3d = flags & DRM_MODE_FLAG_3D_MASK ? true : false;

	if (funcs && funcs->video_set_packet)
		return funcs->video_set_packet(is_3d);

	func_not_register();
	return 0;
}

u8 sunxi_hdmi_get_pixel_mode(void)
{
	if (sunxi_hdmi->plat_data)
		return sunxi_hdmi->plat_data->pixel_mode;
	return 0;
}

u8 sunxi_hdmi_get_hpd(void)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->get_hpd)
		return funcs->get_hpd();

	return 0;
}

u8 sunxi_hdmi_get_loglevel(void)
{
	return shdmi_log_leve;
}

int suxni_hdmi_set_loglevel(u8 level)
{
	shdmi_log_leve = level;
	return shdmi_log_leve;
}

/* first disable output, send mute */
int sunxi_hdmi_disconfig(void)
{
	int ret = 0;
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->hdmi_disconfig)
		ret = funcs->hdmi_disconfig();
	else
		func_not_register();

	sunxi_hdmi_reset_disp_info();
	return ret;
}

int sunxi_hdmi_check(void)
{
	struct shdmi_funcs_s *funcs = _get_funcs();
	if (funcs && funcs->hdmi_check)
		return funcs->hdmi_check();

	return 0;
}

bool sunxi_hdmi_check_vrr(void)
{
	struct shdmi_sink_s *sink = _get_sink();

	if (!sunxi_hdmi->plat_data->support_vrr)
		return false;

	if (!sink->HdmiForum.mValid)
		return false;

	return false;
}

int sunxi_hdmi_config(void)
{
	int ret = 0;
	struct shdmi_funcs_s *funcs = _get_funcs();

	mutex_lock(&sunxi_hdmi->lock_config);

	if (funcs && funcs->hdmi_config)
		ret = funcs->hdmi_config();
	else
		func_not_register();

	mutex_unlock(&sunxi_hdmi->lock_config);
	return ret;
}

int sunxi_hdmi_smooth_config(void)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->hdmi_smooth)
		return funcs->hdmi_smooth();

	func_not_register();
	return 0;
}

int sunxi_hdmi_set_disp_mode(struct drm_display_mode *mode)
{
	struct shdmi_dtd_s video;
	u32 rate = 0;
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (IS_ERR_OR_NULL(mode)) {
		shdmi_err(mode);
		return -1;
	}

	memset(&video, 0x0, sizeof(struct shdmi_dtd_s));

	/* video.mCode = (u32)drm_match_cea_mode(mode); */
	video.mPixelClock = mode->clock;
	video.mInterlaced = (mode->flags & DRM_MODE_FLAG_INTERLACE) ? 0x1 : 0x0;
	video.mPixelRepetitionInput = (mode->flags & DRM_MODE_FLAG_DBLCLK) ? 0x1 : 0x0;

	video.mHActive     = mode->hdisplay;
	video.mHBlanking   = mode->htotal - mode->hdisplay;
	video.mHSyncOffset = mode->hsync_start - mode->hdisplay;
	video.mHSyncPulseWidth = mode->hsync_end - mode->hdisplay - video.mHSyncOffset;
	video.mHSyncPolarity   = (mode->flags & DRM_MODE_FLAG_PHSYNC) ? 0x1 : 0x0;

	video.mVActive     = mode->vdisplay;
	video.mVBlanking   = mode->vtotal - mode->vdisplay;
	video.mVSyncOffset = mode->vsync_start - mode->vdisplay;
	video.mVSyncPulseWidth = mode->vsync_end - mode->vdisplay - video.mVSyncOffset;
	video.mVSyncPolarity   = (mode->flags & DRM_MODE_FLAG_PVSYNC) ? 0x1 : 0x0;

	if (video.mInterlaced) {
		video.mVActive     /= 2;
		video.mVBlanking   /= 2;
		video.mVSyncOffset /= 2;
		video.mVSyncPulseWidth /= 2;
	}

	switch (mode->picture_aspect_ratio) {
	case HDMI_PICTURE_ASPECT_4_3:
		video.mHImageSize = 4;
		video.mVImageSize = 3;
		break;
	case HDMI_PICTURE_ASPECT_16_9:
		video.mHImageSize = 16;
		video.mVImageSize = 9;
		break;
	case HDMI_PICTURE_ASPECT_64_27:
		video.mHImageSize = 64;
		video.mVImageSize = 27;
		break;
	case HDMI_PICTURE_ASPECT_256_135:
		video.mHImageSize = 256;
		video.mVImageSize = 135;
		break;
	default:
		video.mHImageSize = 16;
		video.mVImageSize = 9;
		break;
	}

	rate = drm_mode_vrefresh(mode);

	if (funcs && funcs->video_set_dtd)
		return funcs->video_set_dtd(&video, rate);

	func_not_register();
	return 0;
}

static int _sunxi_hdmi_board_init(struct sunxi_hdmi_s *hdmi)
{
	u8 res_src   = hdmi->resistor_src;
	u8 rescal_en = hdmi->plat_data->need_res_cal;
	struct sunxi_hdmi_rescal_s *res_reg = &hdmi->plat_data->rescal_regs;

	void __iomem *reg_addr;

	if (!rescal_en) {
		hdmi_trace("this plat not need use resistance calibration\n");
		return 0;
	}

	if (res_src == 0x1) {
		reg_addr = ioremap(res_reg->rescal_ctrl_pa, 4);
		writel(readl(reg_addr) | res_reg->bit_hdmi_res_sel, reg_addr);
		iounmap(reg_addr);

		reg_addr = ioremap(res_reg->res0_ctrl_pa, 4);
		writel(readl(reg_addr) & (~res_reg->res0_ctrl_bitmask), reg_addr);
		iounmap(reg_addr);
	} else {
		reg_addr = ioremap(res_reg->rescal_ctrl_pa, 4);

		writel((readl(reg_addr) & (~res_reg->bit_cal_en)), reg_addr);
		writel((readl(reg_addr) & (~res_reg->bit_hdmi_res_sel)), reg_addr);
		writel((readl(reg_addr) | (res_reg->bit_cal_ana_en)), reg_addr);
		writel((readl(reg_addr) | (res_reg->bit_cal_en)), reg_addr);

		udelay(200);

		writel((readl(reg_addr) & (~res_reg->bit_cal_ana_en)), reg_addr);
		writel((readl(reg_addr) & (~res_reg->bit_cal_en)), reg_addr);

		iounmap(reg_addr);
	}
	return 0;
}
static const struct udevice_id hdmi_plat_match[] = {
	{ .compatible = "arm,sun8iw20p1", .data = (ulong)&sun8i_hdmi },
	{ .compatible = "arm,sun50iw9p1", .data = (ulong)&sun50i_hdmi },
	{ .compatible = "arm,sun55iw3p1", .data = (ulong)&sun55iw3_hdmi },
	{ .compatible = "arm64,sun55iw7p1", .data = (ulong)&sun55iw7_hdmi },
	{ .compatible = "arm,sun60iw2p1", .data = (ulong)&sun60i_hdmi },
	{ },
};

int sunxi_hdmi_init(struct sunxi_hdmi_s *hdmi)
{
	int ret = 0, i = 0;
	ofnode root_node;
	const char *compatible = NULL;

	root_node = ofnode_path("/");
	if (!ofnode_valid(root_node)) {
		hdmi_err("failed to find root node\n");
		return -1;
    }

	ret = ofnode_read_string_index(root_node, "compatible", 0x1, &compatible);
    if (ret != 0) {
		hdmi_err("failed to read root node compatible value\n");
		return -1;
    }

	for (i = 0; hdmi_plat_match[i].compatible; i++) {
		if (!strcmp(compatible, hdmi_plat_match[i].compatible)) {
			hdmi->plat_data = (struct sunxi_hdmi_plat_s *)hdmi_plat_match[i].data;
			break;
		}
	}

	if (IS_ERR_OR_NULL(hdmi->plat_data)) {
		shdmi_err(hdmi->plat_data);
		return -1;
	}

	_sunxi_hdmi_board_init(hdmi);


#if IS_HDMI20
	hdmi->dw_hdmi = dw_hdmi_create(hdmi->dev);
	if (IS_ERR_OR_NULL(hdmi->dw_hdmi)) {
		hdmi_err("sunxi hdmi create dw hdmi failed\n");
	}

	hdmi->dw_hdmi->dev       = hdmi->dev;
	hdmi->dw_hdmi->ctrl_addr = hdmi->reg_base;
	hdmi->dw_hdmi->clock_src = hdmi->clock_src;
	hdmi->dw_hdmi->plat_id   = hdmi->plat_data->version;
	hdmi->dw_hdmi->phy_top   = hdmi->plat_data->use_top_phy;
	dw_hdmi_init();
#endif
	sunxi_hdmi = hdmi;
	return 0;
}

void sunxi_hdmi_exit(void)
{
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->hdmi_exit)
		funcs->hdmi_exit();
	else
		func_not_register();

	shdmi_free_point(sunxi_hdmi);
}

ssize_t sunxi_hdmi_tx_dump(char *buf)
{
	int n = 0;
	struct shdmi_funcs_s *funcs = _get_funcs();

	if (funcs && funcs->hdmi_tx_dump)
		n += funcs->hdmi_tx_dump(buf);
	else
		func_not_register();

	return n;
}

ssize_t sunxi_hdmi_rx_dump(char *buf)
{
	int n = 0;
	struct shdmi_sink_s *sink = _get_sink();

	n += sunxi_sink_dump(buf + n, sink);
	return n;
}
