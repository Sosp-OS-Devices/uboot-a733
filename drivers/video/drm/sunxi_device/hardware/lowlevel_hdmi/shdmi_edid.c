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
//#include <linux/slab.h>
#include "shdmi_base.h"
#include "shdmi_edid.h"

enum _sedid_cea_tag_e {
	/* Reserved: 0 */
	SEDID_TAG_ADB        = 1, /* Audio Data Block */
	SEDID_TAG_VDB        = 2, /* Video Data Block */
	SEDID_TAG_VSDB       = 3, /* Vendor-Specific Data Block */
	SEDID_TAG_SADB       = 4, /* Speaker Allocation Data Block */
	SEDID_TAG_VESA_DTCDB = 5, /* VESA Display Transfer Characteristic Data Block */
	/* Reserved: 6 */
	SEDID_TAG_EXT     = 7, /* Use Extended Tag */
};

/* edid data block extern-tag */
enum _sedid_cea_ext_tag_e {
	SEDID_EXT_TAG_VCDB             = 0, /* Video Capability Data Block */
	SEDID_EXT_TAG_VSVDB      = 1, /* Vendor-Specific Video Data Block */
	SEDID_EXT_TAG_VESA_DDDB = 2, /* VESA Display Device Data Block */
	SEDID_EXT_TAG_VESA_VDB                 = 3, /* Reserved: VESA Video Data Block */
	SEDID_EXT_TAG_HDMI_VDB                 = 4, /* Reserved: HDMI Video Data Block */
	SEDID_EXT_TAG_CMDB           = 5, /* Colorimetry Data Block */
	SEDID_EXT_TAG_HDR_SMDB                 = 6, /* HDR Static Metadata Data Block */
	SEDID_EXT_TAG_HDR_DMDB                 = 7, /* HDR Dynamic Metadata Data Block */
	/* reserved: 8...12 */
	EXT_TAG_VFPDB      = 13, /* Video Format Preference Data Block */
	EXT_TAG_YCbCr420VDB        = 14, /* YCbCr 4:2:0 Video Data Block */
	EXT_TAG_YCbCr420CMDB      = 15, /* YCbCr 4:2:0 Capability Map Data Block */
	EXT_TAG_Misc_AFDB      = 16, /* Reserved: CTA Miscellaneous Auido Fields */
	EXT_TAG_VSADB           = 17, /* Vendor-Specific Audio Data Block */
	EXT_TAG_HDMI_ADB            = 18, /* HDMI Audio Data Block */
	EXT_TAG_RCDB           = 19, /* Room Configuration Data Block */
	EXT_TAG_SLDB                = 20, /* Speaker Location Data Block */
	/* reserved: 21...31 */
	EXT_TAG_Infoframe_DB             = 32, /* Infoframe Data Block */
	/* reserved: 33 */
	EXT_TAG_DisplayID_TypeVII_VTDB   = 34, /* DisplayID Type-VII Video Timing Data Block */
	EXT_TAG_DisplayID_TypeVIII_VTDB  = 35, /* DisplayID Type-VIII Video Timing Data Block */
	/* reserved: 36...41 */
	EXT_TAG_DisplayID_TypeX_VTDB     = 42, /* DisplayID Type-X Video Timing Data Block */
	/* reserved: 43...119 */
	EXT_TAG_HF_EEODB                 = 120, /* HDMI Forum EDID Extension Override Data Block */
	EXT_TAG_HF_SCDB                  = 121, /* HDMI Forum Sink Capability Data Block */
	/* reserved: 122...127 for HDMI */
	/* reserved: 128...255 */
};

static int _sedid_parse_scds(u8 *data, struct shdmi_sink_s *sink)
{
	int len = shdmi_bit_filed(data[0], 0, 5);

	if (len < 7)
		return -1;

	sink->sink_ver = 1;
	sink->HdmiForum.mVersion         = data[4];
	sink->HdmiForum.mMaxTmdsCharRate = data[5];
	sink->HdmiForum.mSCDC_Present        = shdmi_bit_filed(data[6], 7, 1);
	sink->HdmiForum.mRR_Capable          = shdmi_bit_filed(data[6], 6, 1);
	sink->HdmiForum.mLTS_340Mcs_scramble = shdmi_bit_filed(data[6], 3, 1);
	sink->HdmiForum.mIndependentView     = shdmi_bit_filed(data[6], 2, 1);
	sink->HdmiForum.mDualView            = shdmi_bit_filed(data[6], 1, 1);
	sink->HdmiForum.m3D_OSD_Disparity    = shdmi_bit_filed(data[6], 0, 1);
	sink->HdmiForum.mMaxFRLRate   = shdmi_bit_filed(data[7], 4, 4);
	sink->HdmiForum.mDC_48bit_420 = shdmi_bit_filed(data[7], 2, 1);
	sink->HdmiForum.mDC_36bit_420 = shdmi_bit_filed(data[7], 1, 1);
	sink->HdmiForum.mDC_30bit_420 = shdmi_bit_filed(data[7], 0, 1);
	sink->HdmiForum.mValid = true;

	if (len >= 8) {
		sink->HdmiForum.Mdelta    = shdmi_bit_filed(data[8], 5, 1);
		sink->HdmiForum.CinemaVRR = shdmi_bit_filed(data[8], 4, 1);
		sink->HdmiForum.CNMVRR    = shdmi_bit_filed(data[8], 3, 1);
		sink->HdmiForum.FVA       = shdmi_bit_filed(data[8], 2, 1);
		sink->HdmiForum.ALLM      = shdmi_bit_filed(data[8], 1, 1);
		sink->HdmiForum.FAPA      = shdmi_bit_filed(data[8], 0, 1);
	}

	if (len >= 10) {
		sink->HdmiForum.VRRmin = data[9];
		sink->HdmiForum.VRRmax = data[10];
	}
	return 0;
}

