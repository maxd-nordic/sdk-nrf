#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

void init_internal_buffer(uint8_t *uid, size_t uid_len);
void command_0x30(const uint8_t* in, uint8_t* out);