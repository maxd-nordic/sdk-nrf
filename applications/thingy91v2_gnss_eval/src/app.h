#pragma once

#if CONFIG_APP_LOW_POWER_MODE
#define STATE_INIT_COLOR		0b000
#define STATE_CONNECT_MQTT_COLOR	0b000
#define STATE_WAIT_FOR_COMMAND_COLOR	0b000
#define STATE_SEND_RESULTS_COLOR	0b000
#define STATE_COUNTDOWN_COLOR		0b000
#define STATE_MEASURE_COLOR		0b000
#define STATE_MEASURE_WAIT_COLOR	0b000
#define STATE_GOT_FIX_COLOR		0b000
#else
#define STATE_INIT_COLOR		0b111 // white
#define STATE_CONNECT_MQTT_COLOR	0b110 // cyan
#define STATE_WAIT_FOR_COMMAND_COLOR	0b010 // green
#define STATE_SEND_RESULTS_COLOR	0b100 // blue
#define STATE_COUNTDOWN_COLOR		0b011 // yellow
#define STATE_MEASURE_COLOR		0b011 // yellow
#define STATE_MEASURE_WAIT_COLOR	0b001 // red
#define STATE_GOT_FIX_COLOR		0b101 // purple
#endif
#include <dk_buttons_and_leds.h>

enum state_type {
	STATE_INIT,
	STATE_CONNECT_MQTT,
	STATE_WAIT_FOR_COMMAND,
	STATE_MEASURE,
	STATE_MEASURE_WAIT,
};

struct connection_data_type
{
	int64_t last_measure_ms;
	uint16_t band;
	uint16_t rsrp;
	char cellid[10];
	uint16_t vbatt;
};


void set_state(enum state_type);
