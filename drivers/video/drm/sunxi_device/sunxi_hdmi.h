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

#ifndef _SUNXI_HDMI_H_
#define _SUNXI_HDMI_H_

#include <drm/drm_modes.h>
#include <drv_hdmi.h>

#include "include.h"
#include "lowlevel_hdmi/include.h"

#define IS_HDMI20 IS_ENABLED(CONFIG_AW_DRM_HDMI20)
#define IS_HDMI21 IS_ENABLED(CONFIG_AW_DRM_HDMI21)

#define SUNXI_HDMI_ENABLE   			(0x1)
#define SUNXI_HDMI_DISABLE  			(0x0)

#define SUNXI_HDMI20	BIT(0)
#define SUNXI_HDMI21	BIT(1)

enum shdmi_dynamic_range_e {
	SHDMI_SDR    = 0,
	SHDMI_HDR10  = 1,
	SHDMI_HDR10P = 2,
	SHDMI_HLG    = 3,
	SHDMI_DV     = 4,
	SHDMI_MAX_DR,
};

enum sunxi_hdmi_update_bits {
	SUNXI_HDMI_UPDATE_FAORMAT = (0),
	SUNXI_HDMI_UPDATE_BITS    = (1),
	SUNXI_HDMI_UPDATE_EOTF    = (2),
	SUNXI_HDMI_UPDATE_SPACE   = (3),
	SUNXI_HDMI_UPDATE_DVIHDMI = (4),
	SUNXI_HDMI_UPDATE_RANGE   = (5),
	SUNXI_HDMI_UPDATE_SCAN    = (6),
	SUNXI_HDMI_UPDATE_RATIO   = (7),
};

enum sunxi_hdmi_color_capability {
	SHDMI_RGB888_8BITS    = 0,
	SHDMI_YUV444_8BITS    = 1,
	SHDMI_YUV422_8BITS    = 2,
	SHDMI_YUV420_8BITS    = 3,
	SHDMI_RGB888_10BITS   = 4,
	SHDMI_YUV444_10BITS   = 5,
	SHDMI_YUV422_10BITS   = 6,
	SHDMI_YUV420_10BITS   = 7,
	SHDMI_RGB888_12BITS   = 8,
	SHDMI_YUV444_12BITS   = 9,
	SHDMI_YUV422_12BITS   = 10,
	SHDMI_YUV420_12BITS   = 11,
	SHDMI_RGB888_16BITS   = 12,
	SHDMI_YUV444_16BITS   = 13,
	SHDMI_YUV422_16BITS   = 14,
	SHDMI_YUV420_16BITS   = 15,
	SHDMI_MAX_MASK,
};

/******************************************************************
 * @desc: sunxi hdmi level struct
 *****************************************************************/
struct sunxi_hdmi_rescal_s {
	unsigned long rescal_ctrl_pa;
	u32 bit_hdmi_res_sel;
	u32 bit_rescal_mode;
	u32 bit_cal_ana_en;
	u32 bit_cal_en;

	unsigned long res0_ctrl_pa;
	u32 res0_ctrl_bitmask;
};

struct sunxi_hdmi_plat_s {
	unsigned char version;
	unsigned char use_top_phy;
	unsigned char pixel_mode;
	unsigned char max_linkmode;
	unsigned char hdmi_ver;
	unsigned char support_vrr;
	/* resistance calibration */
	unsigned char need_res_cal;
	struct sunxi_hdmi_rescal_s  rescal_regs;
};

struct sunxi_hdmi_s {
	struct udevice          *dev;

	uintptr_t reg_base;
	u8 resistor_src;
	u8 clock_src;

	struct sunxi_hdmi_plat_s    *plat_data;
	struct disp_device_config	 disp_info;

#if IS_HDMI20
	struct dw_hdmi_s       *dw_hdmi;
#endif
#if IS_HDMI21
	struct dwqp_hdmi_s     *dwqp_hdmi;
#endif
};

