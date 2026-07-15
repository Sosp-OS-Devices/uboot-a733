/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi_drm_time_trace.h
 *
 * Copyright (C) 2023 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * Sunxi DRM Time Trace framework header.
 */

#ifndef _SUNXI_DRM_TIME_TRACE_H_
#define _SUNXI_DRM_TIME_TRACE_H_

#include <common.h> /* For `get_timer` and data types */

/* Trace detail level */
enum sunxi_trace_level {
	SUNXI_DRM_TRACE_LEVEL0 = 0,
	SUNXI_DRM_TRACE_LEVEL1,
	SUNXI_DRM_TRACE_LEVEL2,
	SUNXI_DRM_TRACE_LEVEL3,
	SUNXI_DRM_TRACE_LEVEL_MAX
};

#ifdef CONFIG_AW_DRM_TIME_TRACE

/* Maximum number of unique trace names we can track */
#define SUNXI_TRACE_MAX_ENTRIES 20

/**
 * sunxi_drm_time_trace_start() - Record the start time for a given name and level.
 * @name: A string identifier for the code section being measured.
 * @level: The detail level of this trace section.
 */
void sunxi_drm_time_trace_start(const char *name, enum sunxi_trace_level level);

/**
 * sunxi_drm_time_trace_end() - Record the end time and accumulate duration for a given name.
 * @name: A string identifier for the code section being measured.
 */
void sunxi_drm_time_trace_end(const char *name);

/**
 * sunxi_drm_time_trace_print() - Print all accumulated trace results up to a certain level.
 * @print_level: The maximum level to print. Entries with level <= print_level will be shown.
 */
void sunxi_drm_time_trace_print(enum sunxi_trace_level print_level);

/**
 * sunxi_drm_time_trace_reset() - Reset all accumulated trace data.
 */
void sunxi_drm_time_trace_reset(void);

/**
 * sunxi_drm_time_trace_set_level() - Set the current global print level.
 * @level: The new print level.
 */
void sunxi_drm_time_trace_set_level(enum sunxi_trace_level level);

/* Macro for easy use */
#define SUNXI_DRM_TIME_TRACE_START(name, level) sunxi_drm_time_trace_start(name, level)
#define SUNXI_DRM_TIME_TRACE_END(name) sunxi_drm_time_trace_end(name)

#else /* CONFIG_AW_DRM_TIME_TRACE */

/* Stubs when configuration is disabled */
#define SUNXI_DRM_TIME_TRACE_START(name, level) do {} while (0)
#define SUNXI_DRM_TIME_TRACE_END(name) do {} while (0)
static inline void sunxi_drm_time_trace_print(enum sunxi_trace_level print_level) {}
static inline void sunxi_drm_time_trace_reset(void) {}
static inline void sunxi_drm_time_trace_set_level(enum sunxi_trace_level level) {}

#endif /* CONFIG_AW_DRM_TIME_TRACE */

#endif /* _SUNXI_DRM_TIME_TRACE_H_ */
