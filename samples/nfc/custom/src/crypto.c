#include <stdint.h>
#include <stdlib.h>

#define B(x,i) ((x>>i)&1)
#define COMBINE4(a,b,c,d) ((a<<0)|(b<<1)|(c<<2)|(d<<3))
#define COMBINE5(a,b,c,d,e) ((a<<0)|(b<<1)|(c<<2)|(d<<3)|(d<<4))

static inline uint32_t filter_bit(uint64_t x)
{
	const uint32_t f_a = 0x9E98;
	const uint32_t f_b = 0xB48E;
	const uint32_t f_c = 0xEC57E80A;

	return B(f_c,
		COMBINE5(
			B(f_b, COMBINE4(B(x, 9),B(x,11),B(x,13),B(x,15))),
			B(f_a, COMBINE4(B(x,17),B(x,19),B(x,21),B(x,23))),
			B(f_a, COMBINE4(B(x,25),B(x,27),B(x,29),B(x,31))),
			B(f_b, COMBINE4(B(x,33),B(x,35),B(x,37),B(x,39))),
			B(f_a, COMBINE4(B(x,41),B(x,43),B(x,45),B(x,47)))
		)
	);
}

static inline uint32_t lfsr_bit(uint64_t x)
{
	return	B(x,  0) ^ B(x,  5) ^ B(x,  9) ^ B(x, 10) ^
		B(x, 12) ^ B(x, 14) ^ B(x, 15) ^ B(x, 17) ^
		B(x, 19) ^ B(x, 24) ^ B(x, 25) ^ B(x, 27) ^
		B(x, 29) ^ B(x, 35) ^ B(x, 39) ^ B(x, 41) ^
		B(x, 42) ^ B(x, 43);
}

void mutual_0(uint64_t *lfsr, uint64_t key)
{
	*lfsr = 0;
	for (size_t i = 0; i < 6; ++i) {
		*lfsr |= ((key >> 8*i) & 0xFF) << ((5-i)*8);
	}
}

void mutual_1_first(uint64_t *lfsr, uint32_t uid, uint32_t nonce)
{
	uint32_t iv = uid ^ nonce;
	// inject iv without "feedback", MSByte first
	for (size_t i = 3; i >= 0; --i) {
		for (size_t j = 0; j < 8; ++j) {
			*lfsr = ((lfsr_bit(*lfsr) ^ B(iv, i*j)) << 47) | (*lfsr >> 1);
		}
	}
}

void mutual_1_reauth(uint64_t *lfsr, uint32_t uid, uint32_t nonce)
{
	uint32_t iv = uid ^ nonce;
	// inject iv with "feedback", MSByte first
	for (size_t i = 3; i >= 0; --i) {
		for (size_t j = 0; j < 8; ++j) {
			*lfsr = ((lfsr_bit(*lfsr) ^ B(iv, i*j) ^ filter_bit(*lfsr)) << 47) | (*lfsr >> 1);
		}
	}
}

// reader sends [nonce, response] MSByte first, unencrypted
void mutual_2_reader(uint64_t *lfsr, uint32_t nonce, uint8_t *response)
{

}

void mutual_2_card(uint64_t *lfsr, uint32_t nonce, const uint8_t *response)
{

}
