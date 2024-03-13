/*
 *
 * Extensions to the debug module functionality for demo purposes
 */
#include "memfault/components.h"

#include <zephyr/kernel.h>

#if !defined(CONFIG_ADP536X)
#error "CONFIG_ADP536X must be defined"
#endif
#include <adp536x.h>

#define MODULE battery_debug_module
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DEBUG_MODULE_LOG_LEVEL);

/* 1 means discharging, 0 means charging, -1 means error */
static int prv_adp536x_is_discharging(void)
{
	uint8_t status;
	int err = adp536x_charger_status_1_read(&status);
	if (err) {
		LOG_ERR("Failed to get charger status: %d", err);
		return -1;
	}
	/*
		bits [2:0] are CHARGER_STATUS states:
		Charger Status Bus. The following values are indications for the charger status:
		000 = off.
		001 = trickle charge.
		010 = fast charge (constant current mode).
		011 = fast charge (constant voltage mode).
		100 = charge complete.
		101 = LDO mode.
		110 = trickle or fast charge timer expired.
		111 = battery detection.
		Only 0b000 means the battery is connected and discharging.
	*/
	return (status & 0x7) == 0 ? 1 : 0;
}

int memfault_platform_get_stateofcharge(sMfltPlatformBatterySoc *soc)
{
	int err;
	uint8_t percentage;

	uint16_t millivolts;
	err = adp536x_fg_volts(&millivolts);
	if (err == 0) {
		memfault_metrics_heartbeat_set_unsigned(MEMFAULT_METRICS_KEY(battery_voltage_mv),
							millivolts);
	} else {
		LOG_ERR("Failed to get battery voltage: %d", err);
	}

	err = adp536x_fg_soc(&percentage);
	if (err) {
		LOG_ERR("Failed to get battery level: %d", err);
		return -1;
	}

	const int discharging = prv_adp536x_is_discharging();

	// failed to retrieve charging status, return error
	if (discharging < 0) {
		return -1;
	}

	*soc = (sMfltPlatformBatterySoc){
		.soc = percentage,
		.discharging = discharging,
	};
	return 0;
}
