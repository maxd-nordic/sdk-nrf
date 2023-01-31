#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT CONFIG_BSEC_LIB)
	return()
endif()

zephyr_library()

if (CONFIG_FP_SOFTABI)
	set(BSEC_LIB_DIR ${ZEPHYR_BASE}/../modules/lib/bsec2/src/cortex-m4)
	zephyr_compile_definitions(BME68X_DO_NOT_USE_FPU)
endif()
	if (CONFIG_FP_HARDABI)
	set(BSEC_LIB_DIR ${ZEPHYR_BASE}/../modules/lib/bsec2/src/cortex-m33/fpv5-sp-d16-hard)
endif()

if(NOT EXISTS "${BSEC_LIB_DIR}/libalgobsec.a")
	assert(0 "Could not find BSEC library.")
endif()

zephyr_library_sources(
	# bme68x driver
	${ZEPHYR_BASE}/../modules/lib/bme68x/src/bme68x/bme68x.c

	# module code
	./bsec2.c
	./conversions.c
	./persistence.c
	./bus_i2c.c
)