static int _sedid_parse_h14b_basic_cap(struct sink_hdmivsdb_s *vsdb, u8 *data)
{
	vsdb->mSupportsAi    = shdmi_bit_filed(data[6], 7, 1);
	vsdb->mDeepColor48   = shdmi_bit_filed(data[6], 6, 1);
	vsdb->mDeepColor36   = shdmi_bit_filed(data[6], 5, 1);
	vsdb->mDeepColor30   = shdmi_bit_filed(data[6], 4, 1);
	vsdb->mDeepColorY444 = shdmi_bit_filed(data[6], 3, 1);
	vsdb->mDviDual       = shdmi_bit_filed(data[6], 0, 1);
	return 0;
}

static u8 _sedid_parse_h14b_latency(struct sink_hdmivsdb_s *vsdb, u8 *data)
{
	u8 video_offset = 9;

	if (shdmi_bit_filed(data[8], 7, 1) != 1) {
		vsdb->mVideoLatency = 0;
		vsdb->mAudioLatency = 0;
		vsdb->mInterlacedVideoLatency = 0;
		vsdb->mInterlacedAudioLatency = 0;
		video_offset = 9;
		goto exit;
	}

	if (shdmi_bit_filed(data[8], 6, 1) != 1) {
		vsdb->mVideoLatency = data[9];
		vsdb->mAudioLatency = data[10];
		vsdb->mInterlacedVideoLatency = 0;
		vsdb->mInterlacedAudioLatency = 0;
		video_offset = 11;
		goto exit;
	}

	vsdb->mVideoLatency = data[9];
	vsdb->mAudioLatency = data[10];
	vsdb->mInterlacedVideoLatency = data[11];
	vsdb->mInterlacedAudioLatency = data[12];
	video_offset = 13;

exit:
	vsdb->mContentTypeSupport = shdmi_bit_filed(data[8], 0, 4);
	return video_offset;
}

static int _sedid_parse_vsdb_h14b(struct sink_hdmivsdb_s *vsdb, u8 *data)
{
	unsigned char len, videoInfoStart, hdmi3dStart, hdmiVicLen;
	unsigned char hdmi3dLen, vic3d, struct3d, detail3d, spanned3d = 0;
	int i, j;

	len = shdmi_bit_filed(data[0], 0, 5);
	if (len < 5) {
		hdmi_err("sedid check len %d invalid when parse vsdb-h14b\n", len);
		return -1;
	}

	/* memset vsdb info */
	memset(vsdb, 0x0, sizeof(struct sink_hdmivsdb_s));

	vsdb->mId = HDMI_IEEE_OUI;
	vsdb->mPhysicalAddress = shdmi_byte_to_word(data[4], data[5]);

	if (len > 5)
		_sedid_parse_h14b_basic_cap(vsdb, data);

	if (len > 6)
		vsdb->mMaxTmdsClk = data[7];

	if (len > 7)
		videoInfoStart = _sedid_parse_h14b_latency(vsdb, data);

	if ((len > 9) && (shdmi_bit_filed(data[8], 5, 1) == 1)) {
		vsdb->mImageSize = shdmi_bit_filed(data[videoInfoStart], 3, 2);
		hdmiVicLen = shdmi_bit_filed(data[videoInfoStart + 1], 5, 3);
		hdmi3dLen = shdmi_bit_filed(data[videoInfoStart + 1], 0, 5);
		for (i = 0; i < hdmiVicLen; i++) {
			vsdb->mHdmiVic[i] = data[videoInfoStart + 2 + i];
		}
		vsdb->mHdmiVicCount = hdmiVicLen;
		if (shdmi_bit_filed(data[videoInfoStart], 7, 1) == 1) {	/* 3d present */
			vsdb->m3dPresent = true;
			vsdb->hdmi3dCount = hdmi3dLen;
			hdmi3dStart = videoInfoStart + hdmiVicLen + 2;
			/* 3d multi 00 -> both 3d_structure_all and 3d_mask_15 are NOT present */
			/* 3d mutli 11 -> reserved */
			if (shdmi_bit_filed(data[videoInfoStart], 5, 2) == 1) {	/* 3d multi 01 */
				/* 3d_structure_all is present but 3d_mask_15 not present */
				for (j = 0; j < 16; j++) {	/* j spans 3d structures */
					if (shdmi_bit_filed(data[hdmi3dStart + (j / 8)], (j % 8), 1) == 1) {
						int struct3d = (j < 8)	? j + 8 : j - 8;
						for (i = 0; i < 16; i++) {	/* go through 2 registers, [videoInfoStart + hdmiVicLen + 1] & [videoInfoStart + hdmiVicLen + 2]  */
							vsdb->mVideo3dStruct[i][struct3d] = 1;
						}
					}
				}
				spanned3d = 2;
			} else if (shdmi_bit_filed(data[videoInfoStart], 5, 2) == 2) {	/* 3d multi 10 */
				/* 3d_structure_all and 3d_mask_15 are present */
				for (j = 0; j < 16; j++) {
					for (i = 0; i < 16; i++) {	/* assign according to mask, through 2 registers, [videoInfoStart + hdmiVicLen + 3] & [videoInfoStart + hdmiVicLen + 4] */
						if (shdmi_bit_filed(data[hdmi3dStart + 2 + (i / 8)], (i % 8), 1) == 1) {	/* go through 2 registers, [videoInfoStart + hdmiVicLen + 1] & [videoInfoStart + hdmiVicLen + 2]  */
							vsdb->mVideo3dStruct[(i < 8) ? i + 8 : i - 8][(j < 8) ? j + 8 : j - 8] = shdmi_bit_filed(data[hdmi3dStart + (j / 8)], (j % 8), 1);
						}
					}
				}
				spanned3d = 4;
			}
			if (hdmi3dLen > spanned3d) {
				hdmi3dStart += spanned3d;
				for (i = 0; i < (hdmi3dLen - spanned3d); i++) {
					vic3d = shdmi_bit_filed(data[hdmi3dStart + i], 4, 4);
					struct3d = shdmi_bit_filed(data[hdmi3dStart + i], 0, 4);
					hdmi_inf("3D struct %u - %u", vic3d, struct3d);
					vsdb->mVideo3dStruct[vic3d][struct3d] = 1;

					if (struct3d > 7) {	/* bytes with 3D_Detail_X and Reserved(0) are present only when 3D_Structure_X > b'1000 - side-by-side(half) */
						i++;
						detail3d = shdmi_bit_filed(data[hdmi3dStart + i], 4, 4);
						hdmi_inf("3D_Detail_X struct %u", detail3d);
						vsdb->mDetail3d[vic3d][struct3d] = detail3d;
					}
				}
			}
		} else {	/* 3d NOT present */
			vsdb->m3dPresent = false;
		}
	}
	vsdb->mValid = true;
	return 0;
}

