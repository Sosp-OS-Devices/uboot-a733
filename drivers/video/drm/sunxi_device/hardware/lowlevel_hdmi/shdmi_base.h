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

#ifndef _HDMI_BASE_H_
#define _HDMI_BASE_H_

#include <linux/types.h>
#include <linux/bitfield.h>
#include <drm/drm_print.h>
#include <drm/drm_edid.h>
#include <drm/drm_modes.h>
#include <drv_hdmi.h>
#include <i2c.h>

#include "../include.h"

extern u8 shdmi_log_leve;

#define shdmi_dw2byte(data, index)   ((u8)((data) >> ((index) * 8)))

#define shdmi_bit_filed(data, shift, width) \
	(((data) >> (shift)) & ((1 << (width)) - 1))

#define shdmi_concat_bits(hi_byte, hi_shift, hi_width, lo_byte, lo_shift, lo_width) \
	(shdmi_bit_filed(hi_byte, hi_shift, hi_width) << (lo_width) |              \
	shdmi_bit_filed(lo_byte, lo_shift, lo_width))

#define shdmi_byte_to_dword(b3, b2, b1, b0) \
	((u32)((b3 << 24) | (b2 << 16) | (b1 << 8) | b0))

#define shdmi_byte_to_word(hi, lo) \
	((__force u16)__cpu_to_be16((hi) << 8 | (lo)))

#define _hdmivic_to_ceavic(code) \
	((code) == 1 ? 95 : (code) == 2 ? 94 : \
	(code) == 3 ? 93 : (code) == 4 ? 98 : -1)

#define _ceavic_to_hdmivic(code) \
	((code) == 95 ? 1 : (code) == 94 ? 2 : \
	(code) == 93 ? 3 : (code) == 98 ? 4 : -1)

#define _shdmi_use_hfvsif(vic) \
	(((vic) == HDMI_VIC_3840x2160P50) || ((vic) == HDMI_VIC_3840x2160P60) || \
	((vic) == HDMI_VIC_4096x2160P50) || ((vic) == HDMI_VIC_4096x2160P60))

#define HDMI_VIC_3D_OFFSET    (0x80)
enum HDMI_CEA_VIC {
	HDMI_VIC_NULL       = 0,
	HDMI_VIC_640x480P60 = 1,
	HDMI_VIC_720x480P60_4_3,
	HDMI_VIC_720x480P60_16_9,
	HDMI_VIC_1280x720P60,
	HDMI_VIC_1920x1080I60,
	HDMI_VIC_720x480I_4_3,
	HDMI_VIC_720x480I_16_9,
	HDMI_VIC_720x240P_4_3,
	HDMI_VIC_720x240P_16_9,
	HDMI_VIC_1920x1080P60 = 16,
	HDMI_VIC_720x576P_4_3,
	HDMI_VIC_720x576P_16_9,
	HDMI_VIC_1280x720P50,
	HDMI_VIC_1920x1080I50,
	HDMI_VIC_720x576I_4_3,
	HDMI_VIC_720x576I_16_9,
	HDMI_VIC_1920x1080P50 = 31,
	HDMI_VIC_1920x1080P24,
	HDMI_VIC_1920x1080P25,
	HDMI_VIC_1920x1080P30,
	HDMI_VIC_1280x720P24 = 60,
	HDMI_VIC_1280x720P25,
	HDMI_VIC_1280x720P30,
	HDMI_VIC_3840x2160P24 = 93,
	HDMI_VIC_3840x2160P25,
	HDMI_VIC_3840x2160P30,
	HDMI_VIC_3840x2160P50,
	HDMI_VIC_3840x2160P60,
	HDMI_VIC_4096x2160P24,
	HDMI_VIC_4096x2160P25,
	HDMI_VIC_4096x2160P30,
	HDMI_VIC_4096x2160P50,
	HDMI_VIC_4096x2160P60,
	HDMI_VIC_2560x1440P60 = 0x201,
	HDMI_VIC_1440x2560P70 = 0x202,
	HDMI_VIC_1080x1920P60 = 0x203,
	HDMI_VIC_1680x1050P60 = 0x204,
	HDMI_VIC_1600x900P60  = 0x205,
	HDMI_VIC_1280x1024P60 = 0x206,
	HDMI_VIC_1024x768P60  = 0x207,
	HDMI_VIC_800x600P60   = 0x208,
	HDMI_VIC_MAX,
};

