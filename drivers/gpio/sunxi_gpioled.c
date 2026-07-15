// SPDX-License-Identifier: GPL-2.0+
/*
 * sunxi GPIOLED driver for uboot.
 *
 * Copyright (C) 2024
 * 2024.12.26 rengaomin <rengaomin@allwinnertech.com>
 */

#include <common.h>
#include <asm/arch/timer.h>
#include <sys_config.h>
#include <fdt_support.h>
#include <sunxi_board.h>
#include <sunxi_gpioled.h>

struct timer_list GPIOLED_TIMER;

/*
 * sunxi_gpioled_timer_func(): timer Interrupt handler, used to control the blinking of the light
 *
 * param: sunxi_gpioled struct
 *
 * description: Initializing the blinking led registers the interrupt, in which the led state is switched
 *
 * return:
 *
 */
static void sunxi_gpioled_timer_func(void *p)
{
	struct sunxi_gpioled *led = (struct sunxi_gpioled *)p;

	gpio_write_one_pin_value(led->sprite_led_hd, led->sprite_led_status,
				 "sprite_gpio0");
	led->sprite_led_status = (~led->sprite_led_status) & 0x01;
	if (led->work_times) {
		led->flash_times++;

		if (led->work_times == led->flash_times) {
			del_timer(&GPIOLED_TIMER);
			return;
		}
	}

	del_timer(&GPIOLED_TIMER);
	add_timer(&GPIOLED_TIMER);
	return;
}

/*
 * sunxi_gpioled_flash_deinit(): exit the led flashing status
 *
 * param1: enum led_ctrl
 * param2: path to gpio_led in the device tree
 *
 * description: exit the led flashing mode under normal conditions, if there
 *		is an anomaly, let the led flash at a frequency of 100hz
 *
 * return: 0:success -1:failed
 *
 */
static int sunxi_gpioled_flash_deinit(int led_ctrl, char *fdt_gpioled_path)
{
	int nodeoffset;
	struct sunxi_gpioled *led;

	switch (led_ctrl) {
	case GPIOLED_SPRITE_FLASHING_STOP:
		del_timer(&GPIOLED_TIMER);
		break;
	case GPIOLED_SPRITE_FLASHING_ERR_STOP:
		led = malloc(sizeof(*led));
		if (!led) {
			pr_err("%s: out of memory\n", __func__);
			return -1;
		}
		memset(led, 0, sizeof(struct sunxi_gpioled));

		nodeoffset = fdt_path_offset(working_fdt, fdt_gpioled_path);
		if (nodeoffset > 0) {
			fdt_getprop_u32(working_fdt, nodeoffset,
					"sprite_err_delay", (uint32_t *)&led->work_delay);
		}
		if (!led->work_delay) {
			led->work_delay = 100;
		}
		fdt_get_one_gpio(fdt_gpioled_path, "sprite_gpio0", &led->gpio_init);

		led->sprite_led_hd = sunxi_gpio_request(&led->gpio_init, 1);
		if (!led->sprite_led_hd) {
			pr_err("reuqest gpio for led failed\n");
			return -1;
		}

		led->sprite_led_status = GPIOLED_ON;

		del_timer(&GPIOLED_TIMER);
		GPIOLED_TIMER.data     = (unsigned long)led;
		GPIOLED_TIMER.expires  = led->work_delay;
		GPIOLED_TIMER.function = sunxi_gpioled_timer_func;
		add_timer(&GPIOLED_TIMER);
	}

	return 0;
}

/*
 * sunxi_gpioled_flash_init(): led flashing initialization
 *
 * param1: enum led_ctrl
 * param2: path to gpio_led in the device tree
 *
 * description: Get the pin configuration from the device tree and use the timer
 *		interrupt to achieve led flashing
 *
 * return: 0:success -1:failed
 *
 */
static int sunxi_gpioled_flash_init(int led_ctrl, char *fdt_gpioled_path)
{
	struct sunxi_gpioled *led = malloc(sizeof(*led));
	if (!led) {
		pr_err("%s: out of memory\n", __func__);
		return -1;
	}
	memset(led, 0, sizeof(struct sunxi_gpioled));

	if (led_ctrl == GPIOLED_BOOT_FLASHING) {
		script_parser_fetch(fdt_gpioled_path, "boot_work_delay",
			  (void *)&led->work_delay, 0);
		script_parser_fetch(fdt_gpioled_path, "boot_work_times",
			  (void *)&led->work_times, 0);
	} else if (led_ctrl == GPIOLED_SPRITE_FLASHING) {
		script_parser_fetch(fdt_gpioled_path, "sprite_work_delay",
				  (void *)&led->work_delay, 0);
	}

	if (!led->work_delay)
		led->work_delay = 500;

	fdt_get_one_gpio(fdt_gpioled_path, "sprite_gpio0", &led->gpio_init);

	led->sprite_led_hd = sunxi_gpio_request(&led->gpio_init, 1);
	if (!led->sprite_led_hd) {
		pr_err("reuqest gpio for led failed\n");
		return -1;
	}

	led->sprite_led_status = GPIOLED_ON;
	led->work_times = 2 * led->work_times;

	GPIOLED_TIMER.data     = (unsigned long)led;
	GPIOLED_TIMER.expires  = led->work_delay;
	GPIOLED_TIMER.function = sunxi_gpioled_timer_func;
	add_timer(&GPIOLED_TIMER);

	return 0;
}

/*
 * sunxi_gpioled_turn(): turn the led on or off
 *
 * param1: 1 or 0
 * param2: path to gpio_led in the device tree
 *
 * description: turn the led on or off
 *
 * return: 0:success -1:failed
 *
 */
static int sunxi_gpioled_turn(int data, char *fdt_gpioled_path)
{
	user_gpio_set_t gpio_init;
	int ret = 0;

	memset(&gpio_init, 0, sizeof(user_gpio_set_t));

	//Get the gpio pin from the device tree
	ret = fdt_get_one_gpio(fdt_gpioled_path, "sprite_gpio0", &gpio_init);
	gpio_init.data = data;
	if (!ret) {
		if (gpio_init.port) {
			ret = sunxi_gpio_request(&gpio_init, 1);
			if (!ret) {
				pr_err("reuqest gpio for led failed\n");
				return -1;
			}
			return 0;
		}
	}

	return -1;
}

/*
 * sunxi_gpioled_ctrl(): control led status
 *
 * param1: enum led_ctrl
 * param2: path to gpio_led in the device tree
 *
 * description: external call interface, to achieve the led status control
 *
 * return:
 *
 */
void sunxi_gpioled_ctrl(int led_ctrl, char *fdt_gpioled_path)
{
	switch (led_ctrl) {
	case GPIOLED_ON:
	case GPIOLED_OFF:
		sunxi_gpioled_turn(led_ctrl, fdt_gpioled_path);
		break;
	case GPIOLED_BOOT_FLASHING:
	case GPIOLED_SPRITE_FLASHING:
		sunxi_gpioled_flash_init(led_ctrl, fdt_gpioled_path);
		break;
	case GPIOLED_SPRITE_FLASHING_STOP:
	case GPIOLED_SPRITE_FLASHING_ERR_STOP:
		sunxi_gpioled_flash_deinit(led_ctrl, fdt_gpioled_path);
		break;
	default:
		break;
	}
}
