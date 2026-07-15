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
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/bitops.h>

#include "dw_dev.h"
#include "dw_fc.h"
#include "dw_mc.h"
#include "dw_edid.h"
#include "dw_i2cm.h"
#include "phy_top.h"
#include "dw_phy.h"
#include "dw_fc.h"
#include "phy_aw.h"
#include "phy_inno.h"
#include "phy_snps.h"
#include "dw_avp.h"

u8 log_level;
static struct dw_hdmi_s *hdmi;

struct dw_hdmi_phy_plat {
	u32 plat_id;
	bool inter_phypll;
	struct shdmi_phy_funcs phy_func;
};

struct dw_hdmi_phy_plat sun8iw20p1_phy = {
	.plat_id = eHDMI_SUN8IW20P1,
	.phy_func.phy_init   = aw_phy_init,
	.phy_func.phy_config = aw_phy_config,
	.phy_func.phy_resume = aw_phy_resume,
	.phy_func.phy_reset  = aw_phy_reset,
	.phy_func.phy_read   = aw_phy_read,
	.phy_func.phy_write  = aw_phy_write,
};

struct dw_hdmi_phy_plat sun50iw9p1_phy = {
	.plat_id = eHDMI_SUN50IW9P1,
	.phy_func.phy_init      = snps_phy_init,
	.phy_func.phy_config    = snps_phy_config,
	.phy_func.phy_disconfig = snps_phy_disconfig,
	.phy_func.phy_read      = snps_phy_read,
	.phy_func.phy_write     = snps_phy_write,
	.phy_func.phy_dump      = snps_phy_dump,
};

struct dw_hdmi_phy_plat sun55iw3p1_phy = {
	.plat_id = eHDMI_SUN55IW3P1,
	.inter_phypll = 1,
	.phy_func.phy_init      = inno_phy_init,
	.phy_func.phy_config    = inno_phy_config,
	.phy_func.phy_read      = inno_phy_read,
	.phy_func.phy_write     = inno_phy_write,
	.phy_func.phy_dump      = inno_phy_dump,
};

struct dw_hdmi_phy_plat sun60iw2p1_phy = {
	.plat_id = eHDMI_SUN60IW2P1,
	.phy_func.phy_init      = snps_phy_init,
	.phy_func.phy_config    = snps_phy_config,
	.phy_func.phy_disconfig = snps_phy_disconfig,
	.phy_func.phy_read      = snps_phy_read,
	.phy_func.phy_write     = snps_phy_write,
	.phy_func.phy_dump      = snps_phy_dump,
};

static struct dw_hdmi_phy_plat *hdmiphy_plat[] = {
	&sun8iw20p1_phy, &sun50iw9p1_phy, &sun55iw3p1_phy, &sun60iw2p1_phy
};

u32 dw_read(u32 addr)
{
	return (u32)readb((volatile void __iomem *)(hdmi->ctrl_addr + addr));
}

void dw_write(u32 addr, u32 data)
{
	writeb((u8)data, (volatile void __iomem *)(hdmi->ctrl_addr + addr));
}

u32 dw_read_mask(u32 addr, u8 mask)
{
	return (dw_read(addr) & mask) >> ((u8)ffs(mask) - 1);
}

void dw_write_mask(u32 addr, u8 mask, u8 data)
{
	u8 temp = dw_read(addr);

	temp &= ~(mask);
	temp |= (mask & (data << ((u8)ffs(mask) - 1)));
	dw_write(addr, temp);
}

int dw_hdmi_wait_event_timeout(cond_func func, u8 times)
{
	while (times) {
		if (func() != 0x0)
			return 0x1;
		mdelay(1);
		times--;
	}
	return 0x0;
}

int dw_hdmi_ctrl_reset(void)
{
	hdmi->hdmi_on = 1;
	hdmi->tmds_clk = 0;
	hdmi->pixel_clk = 0;
	hdmi->color_bits = 8;
	hdmi->pixel_repeat = 0;
	hdmi->audio_on = 1;

	return 0;
}