/***/
#define BLOCK_BASE         0x00
#define BLOCK_EXT_CEA      0x02
#define BLOCK_EXT_VTB      0x10
#define BLOCK_EXT_DI       0x40
#define BLOCK_EXT_LS       0x50
#define BLOCK_EXT_MI       0x60

#define SEDID_DTD_SIZE     0x12
#define SEDID_BLOCK_SIZE       (128)
/* DDC Command Interface Addr */
#define SHDMI_DDC_CI_ADDR      (0x37)
/* DDC Address */
#define SHDMI_DDC_ADDR         (0x50)
/* DDC Segment Address */
#define SHDMI_DDC_SEGMENT_ADDR (0x30)

typedef struct disp_device_config       disp_cfg_t;

enum shdmi_video_fmt_e {
	eSHDMI_VIDEO_FMT_NONE = 0,
	eSHDMI_VIDEO_FMT_EXT  = 1,
	eSHDMI_VIDEO_FMT_3D   = 2,
	eSHDMI_VIDEO_FMT_MAX,
};

enum sunxi_hdmi_plat_id_e {
	eHDMI_SUN8IW20P1 = 1,
	eHDMI_SUN50IW9P1,
	eHDMI_SUN55IW3P1,
	eHDMI_SUN55IW7P1,
	eHDMI_SUN60IW2P1,
};

enum sunxi_hdmi_link_mode_e {
	eSHDMI_LINK_MODE_NULL = -1,
	eSHDMI_LINK_MODE_TMDS = 0,
	eSHDMI_LINK_MODE_FRL_3G3L,
	eSHDMI_LINK_MODE_FRL_6G3L,
	eSHDMI_LINK_MODE_FRL_6G4L,
	eSHDMI_LINK_MODE_FRL_8G4L,
	eSHDMI_LINK_MODE_FRL_10G4L,
	eSHDMI_LINK_MODE_FRL_12G4L,
	eSHDMI_LINK_MODE_MAX,
};

enum sunxi_hdmi_pixel_rep_e {
	eSHDMI_PIXEL_REP0 = 0,
	eSHDMI_PIXEL_REP1,
	eSHDMI_PIXEL_REP2,
	eSHDMI_PIXEL_REP3,
	eSHDMI_PIXEL_REP4,
	eSHDMI_PIXEL_REP5,
	eSHDMI_PIXEL_REP6,
	eSHDMI_PIXEL_REP7,
	eSHDMI_PIXEL_REP8,
	eSHDMI_PIXEL_REP9,
	eSHDMI_PIXEL_REP10,
	eSHDMI_PIXEL_REP11,
	eSHDMI_PIXEL_REP12,
};

enum sunxi_hdmi_color_bit_e {
	eSHDMI_COLOR_BIT_8  = 8,
	eSHDMI_COLOR_BIT_10 = 10,
	eSHDMI_COLOR_BIT_12 = 12,
	eSHDMI_COLOR_BIT_16 = 16,
};

enum sunxi_hdmi_color_fmt_e {
	eSHDMI_COLOR_FMT_RGB = 0,
	eSHDMI_COLOR_FMT_YUV444,
	eSHDMI_COLOR_FMT_YUV422,
	eSHDMI_COLOR_FMT_YUV420,
};

enum sunxi_hdcp_state_e {
	SUNXI_HDCP_DISABLE = 0,
	SUNXI_HDCP_ING     = 1,
	SUNXI_HDCP_FAILED  = 2,
	SUNXI_HDCP_SUCCESS = 3,
};

