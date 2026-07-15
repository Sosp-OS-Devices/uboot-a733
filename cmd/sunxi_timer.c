// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2018-2020
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 *
 */

#include <common.h>
#include <config.h>
#include <command.h>
#include <asm/arch/timer.h>

DECLARE_GLOBAL_DATA_PTR;

struct timer_list timer0_t;
struct timer_list timer1_t;
static int timer_test_flag[2];

static void timer_test_func(void *p)
{
	struct timer_list *timer_t = (struct timer_list *)p;
	int timer_num = timer_t->timer_num;

	printf("timer%d interrupt occurred\n", timer_num);

	del_timer(timer_t);
	timer_test_flag[timer_num] = 0;

	return;
}

int sunxi_timer_test(int timer_num, int delay_time)
{
	int base_count = delay_time;
	struct timer_list *timer_ptr;
	void (*timer_func)(void *p);
	ulong timeout;

	if (timer_test_flag[timer_num]) {
		printf("can not test timer %d now\n", timer_num);
		return -1;
	}

	if (timer_num == 0) {
		timer_ptr = &timer0_t;
		timer_func = timer_test_func;
	} else if (timer_num == 1) {
		timer_ptr = &timer1_t;
		timer_func = timer_test_func;
	} else {
		printf("Invalid timer number. Use 0 or 1\n");
		return -1;
	}

	timer_ptr->data = (unsigned long)timer_ptr;
	timer_ptr->expires = base_count;
	timer_ptr->function = timer_func;
	timer_ptr->timer_num = timer_num;

	init_timer(timer_ptr);
	add_timer(timer_ptr);
	timer_test_flag[timer_num] = 1;

	printf("Waiting for timer%d interrupt trigger...\n", timer_num);
	timeout = get_timer(0);
	while (timer_test_flag[timer_num] && (get_timer(timeout) < (base_count + 100)))
		;

	if (timer_test_flag[timer_num]) {
		printf("timer%d test timeout!\n", timer_num);
		return -1;
	}

	return 0;
}

static int do_timer_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int timer_num = 0;
	int delay_time = 1000;
	int ret;
	if (argc < 2 || argc > 3) {
		printf("Usage: timer_test <timer_num> [delay_time]\n");
		printf("timer_num: 0 for timer0, 1 for timer1\n");
		printf("delay_time: delay time in ms (default: 1000)\n");
		return -1;
	}

	timer_num = simple_strtol(argv[1], NULL, 10);
	if (argc == 3)
		delay_time = simple_strtol(argv[2], NULL, 10);

	ret = sunxi_timer_test(timer_num, delay_time);
	if (ret != 0)
		printf("timer%d test failed!\n", timer_num);
	else
		printf("timer%d test passed!\n", timer_num);

	return ret;
}

U_BOOT_CMD(
	timer_test, 3, 0, do_timer_test,
	"do a timer and int test",
	"<timer_num> [delay_time] - timer_num: 0 for timer0, 1 for timer1; delay_time: delay in ms (default: 1000)"
);