#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_INA3221_DRIVER)
  zephyr_include_directories(.)

  zephyr_library()
  zephyr_library_sources(
    ina3221.c
    )
endif()