enum shdcp_path_e {
	SHDCP1X_PATH = 0,
	SHDCP2X_PATH = 1,
};

enum sunxi_cec_wait_bit_times_e {
	SUNXI_CEC_WAIT_3BIT	= 0,
	SUNXI_CEC_WAIT_5BIT,
	SUNXI_CEC_WAIT_7BIT,
	SUNXI_CEC_WAIT_NULL,
};

enum sunxi_cec_irq_state_e {
	SUNXI_CEC_IRQ_NULL         = 0x0,
	SUNXI_CEC_IRQ_DONE,
	SUNXI_CEC_IRQ_EOM,
	SUNXI_CEC_IRQ_NACK,
	SUNXI_CEC_IRQ_ARB,
	SUNXI_CEC_IRQ_ERR_INITIATOR,
	SUNXI_CEC_IRQ_ERR_FOLLOW,
	SUNXI_CEC_IRQ_WAKEUP,
};

enum shdmi_audio_interface {
	eSHDMI_AUDIO_IF_I2S   = 0,
	eSHDMI_AUDIO_IF_SPDIF = 1,
	eSHDMI_AUDIO_IF_HBR   = 2,
	eSHDMI_AUDIO_IF_GPA   = 3,
	eSHDMI_AUDIO_IF_DMA   = 4,
	eSHDMI_AUDIO_IF_MAX
};

enum shdmi_audio_coding {
	eSHDMI_AUDIO_CODING_NULL               = -1,
	eSHDMI_AUDIO_CODING_PCM                = 1,
	eSHDMI_AUDIO_CODING_AC3                = 2,
	eSHDMI_AUDIO_CODING_MPEG1              = 3,
	eSHDMI_AUDIO_CODING_MP3                = 4,
	eSHDMI_AUDIO_CODING_MPEG2              = 5,
	eSHDMI_AUDIO_CODING_AAC                = 6,
	eSHDMI_AUDIO_CODING_DTS                = 7,
	eSHDMI_AUDIO_CODING_ATRAC              = 8,
	eSHDMI_AUDIO_CODING_ONE_BIT_AUDIO      = 9,
	eSHDMI_AUDIO_CODING_DOLBY_DIGITAL_PLUS = 10,
	eSHDMI_AUDIO_CODING_DTS_HD             = 11,
	eSHDMI_AUDIO_CODING_MAT                = 12,
	eSHDMI_AUDIO_CODING_DST                = 13,
	eSHDMI_AUDIO_CODING_WMAPRO             = 14
};

struct shdmi_audio_info {
	enum shdmi_audio_interface   intf;
	enum shdmi_audio_coding      coding;
	u32 sample_freq;
	u32 sample_size;
	u32 channel_alloc;
	u32 channel_num;
};

/**
 * @file
 * For detailed handling of this structure,
 * refer to documentation of the functions
 */
struct shdmi_dtd_s {
	/* vic code */
	u32 mCode;
	u8 mLimitedToYcc420;
	u8 mYcc420;
	u16 mPixelRepetitionInput;
	u32 mPixelClock; /* pixel clock, unit: KHz */
	u8 mInterlaced; /* 1: interlaced, 0: progressive */

	u16 mHActive;
	u16 mHBlanking;
	u16 mHBorder;
	u16 mHImageSize; /* For picture aspect ratio */
	u16 mHSyncOffset;
	u16 mHSyncPulseWidth;
	u8 mHSyncPolarity; /* 0: active low; 1: active hight */

	u16 mVActive;
	u16 mVBlanking;
	u16 mVBorder;
	u16 mVImageSize; /* For picture aspect ratio */
	u16 mVSyncOffset;
	u16 mVSyncPulseWidth;
	u8 mVSyncPolarity; /* 0: active low; 1: active hight */
};

