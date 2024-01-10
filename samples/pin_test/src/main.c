/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/console/console.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>
#include <modem/nrf_modem_lib.h>

const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PINSTATE_SETTINGS_BASE_KEY "pinstate"

// Function prototypes
void process_command(const char *line);
void handle_read_command(int port, int pin);
void handle_write_command(int port, int pin, char *state);

static const struct {
	const int flags;
	const char *str;
} pin_state[] = {
	{GPIO_INPUT | GPIO_OUTPUT_HIGH, "H"},
	{GPIO_INPUT | GPIO_OUTPUT_LOW, "L"},
	{GPIO_INPUT | GPIO_PULL_DOWN, "PD"},
	{GPIO_INPUT | GPIO_PULL_UP, "PU"},
	{GPIO_INPUT, "Z"},
};

void process_command(const char *line) {
    char command;
    int port, pin;
    char state[3]; // For states like "PU", "PD"

    if (sscanf(line, "%c,%d,%d,%2s", &command, &port, &pin, state) >= 3) {
        if (command == 'R') {
            handle_read_command(port, pin);
        } else if (command == 'W') {
            handle_write_command(port, pin, state);
        } else {
            printf("ERROR1\n");
        }
    } else {
        printf("ERROR2\n");
    }
}

const char *flags_to_str(int flags) {
	for (size_t i = 0; i < ARRAY_SIZE(pin_state); ++i) {
		if (pin_state[i].flags == flags) {
			return pin_state[i].str;
		}
	}
	return "?";
}

void handle_read_command(int port, int pin) {
	int state = gpio_pin_get(gpio0, pin);

	if (state < 0) {
        	printf("ERROR3\n");
	} else {
		printk("OK,%d,%d,%d\n", port, pin, state);
	}
}

void handle_write_command(int port, int pin, char *desired_state) {
	int rc = 0;
	int flags = GPIO_INPUT;
	int state = 0;
	char settings_name_buf[100] = { 0 };


	if (strcmp(desired_state, "H") == 0) {
		flags |= GPIO_OUTPUT_HIGH;
	} else if (strcmp(desired_state, "L") == 0) {
		flags |= GPIO_OUTPUT_LOW;
	} else if (strcmp(desired_state, "PU") == 0) {
		flags |= GPIO_PULL_UP;
	} else if (strcmp(desired_state, "PD") == 0) {
		flags |= GPIO_PULL_DOWN;
	} else if (strcmp(desired_state, "Z") == 0) {
		flags = GPIO_INPUT;
	} else {
		printk("ERROR4");
		return;
	}
	rc = gpio_pin_configure(gpio0, pin, flags);

	if (rc) {
        	printf("ERROR5\n");
		return;
	}

	rc = snprintk(settings_name_buf, ARRAY_SIZE(settings_name_buf),
			"%s/%d/%d", PINSTATE_SETTINGS_BASE_KEY, port, pin);

	printk("Saving to [%s]\n", settings_name_buf);
	settings_save_one(settings_name_buf, &flags, sizeof(flags));

	state = gpio_pin_get(gpio0, pin);
	if (state < 0) {
		printf("ERROR6\n");
	} else {
		printk("OK,%d,%d,%d\n", port, pin, state);
	}


}

/* This callback function is used to apply pin settings. */
static int zephyr_settings_backend_load_key_cb(const char *key, size_t len,
					       settings_read_cb read_cb,
					       void *cb_arg, void *param)
{
	ARG_UNUSED(param);

	/* key validation */
	if (!key) {
		printk("ERROR: settings entry has no key\n");
		return -EINVAL;
	}

	int port, pin, flags;

	if (sscanf(key, "%d/%d",&port, &pin) != 2) {
		printk("ERROR: settings entry has invalid key\n");
		return -EINVAL;
	}

	if(read_cb(cb_arg, &flags, sizeof(flags)) != sizeof(flags)) {
		printk("ERROR: settings entry has invalid value\n");
		return -EINVAL;
	}

	if(gpio_pin_configure(gpio0, pin, flags)) {
		printk("ERROR: couldn't configure pin\n");
		return -EIO;
	}

	return 0;
}

int main(void)
{
	int rc;

	rc = nrf_modem_lib_init();
	if (rc) {
		printk("Error initilizing modem lib\n");
	}

	rc = settings_subsys_init();
	if (rc) {
		printk("Error initilizing settings subsystem\n");
	}

	rc = settings_load_subtree_direct(PINSTATE_SETTINGS_BASE_KEY,
					  zephyr_settings_backend_load_key_cb, NULL);


	console_getline_init();


	printk("Enter a line\n");

	while (1) {
		char *s = console_getline();

		process_command(s);
	}
	return 0;
}