int dw_hdmi_ctrl_update(void)
{
	struct dw_video_s *video  = &hdmi->video_dev;
	u32 pixel_clk = 0;

	hdmi->hdmi_on      = (video->mHdmi == DW_TMDS_MODE_HDMI) ? 1 : 0;
	hdmi->audio_on     = (video->mHdmi == DW_TMDS_MODE_HDMI) ? 1 : 0;
	hdmi->pixel_clk    = dw_video_get_pixel_clk();
	hdmi->color_bits   = video->mColorResolution;
	hdmi->pixel_repeat = video->mDtd.mPixelRepetitionInput;

	if (video->mEncodingIn == DW_COLOR_FORMAT_YCC420)
		hdmi->pixel_clk /= 2;

	pixel_clk = hdmi->pixel_clk * (hdmi->pixel_repeat + 1);

	if (video->mEncodingIn == DW_COLOR_FORMAT_YCC422) {
		hdmi->color_bits  = 8;
		hdmi->tmds_clk = pixel_clk;
		return 0;
	}

	switch (video->mColorResolution) {
	case DW_COLOR_DEPTH_10:
		hdmi->tmds_clk = pixel_clk * 125 / 100;
		break;
	case DW_COLOR_DEPTH_12:
		hdmi->tmds_clk = pixel_clk * 3 / 2;
		break;
	default:
		hdmi->tmds_clk = pixel_clk;
		break;
	}

	return 0;
}

int dw_hdmi_scdc_write(u8 addr, u8 data)
{
	int ret = 0;
	u8 buf[2] = {addr, data};
	struct i2c_msg msgs[1];

	msgs[0].addr = SCDC_I2C_SLAVE_ADDRESS,
	msgs[0].flags = 0,
	msgs[0].len = 2,
	msgs[0].buf = buf,

	ret = dw_i2cm_xfer(msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		hdmi_err("dw i2cm set scdc write failed\n");
		return -1;
	}

	return 0;
}

int dw_hdmi_scdc_read(u8 addr, u8 *data)
{
	int ret = 0;
	struct i2c_msg msgs[2];

	msgs[0].addr = SCDC_I2C_SLAVE_ADDRESS,
	msgs[0].flags = 0,
	msgs[0].len = 1,
	msgs[0].buf = &addr,

	msgs[1].addr = SCDC_I2C_SLAVE_ADDRESS,
	msgs[1].flags = I2C_M_RD,
	msgs[1].len = 0x1,
	msgs[1].buf = data,

	ret = dw_i2cm_xfer(msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		hdmi_err("dw i2cm set scdc read failed\n");
		return -1;
	}

	return 0;
}

int dw_hdmi_scdc_set_scramble(u8 setup)
{
	int ret = 0;
	u8 data = 0;

	if (setup)
		data = SCDC_SCRAMBLING_ENABLE | SCDC_TMDS_BIT_CLOCK_RATIO_BY_40;

	ret = dw_hdmi_scdc_write(SCDC_TMDS_CONFIG, data);
	if (ret != 0) {
		hdmi_err("dw scdc write scramble %s failed\n", setup ? "enable" : "disable");
		return -1;
	}

	return 0;
}

int dw_hdmi_check(void)
{
	int ret = 0;

	ret = dw_avp_check();
	return ret = 0;
}

int dw_hdmi_off(void)
{
	dw_avp_set_mute(0x1);

	if (!dw_phy_get_hpd())
		goto exit;

	if (dw_fc_video_get_scramble() && dw_sink_support_scdc())
		dw_hdmi_scdc_set_scramble(0);

exit:
	dw_phy_standby();
	dw_mc_clk_all_disable();
	dw_hdmi_ctrl_reset();
	return 0;
}