struct sink_hdmiforum_s {
	unsigned char mValid;
	unsigned char mVersion;
	unsigned char mMaxTmdsCharRate;
	unsigned char m3D_OSD_Disparity;
	unsigned char mMaxFRLRate;
	unsigned char mDualView;
	unsigned char mIndependentView;
	unsigned char mLTS_340Mcs_scramble;
	unsigned char mRR_Capable;
	unsigned char mSCDC_Present;
	unsigned char mDC_30bit_420;
	unsigned char mDC_36bit_420;
	unsigned char mDC_48bit_420;

	/* Enabled Addintional Fields */
	unsigned char Mdelta;
	unsigned char CinemaVRR;
	unsigned char CNMVRR;
	unsigned char FVA;
	unsigned char ALLM;
	unsigned char FAPA;

	/* Enabled VRR Fileds */
	unsigned char VRRmin;
	unsigned char VRRmax;
};

/* HDMI 2.X HF_EEODB */
struct sink_hfeeodb_s {
	unsigned char mValid;
	unsigned char mExtBlkCnt;
};

/** For detailed handling of this structure, refer to documentation of the functions */
struct sink_hdmivsdb_s {
	u16 mPhysicalAddress;
	int mSupportsAi;
	int mDeepColor30;
	int mDeepColor36;
	int mDeepColor48;
	int mDeepColorY444;
	int mDviDual;
	u16 mMaxTmdsClk;
	u16 mVideoLatency;
	u16 mAudioLatency;
	u16 mInterlacedVideoLatency;
	u16 mInterlacedAudioLatency;
	unsigned int mId;
	unsigned char mContentTypeSupport;
	unsigned char mImageSize;
	int mHdmiVicCount;
	unsigned char mHdmiVic[16];
	int m3dPresent;
	int hdmi3dCount; //number of 3d entries
	int hdmi3dfirst; //number of 3d entries
	int hdmi3dfirststrc; //number of 3d entries
	int mVideo3dStruct[16][16];	/* row index is the VIC number */
	int mDetail3d[16][16];	/* index is the VIC number */
	int mValid;
};

/**
 * @file
 * Speaker Allocation Data Block.
 * Holds the Speaker Allocation data block information.
 */
struct sink_sadb_s {
	unsigned char mByte1;
	unsigned char mByte2;
	unsigned char mByte3;
	int mValid;
};

/**
 * @file
 * Video Capability Data Block.
 * (sink_vcdb_s * vcdbCEA Data Block Tag Code 0).
 * Parse and hold information from EDID data structure.
 * For detailed handling of this structure, refer to documentation of the functions
 */
struct sink_vcdb_s {
	int mQuantizationRangeSelectable;
	unsigned char mPreferredTimingScanInfo;
	unsigned char mItScanInfo;
	unsigned char mCeScanInfo;
	int mValid;
};

/**
 * @file
 * Room Configuration Data Block.
 * Holds the Room Configuration data block information.
 */
struct sink_rcdb_s {
	unsigned char mSpeakerCount;
	unsigned char mSLD;
	unsigned char mSpeaker;
	unsigned char mDisplay;
	unsigned char mSPM1;
	unsigned char mSPM2;
	unsigned char mSPM3;
	int mValid;
};

struct hdr_static_mdb {
	u8 et_n;
	u8 sm_n;
	/* Desired Content Max Luminance data */
	u8 dc_max_lum_data;
	/* Desired Content Max Frame-average Luminance data */
	u8 dc_max_fa_lum_data;
	/* Desired Content Min Luminance data */
	u8 dc_min_lum_data;
};

/**
 * @file
 * Colorimetry Data Block class.
 * Holds and parses the Colorimetry data-block information.
 */
struct sink_cmdb_s {
	unsigned char mByte3;
	unsigned char mByte4;
	int mValid;
};

/**
 * @file
 * Short Video Descriptor.
 * Parse and hold Short Video Descriptors found in Video Data Block in EDID.
 */
/** For detailed handling of this structure, refer to documentation of the functions */
struct sink_svd_s {
	int mNative;
	unsigned mCode;
	unsigned mLimitedToYcc420;
	unsigned mYcc420;
};

