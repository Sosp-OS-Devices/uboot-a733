/*
 * Copyright (C) 2016 Allwinnertech
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <fdt_support.h>
#include <linux/compat.h>
#include <clk/clk.h>
#include <clk/clk_plat.h>

static int do_sunxi_clk_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int i = 0;
	struct clk *clk;
	int ret;
	struct clk *parent_clk;
	unsigned long rate;
	char *test_clk[1024] = {"hosc", "pll_periph0_2x", "pll_periph1_2x", "pll_periph1_800m", "pll_periph1_600m", "pll_periph0_300m", "sdmmc0_mod", "sdmmc2_mod"};

	for (i = 0; i < 9; i++) {
		/* 获得时钟 */
		clk = clk_get(NULL, test_clk[i]);
		if (!clk) {
			printf("error to get clk: %s\n", test_clk[i]);
		} else {
			printf("success get clk: %s\n", test_clk[i]);
		}

		ret = clk_prepare_enable(clk);
		if (ret)
			printf("enable clk: %s failed!\n", clk->name);
		else
			printf("enable clk: %s \n", clk->name);

		/* 获得父时钟 */
		parent_clk = clk_get_parent(clk);
		if (!parent_clk || IS_ERR(parent_clk)) {
			printf("error to get clk: %s parent_name: %s\n", test_clk[i], parent_clk->name);
		} else {
			printf("success to get clk: %s parent_name: %s\n", test_clk[i], parent_clk->name);
		}

		/* 获得当前频率 */
		rate = clk_get_rate(clk);
		printf("success to get clk: %s parent_name: %s\n rate: %ld\n", test_clk[i], parent_clk->name, rate);
	}

	return 0;
}

static int do_clk_dump(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	clk_summary_show();
	return CMD_RET_SUCCESS;
}

static cmd_tbl_t cmd_clk_sub[] = {
	U_BOOT_CMD_MKENT(dump, 1, 1, do_clk_dump, "", ""),
	U_BOOT_CMD_MKENT(test, 1, 1, do_sunxi_clk_test, "", ""),
};

static int do_clk(cmd_tbl_t *cmdtp, int flag, int argc,
		  char *const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return CMD_RET_USAGE;

	/* Strip off leading 'clk' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_clk_sub[0], ARRAY_SIZE(cmd_clk_sub));

	if (c)
		return c->cmd(cmdtp, flag, argc, argv);
	else
		return CMD_RET_USAGE;
}

#ifdef CONFIG_SYS_LONGHELP
static char clk_help_text[] =
	"dump - Print clock frequencies\n"
	"test - Test clock get/enable/get_parent/get_rate interface\n";
#endif

U_BOOT_CMD(sunxi_clk, 5, 1, do_clk, "CLK sub-system", clk_help_text);
