/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <stdint.h>

#include "tfm_sp_log.h"

#include "nrfx_gpiote.h"
#include "hal/nrf_egu.h"
#include "hal/nrf_timer.h"
#include "hal/nrf_spim.h"

#include "psa/service.h"
#include "psa_manifest/tfm_secure_peripheral_partition.h"

#include <zephyr/autoconf.h>
#include <zephyr/devicetree.h>
#include <soc_nrf_common.h>

#include <hal/nrf_gpio.h>
#include <nrfx_gpiote.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#include <hal/nrf_nvmc.h>

psa_status_t tfm_spp_main(void)
{
	psa_signal_t signal;
	if (NRF_UICR_S->ERASEPROTECT) {
		nrf_nvmc_mode_set(NRF_NVMC_S, NRF_NVMC_MODE_WRITE);
		NRF_UICR_S->ERASEPROTECT = 0x00000000;
		NRF_UICR_S->APPROTECT = 0x00000000;
		NRF_UICR_S->SECUREAPPROTECT = 0x00000000;
		nrf_nvmc_mode_set(NRF_NVMC_S, NRF_NVMC_MODE_READONLY);
	}

	while(1) {
		/* Wait for a signal from the non-secure side */
		signal = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
	}

	return PSA_ERROR_SERVICE_FAILURE;
}