/**
 * @desc: sunxi hdmi get color format string
 * @bits: disp define format
 * @return: format string
 */
char *shdmi_color_fmt_string(enum disp_csc_type format);
/**
 * @desc: sunxi hdmi get color depth string
 * @bits: disp define data bits
 * @return: bits string
 */
char *shdmi_color_bits_string(enum disp_data_bits bits);
/*******************************************************************************
 * sunxi hdmi core register write and read function
 ******************************************************************************/
/**
 * @desc: sunxi hdmi contorl register write
 * @addr: write address
 * @data: write value
 */
void sunxi_hdmi_ctrl_write(uintptr_t addr, u32 data);
/**
 * @desc: sunxi hdmi contorl register read
 * @addr: read address
 * @return: read value
 */
u32 sunxi_hdmi_ctrl_read(uintptr_t addr);
/*******************************************************************************
 * sunxi hdmi core scdc write and read function
 ******************************************************************************/
/**
 * @desc: sunxi hdmi scdc register write
 * @addr: write address
 * @data: write value
 */
void sunxi_hdmi_scdc_write(u8 addr, u8 data);
/**
 * @desc: sunxi hdmi scdc register read
 * @addr: read address
 * @return: read value
 */
u8 sunxi_hdmi_scdc_read(u8 addr);
/*******************************************************************************
 * sunxi hdmi core phy function
 ******************************************************************************/
/**
 * @desc: sunxi hdmi phy register write
 * @addr: write address
 * @data: write value
 * @return: 0 - write success
 *         -1 - write failed
 */
int sunxi_hdmi_phy_write(u8 addr, u32 data);
/**
 * @desc: sunxi hdmi phy register read
 * @addr: read address
 * @data: point to read value
 * @return: 0 - read success
 *         -1 - read failed
 */
int sunxi_hdmi_phy_read(u8 addr, u32 *data);
/**
 * @desc: sunxi hdmi phy config
 * @return: 0 - config success
 *         -1 - config failed
 */
int sunxi_hdmi_phy_config(void);
/**
 * @desc: sunxi hdmi phy reset(Optional)
 */
void sunxi_hdmi_phy_reset(void);
/**
 * @desc: sunxi hdmi phy resume(Optional)
 * @return: 0 - resume success
 *         -1 - resume failed
 */
int sunxi_hdmi_phy_resume(void);
/**
 * @desc: sunxi hdmi i2c set mode and rate
 * @mode: i2c mode. standard or fast mode
 * @rate: i2c rate. (*100Hz)
 * @return: 0 - suaccess
 *         -1 - failed
 */
int sunxi_hdmi_i2cm_set_ddc(u32 mode, u32 rate);
/**
 * @desc: sunxi hdmi i2c master xfer, support send and receive
 * @msgs: buffer fot send or receive message
 * @num: send or receive message number
 */
int sunxi_hdmi_i2cm_xfer(struct i2c_msg *msgs, int num);
/**
 * @desc: sunxi hdmi edid parse
 * @buffer: point to edid buffer
 * @return: 0 - parse success
 *         -1 - parse failed
 */
int sunxi_hdmi_edid_parse(u8 *buffer);

/*******************************************************************************
 * sunxi hdmi core audio function
 ******************************************************************************/
/**
 * @desc: sunxi hdmi audio set info params
 * @snd_param: sunxi snd params
 * @return: 0 - set info success
 *         -1 - set info failed
 */
int sunxi_hdmi_audio_set_info(hdmi_audio_t *audio);
/**
 * @desc: sunxi hdmi audio enable
 * @return: 0 - enable success
 *         -1 - enable failed
 */
int sunxi_hdmi_audio_enable(void);
/*******************************************************************************
 * sunxi hdmi core video info function
 ******************************************************************************/