static int _sedid_parse_vsdb_hf(u8 *data, struct shdmi_sink_s *sink)
{
	return _sedid_parse_scds(data, sink);
}

static int _sedid_parse_ext_vcdb(u8 *data, struct shdmi_sink_s *sink)
{
	hdmi_trace("sedid parse ext data block: video capability\n");
	sink->vcdb.mCeScanInfo = shdmi_bit_filed(data[2], 0, 2);
	sink->vcdb.mItScanInfo = shdmi_bit_filed(data[2], 2, 2);
	sink->vcdb.mPreferredTimingScanInfo = shdmi_bit_filed(data[2], 4, 2);
	sink->vcdb.mQuantizationRangeSelectable = (shdmi_bit_filed(data[2], 6, 1) == 1) ? true : false;
	sink->vcdb.mValid = true;
	return 0;
}

static int _sedid_parse_ext_cmdb(u8 *data, struct shdmi_sink_s *sink)
{
	hdmi_trace("sedid parse ext data block: colorimetry\n");
	sink->cmdb.mValid = true;
	sink->cmdb.mByte3 = data[2];
	sink->cmdb.mByte4 = data[3];
	return 0;
}

static int _sedid_parse_ext_hdr_smdb(u8 *data, struct shdmi_sink_s *sink)
{
	u8 len = shdmi_bit_filed(data[0], 0, 5);

	if (len < 0x03) {
		hdmi_err("edid check hdr static metadata len[%d] invalid\n", len);
		return -1;
	}

	sink->hdr_sdmb.et_n = shdmi_bit_filed(data[2], 0, 5);
	sink->hdr_sdmb.sm_n = data[3];
	if (len > 3)
		sink->hdr_sdmb.dc_max_lum_data = data[4];
	if (len > 4)
		sink->hdr_sdmb.dc_max_fa_lum_data = data[5];
	if (len > 5)
		sink->hdr_sdmb.dc_min_lum_data = data[6];
	return 0;
}

static int _sedid_parse_ext_vpdb(u8 *data, struct shdmi_sink_s *sink)
{
	int i = 0, len = shdmi_bit_filed(data[0], 0, 5) - 1;

	hdmi_trace("sedid parse ext data block: video format prefer\n");
	for (i = 0; i < len; i++) {
		if (sink->prefer_index >= ARRAY_SIZE(sink->prefer_table)) {
			hdmi_err("sedid save video prefer buffer full\n");
			return -1;
		}
		sink->prefer_table[i] = data[2 + i];
		sink->prefer_index++;
	}
	return 0;
}

static int _sedid_parse_ext_hfeeodb(u8 *data, struct shdmi_sink_s *sink)
{
	int len = shdmi_bit_filed(data[0], 0, 5);

	if (len < 2) {
		hdmi_err("sedid check hfeeodb len invalid\n");
		return -1;
	}

	hdmi_trace("sedid parse ext data block: extension override\n");
	sink->HdmiForumEEO.mValid = true;
	sink->HdmiForumEEO.mExtBlkCnt = data[2];
	return 0;
}

static int _sedid_parse_ext_hfscdb(u8 *data, struct shdmi_sink_s *sink)
{
	return _sedid_parse_scds(data, sink);
	return 0;
}

