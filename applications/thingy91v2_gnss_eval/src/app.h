#pragma once

enum state_type {
	STATE_INIT,
	STATE_CONNECT_MQTT,
	STATE_WAIT_FOR_COMMAND,
	STATE_MEASURE,
	STATE_MEASURE_WAIT,
};

void set_state(enum state_type);
