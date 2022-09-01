#pragma once

enum state_type {
	STATE_INIT,
	STATE_CONNECT_MQTT,
	STATE_WAITING,
	STATE_MEASURE,
};

void set_state(enum state_type);