static int _sedid_parse_ext_yuv420vdb(u8 *data, struct shdmi_sink_s *sink)
{
	struct sink_svd_s svd;
	int i = 0, c = 0, length = shdmi_bit_filed(data[0], 0, 5) - 1;;

	hdmi_trace("sedid parse ext data block: yuv420 video\n");
	sink->sink_ver = 1;
	for (i = 0; i < length; i++) {
		if (sink->svd_index >= ARRAY_SIZE(sink->svd_table) - 1) {
			hdmi_wrn("sedid chec svd buffer is full when parse yuv420vdb\n");
			return -1;
		}
		if (data[2 + i] >= 129 && data[2 + i] <= 192) {
			svd.mNative = true;
			svd.mCode = shdmi_bit_filed(data[2 + i], 0, 7);
		} else {
			svd.mNative = false;
			svd.mCode = data[2 + i];
		}
		svd.mLimitedToYcc420 = true;
		svd.mYcc420 = 0;
		for (c = 0; c < sink->svd_index; c++) {
			if (svd.mCode == sink->svd_table[c].mCode) {
				sink->svd_table[c].mLimitedToYcc420 = true;
				goto next;
			}
		}
		sink->svd_table[sink->svd_index] = svd;
		sink->svd_index++;
next: ;
	}
	return 0;
}

static int _sedid_parse_ext_yuv420cmdb(u8 *data, struct shdmi_sink_s *sink)
{
	int i = 0, c = 0, length = shdmi_bit_filed(data[0], 0, 5) - 1;
	int map = 0;

	hdmi_trace("sedid parse ext data block: yuv420 capability map\n");
	sink->sink_ver = 1;

	if (length == 0) {
		sink->support_ycc420_all = true;
		return 0;
	}

	for (i = 0; i < length; i++) {
		for (c = 0; c < 8; c++) {
			if (data[2 + i] & BIT(c)) {
				map = c + (i * 8);
				if (map > sink->svd_index) {
					hdmi_err("sedid yuv420 map index %d invalid!\n", map);
					return -1;
				}
				sink->svd_table[map].mYcc420 = true;
			}
		}
	}
	return 0;
}

static int _sedid_parse_ext_rcdb(u8 *data, struct shdmi_sink_s *sink)
{
	if (shdmi_bit_filed(data[0], 0, 5) < 5)
		return -1;

	hdmi_trace("sedid parse ext data block: room config\n");
	sink->rcdb.mDisplay = shdmi_bit_filed(data[2], 7, 1);
	sink->rcdb.mSpeaker = shdmi_bit_filed(data[2], 6, 1);
	sink->rcdb.mSLD = shdmi_bit_filed(data[2], 5, 1);
	sink->rcdb.mSpeakerCount = shdmi_bit_filed(data[2], 0, 5);
	sink->rcdb.mSPM1 = data[3];
	sink->rcdb.mSPM2 = data[4];
	sink->rcdb.mSPM3 = data[5];
	sink->rcdb.mValid = true;
	return 0;
}

static int _sedid_parse_audio_db(u8 *data, struct shdmi_sink_s *sink)
{
	int c = 1, length = shdmi_bit_filed(data[0], 0, 5) + 1;

	hdmi_trace("edid parse audio data block!\n");
	for (c = 1; c < length; c += 3) {
		if (sink->sad_index >= (ARRAY_SIZE(sink->sad_table) - 1)) {
			hdmi_inf("edid parse save sab buffer full!\n");
			goto exit;
		}
		sink->sad_table[sink->sad_index].mFormat = shdmi_bit_filed(data[0], 3, 4);
		sink->sad_table[sink->sad_index].mMaxChannels = shdmi_bit_filed(data[0], 0, 3);
		sink->sad_table[sink->sad_index].mSampleRates = shdmi_bit_filed(data[1], 0, 7);
		sink->sad_table[sink->sad_index].mByte3 = data[2];
		sink->sad_index++;
	}
exit:
	return length;
}

static int _sedid_parse_video_db(u8 *data, struct shdmi_sink_s *sink)
{
	int c = 1, length = shdmi_bit_filed(data[0], 0, 5) + 1;

	hdmi_trace("edid parse video data block!\n");
	for (c = 1; c < length; c++) {
		if (sink->svd_index >= (ARRAY_SIZE(sink->svd_table) - 1)) {
			hdmi_inf("edid parse save svd buffer full!\n");
			goto exit;
		}

		if ((data[c] >= 129) && (data[c] <= 192)) {
			sink->svd_table[sink->svd_index].mNative = true;
			sink->svd_table[sink->svd_index].mCode = shdmi_bit_filed(data[c], 0, 7);
		} else {
			sink->svd_table[sink->svd_index].mNative = false;
			sink->svd_table[sink->svd_index].mCode = data[c];
		}
		sink->svd_table[sink->svd_index].mLimitedToYcc420 = false;
		sink->svd_table[sink->svd_index].mYcc420 = false;
		hdmi_trace("sunxi edid parse svd[%d] -> code[%d]\n", sink->svd_index, sink->svd_table[sink->svd_index].mCode);
		sink->svd_index++;
	}
exit:
	return length;
}

static int _sedid_parse_vsdb(u8 *data, struct shdmi_sink_s *sink)
{
	int length = shdmi_bit_filed(data[0], 0, 5) + 1;
	unsigned int id = shdmi_byte_to_dword(0x00, data[3], data[2], data[1]);

	switch (id) {
	case HDMI_IEEE_OUI:
		_sedid_parse_vsdb_h14b(&sink->HdmiVsdb, data);
		break;
	case HDMI_FORUM_IEEE_OUI:
		_sedid_parse_vsdb_hf(data, sink);
		break;
	default:
		hdmi_inf("edid unsupport parse vsdb id: 0x%06X\n", id);
	}

	return length;
}

