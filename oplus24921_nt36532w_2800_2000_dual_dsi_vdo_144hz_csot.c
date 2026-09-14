// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/backlight.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_modes.h>
#include <linux/delay.h>
#include <drm/drm_connector.h>
#include <drm/drm_device.h>
#include <linux/of_graph.h>

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <soc/oplus/device_info.h>
#include "../oplus/oplus_display_mtk_debug.h"
#include <soc/oplus/system/oplus_mm_kevent_fb.h>

#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
#include "../mediatek/mediatek_v2/mtk_corner_pattern/oplus_24921_novatek_mtk_data_hw_roundedpattern_l.h"
#include "../mediatek/mediatek_v2/mtk_corner_pattern/oplus_24921_novatek_mtk_data_hw_roundedpattern_r.h"
#endif

#include "ktz8866.h"

#define CONFIG_MTK_PANEL_EXT
#if defined(CONFIG_MTK_PANEL_EXT)
#include "../mediatek/mediatek_v2/mtk_panel_ext.h"
#include "../mediatek/mediatek_v2/mtk_drm_graphics_base.h"
#endif

#if IS_ENABLED(CONFIG_OPLUS_MTK_DRM_GKI_NOTIFY)
#include "../mediatek/mediatek_v2/mtk_disp_notify.h"
#define LCD_CTL_RST_OFF 0x12
#define LCD_CTL_CS_OFF  0x1A
#define LCD_CTL_TP_LOAD_FW 0x10
#define LCD_CTL_CS_ON  0x19
#endif

#define MAX_NORMAL_BRIGHTNESS           2047
#define PHYSICAL_WIDTH_MM               240
#define PHYSICAL_HEIGHT_MM              171

#define REGFLAG_CMD                     0xFFFA
#define REGFLAG_DELAY                   0xFFFC
#define REGFLAG_UDELAY                  0xFFFB
#define REGFLAG_END_OF_TABLE            0xFFFD

static int g_fps_current = 120;

extern unsigned long esd_flag;
extern bool g_dp_support;
static bool is_pd_with_guesture = false;
extern unsigned int oplus_enhance_mipi_strength;

extern unsigned int silence_mode;
extern bool g_shutdown;

#if IS_ENABLED(CONFIG_TOUCHPANEL_NOTIFY)
extern int (*tp_gesture_enable_notifier)(unsigned int tp_index);
#endif

#define  M_DELAY(n) usleep_range(n*1000, n*1000+100)
#define  U_DELAY(n) usleep_range(n, n+10)

#define HFP_144HZ (54)
#define HFP_90_50_48 (169)
#define HFP_120_60_30 (170)
#define HSA (28)
#define HBP (26)
#define HBP_90HZ (120)
#define VFP (26)
#define VFP_90HZ (768)
#define VFP_50HZ (3140)
#define VFP_48HZ (3360)
#define VFP_144HZ (26)
#define VFP_60HZ (2250)
#define VFP_120HZ (26)
#define VFP_30HZ (6698)
#define VSA (2)
#define VBP (196)
#define VAC_FHD (2000)
#define HAC_FHD (2800)

unsigned int level_backup = 0;
extern unsigned int oplus_display_brightness;
static bool lcd_bl_set_led_flag = false;
static const struct drm_display_mode hand_display_mode_90hz = {
	//fhd_sdc_90_mode
	.clock = 835522, //3130*2966*90 htotal*vtotal*fps
	.hdisplay = HAC_FHD,
	.hsync_start = HAC_FHD + HFP_90_50_48,
	.hsync_end = HAC_FHD + HFP_90_50_48 + HSA,
	.htotal = HAC_FHD + HFP_90_50_48 + HSA + HBP_90HZ,//3120
	.vdisplay = VAC_FHD,
	.vsync_start = VAC_FHD + VFP_90HZ,
	.vsync_end = VAC_FHD + VFP_90HZ + VSA,
	.vtotal = VAC_FHD + VFP_90HZ + VSA + VBP, //2966
	.hskew = 1,
};

static const struct drm_display_mode hand_display_mode_50hz = {
	//fhd_sdc_50_mode
	.clock = 835522, //3024*8896*30 htotal*vtotal*fps
	.hdisplay = HAC_FHD,
	.hsync_start = HAC_FHD + HFP_90_50_48,
	.hsync_end = HAC_FHD + HFP_90_50_48 + HSA,
	.htotal = HAC_FHD + HFP_90_50_48 + HSA + HBP_90HZ, //3120
	.vdisplay = VAC_FHD,
	.vsync_start = VAC_FHD + VFP_50HZ,
	.vsync_end = VAC_FHD + VFP_50HZ + VSA,
	.vtotal = VAC_FHD + VFP_50HZ + VSA + VBP, //
	.hskew = 1,
};

static const struct drm_display_mode hand_display_mode_48hz = {
	//fhd_sdc_48_mode
	.clock = 835522, //3024*8896*30 htotal*vtotal*fps
	.hdisplay = HAC_FHD,
	.hsync_start = HAC_FHD + HFP_90_50_48,
	.hsync_end = HAC_FHD + HFP_90_50_48 + HSA,
	.htotal = HAC_FHD + HFP_90_50_48 + HSA + HBP_90HZ, //3120
	.vdisplay = VAC_FHD,
	.vsync_start = VAC_FHD + VFP_48HZ,
	.vsync_end = VAC_FHD + VFP_48HZ + VSA,
	.vtotal = VAC_FHD + VFP_48HZ + VSA + VBP, //
	.hskew = 1,
};


static const struct drm_display_mode hand_display_mode_144hz = {
	//fhd_sdc_144_mode
	.clock = 931104, //2908*2224*144  htotal*vtotal*fps
	.hdisplay = HAC_FHD,
	.hsync_start = HAC_FHD + HFP_144HZ,
	.hsync_end = HAC_FHD + HFP_144HZ + HSA,
	.htotal = HAC_FHD + HFP_144HZ + HSA + HBP,//2908
	.vdisplay = VAC_FHD,
	.vsync_start = VAC_FHD + VFP_144HZ,
	.vsync_end = VAC_FHD + VFP_144HZ + VSA,
	.vtotal = VAC_FHD + VFP_144HZ + VSA + VBP,//2224
	.hskew = 1,
};

static const struct drm_display_mode pan_display_mode_60hz = {
	//fhd_sdc_60_mode
	.clock = 807045, //3024*4448*60 htotal*vtotal*fps
	.hdisplay = HAC_FHD,
	.hsync_start = HAC_FHD + HFP_120_60_30,
	.hsync_end = HAC_FHD + HFP_120_60_30 + HSA,
	.htotal = HAC_FHD + HFP_120_60_30 + HSA + HBP, //3024
	.vdisplay = VAC_FHD,
	.vsync_start = VAC_FHD + VFP_60HZ,
	.vsync_end = VAC_FHD + VFP_60HZ + VSA,
	.vtotal = VAC_FHD + VFP_60HZ + VSA + VBP, //4448
	.hskew = 1,
};
static const struct drm_displa