int dw_hdmi_on(void)
{
	int ret = 0;

	dw_avp_set_mute(0x1);

	if (hdmi->funcs.phy_funcs->phy_disconfig) {
		ret = hdmi->funcs.phy_funcs->phy_disconfig();
		if (ret != 0) {
			hdmi_err("sunxi hdmi phy disconfig failed\n");
			goto exit;
		}
	}

	ret = dw_avp_config();
	if (ret != 0) {
		hdmi_err("sunxi hdmi avp config failed\n");
		goto exit;
	}

	if (hdmi->phy_top) {
		ret = top_phy_config();
		if (ret != 0) {
			hdmi_err("sunxi hdmi top phy config failed\n");
			goto exit;
		}
	}

	if (hdmi->funcs.phy_funcs->phy_config) {
		ret = hdmi->funcs.phy_funcs->phy_config();
		if (ret != 0) {
			hdmi_err("sunxi hdmi ext phy config failed\n");
			goto exit;
		}
	}

	mdelay(50);
	dw_avp_set_mute(0x0);
exit:
	hdmi_trace("%s ret = %d\n", __func__, ret);
	return ret;
}

int dw_hdmi_smooth(void)
{
	int ret = 0;

	ret = dw_hdmi_ctrl_update();
	if (ret != 0) {
		hdmi_err("sunxi hdmi update control param failed\n");
		return -1;
	}

	ret = dw_infoframe_packet();
	if (ret != 0) {
		hdmi_err("sunxi hdmi smooth config infoframe failed\n");
		return -1;
	}
	return 0;
}

int dw_hdmi_cb_init(void)
{
	struct shdmi_funcs_s *funcs = &hdmi->funcs;
	int i = 0;

	funcs->ctrl_read  = dw_read;
	funcs->ctrl_write = dw_write;
	funcs->scdc_write = dw_hdmi_scdc_write;
	funcs->scdc_read  = dw_hdmi_scdc_read;

	/**************************************************
	 * @desc: register i2cm callback functions
	 *************************************************/
	funcs->i2cm_xfer = dw_i2cm_xfer;

	/**************************************************
	 * @desc: register hdmi control callback functions
	 *************************************************/
	funcs->get_hpd        = dw_phy_get_hpd;
	funcs->hdmi_disconfig = dw_hdmi_off;
	funcs->hdmi_config    = dw_hdmi_on;
	funcs->hdmi_smooth    = dw_hdmi_smooth;
	funcs->hdmi_check     = dw_hdmi_check;
	funcs->hdmi_exit      = dw_hdmi_exit;
	funcs->hdmi_tx_dump   = dw_hdmi_dump;

	/**************************************************
	 * @desc: register video callback functions
	 *************************************************/
	funcs->video_set_dtd    = dw_video_filling_timing;
	funcs->video_set_info   = dw_video_set_info;
	funcs->video_set_packet = dw_video_set_packet;
	funcs->video_set_pattern = dw_fc_video_force_value;

	/**************************************************
	 * @desc: register audio callback functions
	 *************************************************/
	funcs->audio_on  = dw_audio_on;
	funcs->audio_set = dw_audio_set_info;

	/**************************************************
	 * @desc: register phy callback functions
	 *************************************************/
	for (i = 0; i < ARRAY_SIZE(hdmiphy_plat); i++) {
		if (hdmi->plat_id == hdmiphy_plat[i]->plat_id) {
			funcs->phy_funcs = &hdmiphy_plat[i]->phy_func;
		}
	}
	if (IS_ERR_OR_NULL(funcs->phy_funcs)) {
		hdmi_err("dw hdmi register phy funcs failed!\n");
		return -1;
	}

	return 0;
}

int dw_hdmi_init(void)
{
	int ret = 0;

	ret = dw_hdmi_cb_init();
	if (ret != 0)
		return -ret;

	dw_edid_init();
	dw_audio_init();
	dw_phy_init();
	dw_i2cm_init();

	if (hdmi->phy_top)
		ret = top_phy_init();

	return ret;
}