static int _sedid_parse_sadb(u8 *data, struct shdmi_sink_s *sink)
{
	int length = shdmi_bit_filed(data[0], 0, 5) + 1;

	hdmi_trace("edid parse data block: speaker allocation\n");
	sink->sadb.mByte1 = data[1];
	sink->sadb.mByte2 = data[2];
	sink->sadb.mByte3 = data[3];
	sink->sadb.mValid = true;
	return length;
}

static int _sedid_parse_ext_db(u8 *data, struct shdmi_sink_s *sink)
{
	switch (data[1]) {
	case SEDID_EXT_TAG_VCDB:
		_sedid_parse_ext_vcdb(data, sink);
		break;
	case SEDID_EXT_TAG_HDMI_VDB:
		hdmi_wrn("HDMI Video Data Block");
		break;
	case SEDID_EXT_TAG_CMDB:
		_sedid_parse_ext_cmdb(data, sink);
		break;
	case SEDID_EXT_TAG_HDR_SMDB:
		_sedid_parse_ext_hdr_smdb(data, sink);
		break;
	case EXT_TAG_VFPDB:
		_sedid_parse_ext_vpdb(data, sink);
		break;
	case EXT_TAG_YCbCr420VDB:
		_sedid_parse_ext_yuv420vdb(data, sink);
		break;
	case EXT_TAG_YCbCr420CMDB:
		_sedid_parse_ext_yuv420cmdb(data, sink);
		break;
	case EXT_TAG_HDMI_ADB:
		hdmi_wrn("HDMI Audio Data Block");
		break;
	case EXT_TAG_RCDB:
		_sedid_parse_ext_rcdb(data, sink);
		break;
	case EXT_TAG_HF_EEODB:
		_sedid_parse_ext_hfeeodb(data, sink);
		break;
	case EXT_TAG_HF_SCDB:
		_sedid_parse_ext_hfscdb(data, sink);
		break;
	default:
		hdmi_wrn("not support parse Extended Data Block %d.\n", data[1]);
		break;
	}
	return 0;
}

int _sedid_parse_cea_block_data(u8 *data, struct shdmi_sink_s *sink)
{
	u8 tag = shdmi_bit_filed(data[0], 5, 3);
	u8 length = shdmi_bit_filed(data[0], 0, 5);

	switch (tag) {
	case SEDID_TAG_ADB:
		_sedid_parse_audio_db(data, sink);
		break;
	case SEDID_TAG_VDB:
		_sedid_parse_video_db(data, sink);
		break;
	case SEDID_TAG_VSDB:
		_sedid_parse_vsdb(data, sink);
		break;
	case SEDID_TAG_SADB:
		_sedid_parse_sadb(data, sink);
		break;
	case SEDID_TAG_EXT:
		_sedid_parse_ext_db(data, sink);
		break;
	default:
		hdmi_inf("sedid unsupport parse cea block tag: 0x%02X\n", tag);
		break;
	}

	return length + 1;
}

static int _sedid_parse_desc_name(struct detailed_data_string *string,
				struct shdmi_sink_s *sink)
{
	memset(sink->sink_name, 0x0, ARRAY_SIZE(sink->sink_name));
	memcpy(sink->sink_name, string, sizeof(struct detailed_data_string));
	return 0;
}

static int _sedid_parse_desc_rangelimit(struct detailed_data_monitor_range *data,
				struct shdmi_sink_s *sink)
{
	struct sink_range_limit_s *range = &sink->RangeLimits;

	range->mValid = true;
	range->mMinVerticalRate   = data->min_vfreq;
	range->mMaxVerticalRate   = data->max_vfreq;
	range->mMinHorizontalRate = data->min_hfreq_khz;
	range->mMaxHorizontalRate = data->max_hfreq_khz;
	range->mMaxPixelClock     = data->pixel_clock_mhz;
	return 0;
}

static int _sedid_parse_desc_other(struct detailed_non_pixel *data,
				struct shdmi_sink_s *sink)
{
	switch (data->type) {
	case EDID_DETAIL_MONITOR_NAME:
		return _sedid_parse_desc_name(&data->data.str, sink);
	case EDID_DETAIL_MONITOR_RANGE:
		return _sedid_parse_desc_rangelimit(&data->data.range, sink);
	default:
		hdmi_inf("edid unsupport parse desc type: 0x%02X\n", data->type);
		break;
	}
	return 0;
}

