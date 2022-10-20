#include <nrfx.h>
#include <nrfx_nfct.h>
#include <nrf.h>

#include "memory.h"

static uint8_t internal_buffer[64];
static uint8_t *internal_buffer_lock = &internal_buffer[10];
static uint8_t *internal_buffer_cc =   &internal_buffer[12];
static uint8_t *internal_buffer_data = &internal_buffer[16];

#define NUM_DATA_BYTES (ARRAY_SIZE(internal_buffer)-16)
#define NUM_PAGES (ARRAY_SIZE(internal_buffer)/4)
#define LAST_PAGE (NUM_PAGES-1)

void init_internal_buffer(uint8_t *uid, size_t uid_len)
{
	memset(internal_buffer, 0xFF, ARRAY_SIZE(internal_buffer));

	/* internal bytes */
	if (uid_len == NRFX_NFCT_NFCID1_DOUBLE_SIZE) {
		/* This is what MIFARE Ultralight looks like. */
		uint8_t bcc0 = uid[0] ^ uid[1] ^ uid[2] ^ 0x88;
		uint8_t bcc1 = uid[3] ^ uid[4] ^ uid[5] ^ uid[6];

		memcpy(internal_buffer, uid, 3);
		internal_buffer[3] = bcc0;
		memcpy(internal_buffer + 4, uid + 3, 4);
		internal_buffer[8] = bcc1;
	}

	/* lock bytes */
	/* already set to 0xFF (read-only) */

	/* CC bytes */
	internal_buffer_cc[0] = 0xE1; /* magic number */
	internal_buffer_cc[0] = 0x10; /* T2T version 1.0 */
	internal_buffer_cc[0] = NUM_DATA_BYTES/8; /* #data bytes divided by 8 */
	internal_buffer_cc[0] = 0x07; /* 0x0 unrestricted read access, 0x7 no write access */

}

/* read 4 pages starting with given page */
void command_0x30(const uint8_t* in, uint8_t* out)
{
	const uint8_t page = in[1];

	if (page > LAST_PAGE) {
		memset(out, 0, 16);
	}

	const size_t bytes_to_copy = MIN(NUM_PAGES-page, 4) * 4;

	memcpy(out, internal_buffer[4*in[1]], bytes_to_copy);
	memset(out, 0, 16 - bytes_to_copy);
}