struct dw_hdmi_s *dw_hdmi_create(struct udevice *dev)
{
	hdmi = devm_kzalloc(dev, sizeof(struct dw_hdmi_s), GFP_KERNEL);
	if (IS_ERR_OR_NULL(hdmi))
		goto exit;

	hdmi->dev = dev;

	hdmi->sink_info = kzalloc(sizeof(struct shdmi_sink_s), GFP_KERNEL);
	if (IS_ERR_OR_NULL(hdmi->sink_info)) {
		shdmi_err(hdmi->sink_info);
		goto exit;
	}

	return hdmi;
exit:
	hdmi_err("dw hdmi creat failed!\n");
	return NULL;
}

int dw_hdmi_exit(void)
{
	shdmi_free_point(hdmi->sink_info);
	shdmi_free_point(hdmi);
	return 0;
}

ssize_t dw_hdmi_dump(char *buf)
{
	ssize_t n = 0;
	struct dw_i2cm_s *i2cm = &hdmi->i2cm_dev;

	n += sprintf(buf + n, "\n[dw ctrl]\n");
	n += sprintf(buf + n, "|       |              control params                 |                  clock domain                    |              ddc            |\n");
	n += sprintf(buf + n, "|  name |---------------------------------------------+--------------------------------------------------+-----------------------------|\n");
	n += sprintf(buf + n, "|       | pixel clk |  tmds clk | repeat | color bits | pixel | tmds | repeat | audio | csc | cec | hdcp |   mode   | ref clk |  rate  |\n");
	n += sprintf(buf + n, "|-------+-----------+-----------+--------+------------+-------+------+--------+-------+-----+-----+------+----------+---------+--------|\n");
	n += sprintf(buf + n, "| state |  %-6d   |  %-6d   |   %-2d   |     %-2d     |  %-4s | %-4s |  %-4s  |  %-4s | %-4s| %-4s| %-4s | %-8s |  %-2dMHz  | %-2d.%-1dK  |\n",
		hdmi->pixel_clk, hdmi->tmds_clk, hdmi->pixel_repeat, hdmi->color_bits,
		dw_mc_get_clk(DW_MC_CLK_PIXEL) ? "on" : "off",
		dw_mc_get_clk(DW_MC_CLK_TMDS)  ? "on" : "off",
		dw_mc_get_clk(DW_MC_CLK_PREP)  ? "on" : "off",
		dw_mc_get_clk(DW_MC_CLK_AUDIO) ? "on" : "off",
		dw_mc_get_clk(DW_MC_CLK_CSC)   ? "on" : "off",
		dw_mc_get_clk(DW_MC_CLK_CEC)   ? "on" : "off",
		dw_mc_get_clk(DW_MC_CLK_HDCP)  ? "on" : "off",
		i2cm->mode ? "fast" : "standard", i2cm->sfrClock / 100,
		dw_i2cm_get_rate() / 10, dw_i2cm_get_rate() % 10);
	printf("%s\n", buf);

	memset(buf, 0x0, ARRAY_SIZE(buf));
	n = 0;
	n += dw_avp_dump(buf + n);
	printf("%s\n", buf);

	memset(buf, 0x0, ARRAY_SIZE(buf));
	n = 0;
	n += dw_phy_dump(buf + n);
	printf("%s\n", buf);

	memset(buf, 0x0, ARRAY_SIZE(buf));
	n = 0;
	if (hdmi->funcs.phy_funcs->phy_dump)
		n += hdmi->funcs.phy_funcs->phy_dump(buf + n);

	if (hdmi->phy_top)
		n += top_phy_dump(buf + n);
	printf("%s\n", buf);

	memset(buf, 0x0, ARRAY_SIZE(buf));
	n = 0;
	return n;
}

struct dw_hdmi_s *dw_get_hdmi(void)
{
	return hdmi;
}
