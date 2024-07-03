/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <hal/nrf_gpio.h>

#if defined(CONFIG_SOC_SERIES_NRF52X)
#define CPU_CLOCK		64000000U
#else
#define CPU_CLOCK		CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#endif

#if defined(CONFIG_SOC_SERIES_NRF52X) || defined(CONFIG_SOC_SERIES_NRF53X)
#define FAST_BITBANG_HW_SUPPORT 1
#else
#define FAST_BITBANG_HW_SUPPORT 0
#endif

static ALWAYS_INLINE void pin_delay_asm(uint32_t delay)
{
#if defined(CONFIG_CPU_CORTEX_M)
	__asm volatile ("movs r3, %[p]\n"
			".start_%=:\n"
			"subs r3, #1\n"
			"bne .start_%=\n"
			:
			: [p] "r" (delay)
			: "r3", "cc"
			);
#else
#warning "Pin delay is not defined"
#endif
}

static ALWAYS_INLINE void swdp_ll_pin_input(void *const base, uint8_t pin)
{
#if defined(CONFIG_SOC_SERIES_NRF52X) || defined(CONFIG_SOC_SERIES_NRF53X)
	NRF_GPIO_Type * reg = base;

	reg->PIN_CNF[pin] = 0b0000;
#endif
}

static ALWAYS_INLINE void swdp_ll_pin_output(void *const base, uint8_t pin)
{
#if defined(CONFIG_SOC_SERIES_NRF52X) || defined(CONFIG_SOC_SERIES_NRF53X)
	NRF_GPIO_Type * reg = base;

	reg->PIN_CNF[pin] = 0b0001;
#endif
}


static ALWAYS_INLINE void swdp_ll_pin_set(void *const base, uint8_t pin)
{
#if defined(CONFIG_SOC_SERIES_NRF52X) || defined(CONFIG_SOC_SERIES_NRF53X)
	NRF_GPIO_Type * reg = base;

	reg->OUTSET = BIT(pin);
#endif
}

static ALWAYS_INLINE void swdp_ll_pin_clr(void *const base, uint8_t pin)
{
#if defined(CONFIG_SOC_SERIES_NRF52X) || defined(CONFIG_SOC_SERIES_NRF53X)
	NRF_GPIO_Type * reg = base;

	reg->OUTCLR = BIT(pin);
#endif
}

static ALWAYS_INLINE uint32_t swdp_ll_pin_get(void *const base, uint8_t pin)
{
#if defined(CONFIG_SOC_SERIES_NRF52X) || defined(CONFIG_SOC_SERIES_NRF53X)
	NRF_GPIO_Type * reg = base;

	return ((reg->IN >> pin) & 1);
#else
	return 0UL;
#endif
}

void ll_clock_byte(void *const base, uint8_t pin)
{
	for (uint32_t n = 8U; n; n--) {
		swdp_ll_pin_set(base, pin);
		swdp_ll_pin_clr(base, pin);
	}
}

uint8_t ll_read_byte(void *const base, uint8_t pin)
{
	uint8_t byte = 0U;

	for (uint32_t n = 8U; n; n--) {
		byte >>= 1;
		if (swdp_ll_pin_get(base, pin)) {
			byte |= 0x80;
		}
	}

	return byte;
}

void hal_clock_byte(void *const base, uint8_t pin)
{
	for (uint32_t n = 8U; n; n--) {
		nrf_gpio_port_out_set(base, BIT(pin));
		nrf_gpio_port_out_clear(base, BIT(pin));
	}
}

uint8_t hal_read_byte(uint8_t port, uint8_t pin)
{
	uint8_t byte = 0U;

	for (uint32_t n = 8U; n; n--) {
		byte >>= 1;
		if (nrf_gpio_pin_read(NRF_GPIO_PIN_MAP(port, pin))) {
			byte |= 0x80;
		}
	}
	return byte;
}


int main(void)
{
	NRF_GPIO_Type* const gpio_base = NRF_P0_S;
	const uint8_t pin = 0;
	const uint8_t port = 0;

	printf("HAL %d\n", sys_clock_cycle_get_32());
	for (int i = 0; i < 100; i++) {
		nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(port, pin));
		hal_clock_byte(gpio_base, pin);
		nrf_gpio_cfg_input(NRF_GPIO_PIN_MAP(port, pin), NRF_GPIO_PIN_NOPULL);
		hal_read_byte(port, pin);
	}
	printf("LL  %d\n", sys_clock_cycle_get_32());
	for (int i = 0; i < 100; i++) {
		swdp_ll_pin_output(gpio_base, pin);
		ll_clock_byte(gpio_base, pin);
		swdp_ll_pin_input(gpio_base, pin);
		ll_read_byte(gpio_base, pin);
	}
	printf("END %d\n", sys_clock_cycle_get_32());

	return 0;
}
