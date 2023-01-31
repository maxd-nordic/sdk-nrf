#include "bsec2.h"

// bus_i2c
int bsec_bme68x_init(struct bme68x_dev *dev);

// persistence
void bsec_state_save(uint8_t work_buffer, size_t work_buffer_len);
void bsec_state_load(uint8_t work_buffer, size_t work_buffer_len);

// conversions
size_t bme68x_data_to_bsec_inputs(bsec_bme_settings_t sensor_settings,
					 const struct bme68x_data *data,
					 bsec_input_t *inputs, uint64_t timestamp_ns);
int bsec_apply_sensor_settings(bsec_bme_settings_t sensor_settings);

#if defined(CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_ULTRA_LOW_POWER)
#define BSEC_SAMPLE_RATE BSEC_SAMPLE_RATE_ULP
#define BSEC_SAMPLE_PERIOD_S 300
#elif defined(CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_LOW_POWER)
#define BSEC_SAMPLE_RATE BSEC_SAMPLE_RATE_LP
#define BSEC_SAMPLE_PERIOD_S 3
#elif defined(CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_CONTINUOUS)
#define BSEC_SAMPLE_RATE BSEC_SAMPLE_RATE_CONT
#define BSEC_SAMPLE_PERIOD_S 1
#endif

#define BSEC_SAVE_INTERVAL 600
#define BSEC_SAVE_INTERVAL_S (BSEC_SAVE_INTERVAL * BSEC_SAMPLE_PERIOD_S)
