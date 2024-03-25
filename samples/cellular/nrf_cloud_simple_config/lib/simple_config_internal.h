#include "simple_config.h"

int simple_config_init_queued_configs(void);
void simple_config_clear_queued_configs(void);
int simple_config_handle_incoming_settings(char *buf, size_t buf_len);
cJSON *simple_config_construct_settings_obj(void);
