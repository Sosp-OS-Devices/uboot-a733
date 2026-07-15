/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi_drm_time_trace.c
 *
 * Copyright (C) 2023 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * Sunxi DRM Time Trace framework implementation.
 */

#include <common.h>
#include <command.h>
#include <console.h>
#include <asm/global_data.h>
#include <sunxi_drm_time_trace.h>

#ifdef CONFIG_AW_DRM_TIME_TRACE

/* Structure to hold data for each traced name */
struct sunxi_trace_entry {
	const char *name;		/* Name of the trace section */
	enum sunxi_trace_level level;	/* Level of this trace section */
	ulong total_time_ms;		/* Total accumulated time (ms) */
	ulong count;			/* Number of times this section was executed */
	ulong start_time_ms;		/* Start time for the current measurement (if active) */
};

/* Static array to hold all trace entries */
static struct sunxi_trace_entry sunxi_trace_data[SUNXI_TRACE_MAX_ENTRIES];

/* Global variable to store the current print level */
static enum sunxi_trace_level current_print_level = SUNXI_DRM_TRACE_LEVEL0;

void sunxi_drm_time_trace_start(const char *name, enum sunxi_trace_level level)
{
	int i;
	struct sunxi_trace_entry *entry = NULL;
	ulong current_time;

	/* Find an existing entry with this name or the first empty slot */
	for (i = 0; i < SUNXI_TRACE_MAX_ENTRIES; i++) {
		if (sunxi_trace_data[i].name == NULL) {
			/* Found an empty slot, claim it */
			sunxi_trace_data[i].name = name;
			sunxi_trace_data[i].level = level;
			entry = &sunxi_trace_data[i];
			break;
		} else if (sunxi_trace_data[i].name == name) {
			/* Found an existing entry with the same name */
			entry = &sunxi_trace_data[i];
			/* Check if level matches and if already started */
			if (entry->level != level) {
				printf("WARNING: %s('%s'): Level mismatch (had %d, new %d). Using new level.\n",
				       __func__, name, entry->level, level);
				entry->level = level;
			}
			if (entry->start_time_ms != 0) {
				printf("WARNING: %s('%s') called without matching END. Overwriting start time.\n", __func__, name);
			}
			break;
		}
	}

	if (entry == NULL) {
		printf("ERROR: %s('%s'): No more slots available (max=%d).\n", __func__, name, SUNXI_TRACE_MAX_ENTRIES);
		return;
	}

	/* Record the start time using U-Boot's get_timer (0) which returns milliseconds */
	current_time = get_timer(0);
	entry->start_time_ms = current_time;
}

void sunxi_drm_time_trace_end(const char *name)
{
	int i;
	struct sunxi_trace_entry *entry = NULL;
	ulong current_time, duration;

	/* Find the entry with the matching name */
	for (i = 0; i < SUNXI_TRACE_MAX_ENTRIES; i++) {
		if (sunxi_trace_data[i].name == name) {
			entry = &sunxi_trace_data[i];
			break;
		}
	}

	if (entry == NULL) {
		printf("ERROR: %s('%s'): No START recorded for this name.\n", __func__, name);
		return;
	}

	if (entry->start_time_ms == 0) {
		printf("ERROR: %s('%s'): START not called or already ended for this segment.\n", __func__, name);
		return;
	}

	/* Calculate duration and update stats */
	current_time = get_timer(0);
	duration = current_time - entry->start_time_ms;

	entry->total_time_ms += duration;
	entry->count++;
	/* Reset the start time for this entry */
	entry->start_time_ms = 0;
}

void sunxi_drm_time_trace_print(enum sunxi_trace_level print_level)
{
	int i;
	ulong grand_total_ms = 0;
	int has_entries = 0;

	printf("\n--- Sunxi DRM Time Trace Results (Level <= %d) ---\n", print_level);
	printf("%-20s %6s %12s %10s\n", "Name", "Level", "Total (ms)", "Count");
	printf("------------------------------------------------\n");

	for (i = 0; i < SUNXI_TRACE_MAX_ENTRIES; i++) {
		if (sunxi_trace_data[i].name != NULL &&
		    sunxi_trace_data[i].count > 0 &&
		    sunxi_trace_data[i].level <= print_level) {
			printf("%-20s %6d %12lu %10lu\n",
			       sunxi_trace_data[i].name,
			       sunxi_trace_data[i].level,
			       sunxi_trace_data[i].total_time_ms,
			       sunxi_trace_data[i].count);
			grand_total_ms += sunxi_trace_data[i].total_time_ms;
			has_entries = 1;
		}
	}

	if (has_entries) {
		printf("------------------------------------------------\n");
		printf("%-20s %6s %12lu\n", "Grand Total", "-", grand_total_ms);
	} else {
		printf("No trace data recorded for level <= %d.\n", print_level);
	}
	printf("--- End of Results ---\n\n");
}

void sunxi_drm_time_trace_reset(void)
{
	int i;
	for (i = 0; i < SUNXI_TRACE_MAX_ENTRIES; i++) {
		sunxi_trace_data[i].name = NULL;
		sunxi_trace_data[i].level = SUNXI_DRM_TRACE_LEVEL0;
		sunxi_trace_data[i].total_time_ms = 0;
		sunxi_trace_data[i].count = 0;
		sunxi_trace_data[i].start_time_ms = 0;
	}
	printf("Sunxi DRM time trace data reset.\n");
}

void sunxi_drm_time_trace_set_level(enum sunxi_trace_level level)
{
	if (level < SUNXI_DRM_TRACE_LEVEL0 || level >= SUNXI_DRM_TRACE_LEVEL_MAX) {
		printf("Error: Invalid level %d. Must be between %d and %d.\n",
		       level, SUNXI_DRM_TRACE_LEVEL0, SUNXI_DRM_TRACE_LEVEL_MAX - 1);
		return;
	}
	current_print_level = level;
	printf("Sunxi DRM time trace print level set to %d.\n", level);
}

/* Command implementation */
static int do_sunxi_drm_time_trace(struct cmd_tbl_s *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc == 1) {
		/* Default action: print results using current global level */
		sunxi_drm_time_trace_print(current_print_level);
		return 0;
	}

	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			sunxi_drm_time_trace_reset();
			return 0;
		} else {
			/* Assume the argument is a number for the print level */
			ulong level;
			if (strict_strtoul(argv[1], 10, &level) == 0) {
				if (level < SUNXI_DRM_TRACE_LEVEL_MAX) {
					sunxi_drm_time_trace_set_level(level);
					return 0;
				} else {
					printf("Error: Level must be less than %d.\n", SUNXI_DRM_TRACE_LEVEL_MAX);
					return CMD_RET_USAGE;
				}
			} else {
				return CMD_RET_USAGE;
			}
		}
	}
	/* If more arguments or parsing failed */
	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	sunxi_drm_time_trace,	/* Command name */
	2,			/* Max number of arguments */
	1,			/* Repeatable? */
	do_sunxi_drm_time_trace, /* Command function */
	"Sunxi DRM Time Trace: measure and print execution time of code sections", /* Description */
	"[<level>|reset]\n"	/* Usage */
	"\t- Print results up to <level> (default: current level).\n"
	"\t- Use 'reset' to clear all data.\n"
	"\t- Use a number (0-3) to set the current print level.\n"
	"\t\t0 - 'For Test Case'\n\t\t1 - 'For Debug Boot Time'\n\t\t2 - 'Almost All'\n\t\t3 - 'Other'\n"
);

#endif /* CONFIG_AW_DRM_TIME_TRACE */