static int _sedid_parse_desc_timing(u8 data[18], struct shdmi_sink_s *sink)
{
	struct shdmi_dtd_s *dtd = &sink->dtd_table[sink->dtd_index];

	if (sink->dtd_index >= (ARRAY_SIZE(sink->dtd_table) - 1)) {
		hdmi_inf("edid parse dtd timing buffer is full!\n");
		return 0;
	}

	dtd->mCode = -1;
	dtd->mPixelRepetitionInput = 0;
	dtd->mLimitedToYcc420 = 0;
	dtd->mYcc420 = 0;

	dtd->mPixelClock = (unsigned int)(shdmi_byte_to_word(data[1], data[0]) * 10);
	if (dtd->mPixelClock < 0x01) {	/* 0x0000 is defined as reserved */
		hdmi_err("edid parse dtd pixel clock %dKhz invalid!\n", dtd->mPixelClock);
		return -1;
	}

	dtd->mHActive = shdmi_concat_bits(data[4], 4, 4, data[2], 0, 8);
	dtd->mHBlanking = shdmi_concat_bits(data[4], 0, 4, data[3], 0, 8);
	dtd->mHSyncOffset = shdmi_concat_bits(data[11], 6, 2, data[8], 0, 8);
	dtd->mHSyncPulseWidth = shdmi_concat_bits(data[11], 4, 2, data[9], 0, 8);
	dtd->mHImageSize = shdmi_concat_bits(data[14], 4, 4, data[12], 0, 8);
	dtd->mHBorder = data[15];

	dtd->mVActive = shdmi_concat_bits(data[7], 4, 4, data[5], 0, 8);
	dtd->mVBlanking = shdmi_concat_bits(data[7], 0, 4, data[6], 0, 8);
	dtd->mVSyncOffset = shdmi_concat_bits(data[11], 2, 2, data[10], 4, 4);
	dtd->mVSyncPulseWidth = shdmi_concat_bits(data[11], 0, 2, data[10], 0, 4);
	dtd->mVImageSize = shdmi_concat_bits(data[14], 0, 4, data[13], 0, 8);
	dtd->mVBorder = data[16];

	if (shdmi_bit_filed(data[17], 4, 1) != 1) {	/* if not DIGITAL SYNC SIGNAL DEF */
		hdmi_err("Invalid DTD Parameters - DIGITAL SYNC SIGNAL DEF");
		return -1;
	}

	if (shdmi_bit_filed(data[17], 3, 1) != 1) {	/* if not DIGITAL SEPATATE SYNC */
		hdmi_err("Invalid DTD Parameters - DIGITAL SEPATATE SYNC");
		return -1;
	}

	/* no stereo viewing support in HDMI */
	dtd->mInterlaced = shdmi_bit_filed(data[17], 7, 1) == 1;
	dtd->mVSyncPolarity = shdmi_bit_filed(data[17], 2, 1) == 1;
	dtd->mHSyncPolarity = shdmi_bit_filed(data[17], 1, 1) == 1;

	sink->dtd_index++;
	return 0;
}


static int _sedid_parse_base_block(u8 *data, struct shdmi_sink_s *sink)
{
	int i, ret = 0;
	struct edid *edid = (struct edid *)data;
	struct detailed_timing *desc = NULL;

	/* save sink date */
	sink->mfg_week = edid->mfg_week;
	sink->mfg_year = edid->mfg_year;
	sink->extern_flag = edid->extensions;

	for (i = 0; i < 4; i++) {
		desc = &edid->detailed_timings[i];
		if (desc->pixel_clock) {
			if (_sedid_parse_desc_timing((u8 *)desc, sink) != 0) {
				ret = -1;
				hdmi_err("sunxi edid parse desc[%d] timing failed!\n", i);
			}
		} else {
			if (_sedid_parse_desc_other(&desc->data.other_data, sink) != 0) {
				ret = -1;
				hdmi_err("sunxi edid parse desc[%d] other failed!\n", i);
			}
		}
	}

	return ret;
}

int _sedid_parse_cea_block(u8 *buffer, struct shdmi_sink_s *sink)
{
	u8 i = 0, offset = buffer[2];
	int ret = 0;

	if ((offset < 4) || (offset > 0x7F)) {
		hdmi_err("cea block check desc offset %d invalid!\n", offset);
		return -1;
	}

	if (shdmi_bit_filed(buffer[3], 0, 4))
		sink->native_dtd_num = shdmi_bit_filed(buffer[3], 0, 4);

	if (shdmi_bit_filed(buffer[3], 4, 1))
		sink->support_ycc422 = 1;

	if (shdmi_bit_filed(buffer[3], 5, 1))
		sink->support_ycc444 = 1;

	if (shdmi_bit_filed(buffer[3], 6, 1))
		sink->support_audio = 1;

	if (shdmi_bit_filed(buffer[3], 7, 1))
		sink->support_underscan = 1;

	if (offset != 4) {
		for (i = 4; i < offset; i += _sedid_parse_cea_block_data(buffer + i, sink))
			;
	}

	/* last is checksum */
	for (i = offset; i < SEDID_BLOCK_SIZE - SEDID_DTD_SIZE; i += SEDID_DTD_SIZE) {
		hdmi_inf("start parse cea block dtd timing\n");
		ret = _sedid_parse_desc_timing(&buffer[i], sink);
		if (ret != 0) {
			hdmi_err("edid cea block parse dtd timing failed!\n");
			continue;
		}
	}

	return 0;
}

static void _sedid_sync_sink_cap(struct shdmi_sink_s *sink)
{
	int i = 0, c = 0;
	u32 code = 0;

	if (sink->HdmiVsdb.mValid) {
		for (i = 0; i < sink->HdmiVsdb.mHdmiVicCount; i++) {
			code = _hdmivic_to_ceavic(sink->HdmiVsdb.mHdmiVic[i]);
			for (c = 0; c < sink->svd_index; c++) {
				if (sink->svd_table[c].mCode == code)
					break;
			}
			if (c == sink->svd_index)
				hdmi_inf("sedid check sink hdmi14 code %d not in svd\n",
					sink->HdmiVsdb.mHdmiVic[i]);
		}
	}

	if (sink->support_ycc420_all) {
		for (i = 0; i < sink->svd_index; i++) {
			if (sink->svd_table[i].mLimitedToYcc420 == false)
				sink->svd_table[i].mYcc420 = true;
		}
	}

	/* add other update info */
}

