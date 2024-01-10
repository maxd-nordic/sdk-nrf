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
#include <SEGGER_RTT.h>
#include <SEGGER_RTT_Conf.h>

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

void printf_segger(const char *fmt, ...)
{
	uint8_t buf[1024] = {0};
	int rc = 0;
	va_list ap;

	va_start(ap, fmt);

	rc = snprintf(buf, ARRAY_SIZE(buf), fmt, ap);
	if (rc > 0) {
		SEGGER_RTT_WriteSkipNoLock(0, buf, rc);
	}

	va_end(ap);
}

#define READ_TIMEOUT_MS 1000

size_t readline_segger(uint8_t *buf, size_t buf_len)
{
	int rc = 0;
	int i = 0;

	int64_t reftime = k_uptime_get();

	while (i < buf_len) {
		rc = SEGGER_RTT_ReadNoLock(0, &buf[i], 1);
		if (rc == 1) {
			if (buf[i] == '\n') {
				buf[i] = 0; // terminate string
				printf_segger("%s\n", buf);
				return i;
			}
			reftime = k_uptime_get();
			i += 1;
		} else {
			// busy-wait with timeout
			if (k_uptime_delta(reftime) > READ_TIMEOUT_MS) {
				return 0;
			}
		}
	}
	return 0;
}

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
            printf_segger("ERROR1\n");
        }
    } else {
        printf_segger("ERROR2\n");
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
        	printf_segger("ERROR3\n");
	} else {
		printf_segger("OK,%d,%d,%d\n", port, pin, state);
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
		printf_segger("ERROR4");
		return;
	}
	rc = gpio_pin_configure(gpio0, pin, flags);

	if (rc) {
        	printf_segger("ERROR5\n");
		return;
	}

	rc = snprintf(settings_name_buf, ARRAY_SIZE(settings_name_buf),
			"%s/%d/%d", PINSTATE_SETTINGS_BASE_KEY, port, pin);

	printf_segger("Saving to [%s]\n", settings_name_buf);
	settings_save_one(settings_name_buf, &flags, sizeof(flags));

	state = gpio_pin_get(gpio0, pin);
	if (state < 0) {
		printf_segger("ERROR6\n");
	} else {
		printf_segger("OK,%d,%d,%d\n", port, pin, state);
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
		printf_segger("ERROR: settings entry has no key\n");
		return -EINVAL;
	}

	int port, pin, flags;

	if (sscanf(key, "%d/%d",&port, &pin) != 2) {
		printf_segger("ERROR: settings entry has invalid key\n");
		return -EINVAL;
	}

	if(read_cb(cb_arg, &flags, sizeof(flags)) != sizeof(flags)) {
		printf_segger("ERROR: settings entry has invalid value\n");
		return -EINVAL;
	}

	if(gpio_pin_configure(gpio0, pin, flags)) {
		printf_segger("ERROR: couldn't configure pin\n");
		return -EIO;
	}

	return 0;
}

int main(void)
{
	int rc;
	uint8_t buf[1024];

	rc = nrf_modem_lib_init();
	if (rc) {
		printf_segger("Error initilizing modem lib\n");
	}

	rc = settings_subsys_init();
	if (rc) {
		printf_segger("Error initilizing settings subsystem\n");
	}

	rc = settings_load_subtree_direct(PINSTATE_SETTINGS_BASE_KEY,
					  zephyr_settings_backend_load_key_cb, NULL);



	printf_segger("Enter a line\n");

	while (1) {
		rc = readline_segger(buf, ARRAY_SIZE(buf));
		if (rc) {
			process_command(buf);
		}
	}
	return 0;
}
