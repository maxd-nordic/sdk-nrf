/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/retention/bootmode.h>
LOG_MODULE_REGISTER(boot_mode, CONFIG_LOG_DEFAULT_LEVEL);

static int cmd_bootmode(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
	if (ret) {
		LOG_ERR("Failed to set boot mode");
	} else {
		LOG_INF("Boot mode set to bootloader");
	}
	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	boot_mode_cmd,
	SHELL_CMD_ARG(set, NULL,
		      "Set boot mode to bootloader",
		      cmd_bootmode,
		      1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(boot_mode, &boot_mode_cmd, "Boot mode commands", NULL);