/**
 * @file
 * Short Audio Descriptor.
 * Found in Audio Data Block (sink_sad_s *sad, CEA Data Block Tage Code 1).
 * Parse and hold information from EDID data structure
 */
/** For detailed handling of this structure, refer to documentation of the functions */
struct sink_sad_s {
	unsigned char mFormat;
	unsigned char mMaxChannels;
	unsigned char mSampleRates;
	unsigned char mByte3;
};

/**
 * @file
 * Second Monitor Descriptor
 * Parse and hold Monitor Range Limits information read from EDID
 */
struct sink_range_limit_s {
	unsigned char mMinVerticalRate;
	unsigned char mMaxVerticalRate;
	unsigned char mMinHorizontalRate;
	unsigned char mMaxHorizontalRate;
	unsigned char mMaxPixelClock;
	int mValid;
};

struct shdmi_sink_s {
	u8 mfg_week;
	u8 mfg_year;
	char sink_name[30];
	u8 extern_flag;

	bool sink_ver; /* 0: hdmi14, 1:hdmi20, 2:hdmi21*/

	struct shdmi_dtd_s dtd_table[16];
	unsigned int dtd_index;

	struct sink_svd_s svd_table[128];
	unsigned int svd_index;

	unsigned char prefer_table[30];
	unsigned int prefer_index;

	struct sink_sad_s sad_table[128];
	unsigned int sad_index;


	/* number of native dtd */
	unsigned int native_dtd_num;
	unsigned int support_ycc444;
	unsigned int support_ycc422;
	unsigned int support_ycc420;
	unsigned int support_ycc420_all;
	unsigned int support_audio;
	unsigned int support_underscan;

	struct sink_hdmivsdb_s    HdmiVsdb;
	struct sink_hdmiforum_s   HdmiForum;
	struct sink_hfeeodb_s     HdmiForumEEO;
	struct sink_range_limit_s RangeLimits;
	struct sink_vcdb_s        vcdb;
	struct sink_cmdb_s        cmdb;
	struct sink_sadb_s        sadb;
	struct sink_rcdb_s        rcdb;
	struct hdr_static_mdb     hdr_sdmb;
};

/*******************************************************************************
 *
 * @desc: hardware callback for sunxi hdmi level
 *
 ******************************************************************************/
struct shdmi_phy_funcs {
	int (*phy_init)(void);
	int (*phy_resume)(void);
	int (*phy_reset)(void);
	int (*phy_disconfig)(void);
	int (*phy_config)(void);
	int (*phy_read)(u8 addr, void *data);
	int (*phy_write)(u8 addr, void *data);
	ssize_t (*phy_dump)(char *buf);

	int (*phy_check_config)(void);
	void (*phy_set_ffe_mode)(u16 mode);
	void (*phy_set_hpd_rxsense)(unsigned char en);
	unsigned char (*phy_get_hpd_rxsense)(void);
	void (*phy_set_hpd_protect)(unsigned char en);
	unsigned char (*phy_get_hpd_protect)(void);
	unsigned char (*phy_get_hpd_status)(void);
};

struct shdmi_funcs_s {
	/* write or read funcs */
	void (*ctrl_write)(u32 addr, u32 data);
	u32(*ctrl_read)(u32 addr);

	int (*scdc_read)(u8 addr, u8 *data);
	int (*scdc_write)(u8 addr, u8 data);

	u8 (*get_hpd)(void);
	u8 (*get_rxsense)(void);

	/***************************************************************************
	 * @desc: i2cm callback functions
	 * @func: i2cm_set_rate: for set i2cm mode and rate;
	 *     @mode:
	 **************************************************************************/
	int (*i2cm_set_rate)(u32 mode, u32 rate);
	int (*i2cm_xfer)(struct i2c_msg *msgs, int num);
	int (*i2cm_adap_get)(struct i2c_adapter *ddc);

