// SPDX-License-Identifier: GPL-2.0+
/*
 * sunxi GPIOLED driver for uboot.
 *
 * Copyright (C) 2024
 * 2024.12.26 rengaomin <rengaomin@allwinnertech.com>
 */

#ifndef _SUNXI_GPIOLED_H_
#define _SUNXI_GPIOLED_H_

#define SPRITE_LED_OFF 0
#define SPRITE_LED_ON 1

enum led_ctrl {
	GPIOLED_OFF = 0,
	GPIOLED_ON,
	GPIOLED_BOOT_FLASHING,
	GPIOLED_SPRITE_FLASHING,
	GPIOLED_SPRITE_FLASHING_STOP,
	GPIOLED_SPRITE_FLASHING_ERR_STOP
};

struct sunxi_gpioled {
	u32 flash_times;
	u32 work_times;
	u32 work_delay;
	user_gpio_set_t gpio_init;
	__u32 sprite_led_hd;
	int sprite_led_status;
};

void sunxi_gpioled_ctrl(int led_ctrl, char *fdt_gpioled_path);
#endif /* _SUNXI_GPIOLED_H_ */