int sunxi_edid_reset(struct shdmi_sink_s *sink)
{
	memset(sink, 0x0, sizeof(struct shdmi_sink_s));
	return 0;
}

int sunxi_edid_parse(u8 *raw_edid, struct shdmi_sink_s *sink)
{
	int i = 0, ret = 0;
	u8 block[128] = {0x0}, block_num = 1;

	if (IS_ERR_OR_NULL(raw_edid) || IS_ERR_OR_NULL(sink))
		return -1;

	if (!drm_edid_is_valid((struct edid *)raw_edid)) {
		hdmi_err("sunxi edid check invalid!\n");
		return -1;
	}

	for (i = 0; i < block_num; i++) {
		memcpy((u8 *)block, (u8 *)(raw_edid + (SEDID_BLOCK_SIZE * i)), SEDID_BLOCK_SIZE);
		switch (block[0]) {
		case BLOCK_BASE:
			hdmi_trace("sunxi edid parse base block >>>>>>>>>>\n");
			ret = _sedid_parse_base_block(block, sink);
			if (ret != 0) {
				hdmi_err("edid base block parse failed!\n");
				return -1;
			}
			if (sink->extern_flag) {
				hdmi_trace("edid block number change: %d -> %d when has ext flag.\n",
						block_num, sink->extern_flag + 1);
				block_num = sink->extern_flag + 1;
			}
			break;
		case BLOCK_EXT_CEA:
			hdmi_trace("sunxi edid parse cea block >>>>>>>>>>\n");
			ret = _sedid_parse_cea_block(block, sink);
			if (ret != 0) {
				hdmi_err("edid cea block %d parse failed!\n", i);
				return -1;
			}
			if (sink->HdmiForumEEO.mValid) {
				hdmi_trace("edid block number change: %d -> %d when has hf eeodb.\n",
					block_num, sink->HdmiForumEEO.mExtBlkCnt + 2);
				block_num = sink->HdmiForumEEO.mExtBlkCnt + 2;
			}
			break;
		case BLOCK_EXT_VTB:
		case BLOCK_EXT_DI:
		case BLOCK_EXT_LS:
		case BLOCK_EXT_MI:
		default:
			hdmi_inf("sunxi edid parse unsupport block header: 0x%02X\n", block[0]);
		}
	}

	_sedid_sync_sink_cap(sink);
	return 0;
}

ssize_t sunxi_sink_dump(char *buf, struct shdmi_sink_s *sink)
{
	ssize_t n = 0;
	int i = 0;

	n += sprintf(buf + n, "[sink]\n");
	n += sprintf(buf + n, " - date    : %d-%02d\n",
		(sink->mfg_year + 1990), (sink->mfg_week));
	n += sprintf(buf + n, " - name    : %s", sink->sink_name);
	n += sprintf(buf + n, " - version : %s\n", sunxi_sink_support_hdmi2x(sink) ? "2.0" : "1.4");
	n += sprintf(buf + n, " - dtd timing\n");
	for (i = 0; i < sink->dtd_index; i++)
		n += sprintf(buf + n, "    [%d]: %dx%d%s\n", i,
			sink->dtd_table[i].mHActive, sink->dtd_table[i].mVActive,
			sink->dtd_table[i].mInterlaced ? "i" : "p");

	n += sprintf(buf + n, " - svd timing\n");
	for (i = 0; i < sink->svd_index; i++)
		n += sprintf(buf + n, "    svd[%02d]: code[%03d], OnlyYcc420[%d], Ycc420[%d]\n",
				i, sink->svd_table[i].mCode,
				sink->svd_table[i].mLimitedToYcc420,
				sink->svd_table[i].mYcc420);

	n += sprintf(buf + n, "[hfvsdb]\n");
	n += sprintf(buf + n, " - scdc    : %s\n",
			sink->HdmiForum.mSCDC_Present ? "support" : "un-support");
	n += sprintf(buf + n, " - max rate: %dMhz\n",
			sink->HdmiForum.mMaxTmdsCharRate * 5);
	if (sink->HdmiForum.mDC_30bit_420)
		n += sprintf(buf + n, " - dc-10bits\n");
	if (sink->HdmiForum.mDC_36bit_420)
		n += sprintf(buf + n, " - dc-12bits\n");
	if (sink->HdmiForum.mDC_48bit_420)
		n += sprintf(buf + n, " - dc-16bits\n");

	n += sprintf(buf + n, "\n");

	return n;
}

int sunxi_sink_support_hdmi2x(struct shdmi_sink_s *sink)
{
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return 0;
	}

	return sink->sink_ver;
}

bool sunxi_sink_support_hdmivic(struct shdmi_sink_s *sink, u32 code)
{
	int i;

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	if (code == 0) {
		hdmi_inf("%s code %d invalid!\n", __func__, code);
		return false;
	}

	for (i = 0; i < ARRAY_SIZE(sink->HdmiVsdb.mHdmiVic); i++) {
		if (sink->HdmiVsdb.mHdmiVic[i] == code)
			return true;
	}
	return false;
}