	/***************************************************************************
	 * @desc: hdcp callback functions
	 **************************************************************************/
	int (*hdcp1x_on)(void);
	int (*hdcp1x_off)(void);
	int (*hdcp2x_on)(void);
	int (*hdcp2x_off)(void);
	int (*hdcp2x_fw_load)(const u8 *fw, size_t size);
	int (*hdcp2x_fw_state)(void);
	int (*hdcp_get_state)(void);
	int (*hdcp_set_path)(enum shdcp_path_e path);

	/***************************************************************************
	 * @desc: cec callback functions
	 **************************************************************************/
	void (*cec_set_la)(u16 addr);
	int (*cec_on)(void);
	int (*cec_off)(void);
	int (*cec_receive)(u8 *msg, u8 size);
	int (*cec_send)(u8 *msg, u8 len, u8 type);
	u8 (*cec_get_irq)(void);

	/***************************************************************************
	 * @desc: audio callback functions
	 **************************************************************************/
	int (*audio_set)(struct shdmi_audio_info info);
	int (*audio_on)(void);
	int (*audio_off)(void);

	/***************************************************************************
	 * @desc: video callback functions
	 **************************************************************************/
	int (*video_set_dtd)(struct shdmi_dtd_s *dtd, u32 rate);
	int (*video_set_info)(disp_cfg_t *info);
	int (*video_set_packet)(u8 is_3d);
	void (*video_set_pattern)(u8 en, u32 value);

	/***************************************************************************
	 * @desc: hdmi base callback functions
	 **************************************************************************/
	int (*hdmi_config)(void);
	int (*hdmi_disconfig)(void);
	int (*hdmi_smooth)(void);
	int (*hdmi_check)(void);
	int (*hdmi_exit)(void);
	ssize_t (*hdmi_tx_dump)(char *buf);
	ssize_t (*hdmi_rx_dump)(char *buf);

	struct shdmi_phy_funcs   *phy_funcs;
};

/*****************************************************
 * sunxi hdmi print macro
 * @desc: support uart print and hdmi log buffer
 *        support keywords sunxi-hdmi
 *****************************************************/
#ifdef hdmi_inf
#undef hdmi_inf
#endif
#define hdmi_inf(fmt, args...)                    \
	do {                                          \
		DRM_INFO("sunxi-hdmi: "fmt, ##args);      \
	} while (0)

#ifdef hdmi_wrn
#undef hdmi_wrn
#endif
#define hdmi_wrn(fmt, args...)                      \
	do {                                            \
		DRM_INFO("sunxi-hdmi: [warn] "fmt, ##args); \
	} while (0)

#ifdef hdmi_err
#undef hdmi_err
#endif
#define hdmi_err(fmt, args...)                       \
	do {                                             \
		DRM_INFO("sunxi-hdmi: [error] "fmt, ##args); \
	} while (0)

#ifdef hdcp_log
#undef hdcp_log
#endif
#define hdcp_log(fmt, args...)                          \
	do {                                                \
		if (shdmi_log_leve)  \
			DRM_INFO("sunxi-hdmi: [hdcp] "fmt, ##args); \
	} while (0)

#ifdef hdmi_trace
#undef hdmi_trace
#endif
#define hdmi_trace(fmt, args...)                        \
	do {                                                \
		if (shdmi_log_leve) \
			DRM_INFO("sunxi-hdmi: "fmt, ##args);        \
	} while (0)

#ifdef shdmi_err
#undef shdmi_err
#endif
#define shdmi_err(name)                                     \
	do {                                                    \
		hdmi_err("check point %s is err or null\n", #name); \
		dump_stack();                                       \
	} while (0)

#ifdef shdmi_free_point
#undef shdmi_free_point
#endif
#define shdmi_free_point(x)   \
	do {                      \
		if (x != NULL) {      \
			kfree(x);         \
			x = NULL;         \
		}                     \
	} while (0)

#endif /* _HDMI_BASE_H_ */
