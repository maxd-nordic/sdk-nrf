
#include <zephyr/kernel.h>
#include "cJSON.h"

#define COAP_SHADOW_MAX_SIZE 512

struct simple_config_val {
	enum {
		SIMPLE_CONFIG_VAL_STRING,
		SIMPLE_CONFIG_VAL_BOOL,
		SIMPLE_CONFIG_VAL_DOUBLE,
	} type;
	union {
		const char *_str;
		bool _bool;
		double _double;
	} val;
};

typedef int (*simple_config_callback_t)(const char *key, const struct simple_config_val *val);

int simple_config_handle_incoming_settings(char *buf, size_t buf_len);
cJSON *simple_config_construct_settings_obj(void);
int simple_config_update(void);
int simple_config_init_queued_configs(void);
int simple_config_set(const char *key, const struct simple_config_val *val);
void simple_config_set_callback(simple_config_callback_t cb);
void simple_config_clear_queued_configs(void);