bool sunxi_sink_support_ceavic(struct shdmi_sink_s *sink, u32 code)
{
	int i;

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	if (code == 0) {
		hdmi_inf("%s code %d invalid!\n", __func__, code);
		return false;
	}

	for (i = 0; i < sink->svd_index; i++) {
		if (sink->svd_table[i].mCode == code)
			return true;
	}
	return false;
}

bool sunxi_sink_support_scdc(struct shdmi_sink_s *sink)
{
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	return sink->HdmiForum.mSCDC_Present ? true : false;
}

bool sunxi_sink_support_yuv420_limit(struct shdmi_sink_s *sink, u32 code)
{
	int i = 0;

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	if (code == 0) {
		hdmi_inf("%s code %d invalid!\n", __func__, code);
		return false;
	}

	for (i = 0; i < sink->svd_index; i++) {
		if (sink->svd_table[i].mCode == code) {
			if (sink->svd_table[i].mLimitedToYcc420) {
				return true;
			}
		}
	}
	return false;
}

bool sunxi_sink_support_yuv420(struct shdmi_sink_s *sink, u32 code)
{
	int i = 0;

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	if (code == 0) {
		hdmi_inf("%s code %d invalid!\n", __func__, code);
		return false;
	}

	for (i = 0; i < sink->svd_index; i++) {
		if (sink->svd_table[i].mCode == code) {
			if (sink->svd_table[i].mLimitedToYcc420 || sink->svd_table[i].mYcc420) {
				hdmi_trace("this code %d is support 420. limit - %d, all - %d\n",
					code, sink->svd_table[i].mLimitedToYcc420, sink->svd_table[i].mYcc420);
				return true;
			}
		}
	}
	return false;
}

bool sunxi_sink_support_yuv422(struct shdmi_sink_s *sink)
{
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	return sink->support_ycc422 ? true : false;
}

bool sunxi_sink_support_yuv444(struct shdmi_sink_s *sink)
{
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	return sink->support_ycc444 ? true : false;
}

bool sunxi_sink_support_rgb_dc(struct shdmi_sink_s *sink, u8 bits)
{
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	switch (bits) {
	case 1: /* 10-bits */
		return sink->HdmiVsdb.mDeepColor30 ? true : false;
	case 2: /* 12-bits */
		return sink->HdmiVsdb.mDeepColor36 ? true : false;
	case 3: /* 16-bits */
		return sink->HdmiVsdb.mDeepColor48 ? true : false;
	default:
		return false;
	}
}

bool sunxi_sink_support_yuv444_dc(struct shdmi_sink_s *sink, u8 bits)
{
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	if (sink->HdmiVsdb.mDeepColorY444 == 0)
		return false;

	switch (bits) {
	case 1: /* 10-bits */
		return sink->HdmiVsdb.mDeepColor30 ? true : false;
	case 2: /* 12-bits */
		return sink->HdmiVsdb.mDeepColor36 ? true : false;
	case 3: /* 16-bits */
		return sink->HdmiVsdb.mDeepColor48 ? true : false;
	default:
		return false;
	}
}

bool sunxi_sink_support_yuv422_dc(struct shdmi_sink_s *sink, u8 bits)
{
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	switch (bits) {
	case 1: /* 10-bits */
		return sink->HdmiVsdb.mDeepColor30 ? true : false;
	case 2: /* 12-bits */
		return sink->HdmiVsdb.mDeepColor36 ? true : false;
	case 3: /* 16-bits */
		return sink->HdmiVsdb.mDeepColor48 ? true : false;
	default:
		return false;
	}
}

bool sunxi_sink_support_yuv420_dc(struct shdmi_sink_s *sink, u8 bits)
{
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	switch (bits) {
	case 1: /* 10-bits */
		return sink->HdmiForum.mDC_30bit_420 ? true : false;
	case 2: /* 12-bits */
		return sink->HdmiForum.mDC_36bit_420 ? true : false;
	case 3: /* 16-bits */
		return sink->HdmiForum.mDC_48bit_420 ? true : false;
	default:
		return false;
	}
}

bool sunxi_sink_support_sdr(struct shdmi_sink_s *sink)
{
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	return sink->hdr_sdmb.et_n & BIT(0) ? true : false;
}

bool sunxi_sink_support_hdr10(struct shdmi_sink_s *sink)
{
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	return sink->hdr_sdmb.et_n & BIT(2) ? true : false;
}

bool sunxi_sink_support_hlg(struct shdmi_sink_s *sink)
{
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	return sink->hdr_sdmb.et_n & BIT(3) ? true : false;
}

bool sunxi_sink_support_max_tmdsclk(struct shdmi_sink_s *sink, u32 clk)
{
	u32 max_rate = 340;

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	if (sunxi_sink_support_hdmi2x(sink)) {
		if (sink->HdmiForum.mMaxTmdsCharRate)
			max_rate = sink->HdmiForum.mMaxTmdsCharRate * 5;
		else
			max_rate = 600;
	}

	hdmi_trace("sink %s max rate: %d, check rate: %d\n",
		sunxi_sink_support_hdmi2x(sink) ? "hdmi-2x" : "hdmi-1x", max_rate * 1000, clk);
	return (max_rate * 1000) > clk ? true : false;
}

bool sunxi_sink_support_isrc(struct shdmi_sink_s *sink)
{
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return false;
	}

	return sink->HdmiVsdb.mSupportsAi ? true : false;
}

/* file end */