u32 sunxi_hdmi_get_support_hdr_mode(void);
/**
 * @desc: sunxi hdmi get useing disp info
 * @return: disp info struct
 */
struct disp_device_config *sunxi_hdmi_get_disp_info(void);
/**
 * @desc: check sink is support current eotf
 * @info: point to config
 */
int sunxi_hdmi_disp_select_eotf(struct disp_device_config *info);
/**
 * @desc: check sink is support current color space
 * @info: point to config
 * @vic_code: vic code
 */
int sunxi_hdmi_disp_select_space(struct disp_device_config *info, u32 vic_code);
/**
 * @desc: check sink is support current color format
 * @info: point to config
 * @vic_code: vic code
 */
int sunxi_hdmi_disp_select_format(struct disp_device_config *info, u32 vic_code);
/**
 * @desc: check current format,bits is sink support
 * @format: current use format
 * @bits: current use bits
 * @pixel_clk: current timing pixel clock
 */
int sunxi_hdmi_video_check_tmds_clock(u8 format, u8 bits, u32 pixel_clk);
/**
 * @desc: sunxi hdmi video set disp info
 * @disp_param: disp info
 */
int sunxi_hdmi_set_disp_info(struct disp_device_config *disp_param);
/**
 * @desc: sunxi hdmi video force patterm
 * @bit: force enable
 * @value: force value
 */
void sunxi_hdmi_video_set_pattern(u8 bit, u32 value);
/**
 * @desc: sunxi hdmi update sink color capality
 * @vic: current timing vic
 * @return: current timing support vic
 */
u32 sunxi_hdmi_get_color_capality(u32 vic);
/**
 * @desc: sunxi hdmi update send perfer video infoframe
 */
int sunxi_hdmi_select_output_packets(u8 flags);
u8 sunxi_hdmi_get_pixel_mode(void);
/**
 * @desc: sunxi hdmi get hpd state
 * @return: 1 - hight
 *          0 - low
 */
u8 sunxi_hdmi_get_hpd(void);
/**
 * @desc: sunxi hdmi get lowlevel loglevel
 * @return: log level
 */
u8 sunxi_hdmi_get_loglevel(void);
/**
 * @desc: sunxi hdmi set log level
 * @level: log level
 */
int suxni_hdmi_set_loglevel(u8 level);
/**
 * @desc: sunxi hdmi disconfig
 * @return: 0 - success
 */
int sunxi_hdmi_disconfig(void);
/**
 * @desc: sunxi hdmi check config
 * @return: 0: check done or not issue.
 */
int sunxi_hdmi_check(void);
/**
 * @desc: check vrr is support
 * check rule: rx edid support and tx platform support
 */
bool sunxi_hdmi_check_vrr(void);
/**
 * @desc: sunxi hdmi config
 * @return: 0 - success
 */
int sunxi_hdmi_config(void);
int sunxi_hdmi_smooth_config(void);
/**
 * @desc: sunxi hdmi mode set.
 * @hdmi: sunxi hdmi point
 * @mode: set mode
 * @return: 0 - success
 *         -1 - failed
 */
int sunxi_hdmi_set_disp_mode(struct drm_display_mode *mode);
/**
 * @desc: sunxi hdmi init
 * @hdmi: sunxi hdmi point
 * @return: 0 - init success
 */
int sunxi_hdmi_init(struct sunxi_hdmi_s *hdmi);
/**
 * @desc: sunxi hdmi exit
 */
void sunxi_hdmi_exit(void);
/**
 * @desc: sunxi hdmi tx info dump
 * @buf: point to save info buffer
 * @return: save buffer size
 */
ssize_t sunxi_hdmi_tx_dump(char *buf);
/**
 * @desc: sunxi hdmi rx info dump
 * @buf: point to save info buffer
 * @return: save buffer size
 */
ssize_t sunxi_hdmi_rx_dump(char *buf);

#endif /* _SUNXI_HDMI_H_ */
