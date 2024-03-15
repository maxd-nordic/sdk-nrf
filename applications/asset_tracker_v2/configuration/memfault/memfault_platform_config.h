#pragma once

/*
 * Platform overrides for the default configuration settings in the
 * memfault-firmware-sdk. Default configuration settings can be found in
 * "<NCS folder>/modules/lib/memfault-firmware-sdk/components/include/memfault/default_config.h"
 */

#if defined(CONFIG_DEBUG_MODULE)
 /* Prepare captured metric data for upload to Memfault cloud every configured interval. */
#define MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS CONFIG_DEBUG_MODULE_MEMFAULT_HEARTBEAT_INTERVAL_SEC
#define MEMFAULT_DATA_EXPORT_CHUNK_MAX_LEN CONFIG_DEBUG_MODULE_MEMFAULT_CHUNK_SIZE_MAX
#endif /* defined(CONFIG_DEBUG_MODULE) */


#define MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME 1
#define MEMFAULT_METRICS_SYNC_SUCCESS 1
#define MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS 1

#if defined(CONFIG_DEBUG_MODULE_BATTERY)
#define MEMFAULT_METRICS_BATTERY_ENABLE 1
#else
#define MEMFAULT_METRICS_BATTERY_ENABLE 0
#endif
