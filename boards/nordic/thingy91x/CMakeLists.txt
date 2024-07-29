# Kconfig - nRF91 Thingy:91 X board configuration
#
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_WIFI_NRF700X)
  if(CONFIG_BOARD_THINGY91X_NRF9151 OR CONFIG_BOARD_THINGY91X_NRF9151_NS)
    zephyr_library()
    zephyr_library_sources(nrf70_support.c)
  endif()
endif()

if(CONFIG_BOARD_THINGY91X_NRF9151_NS)
  # Use static partition layout to ensure the partition layout remains
  # unchanged after DFU. This needs to be made globally available
  # because it is used in other CMake files.
  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf9151_pm_static.yml CACHE INTERNAL "")
endif()

if(CONFIG_BOARD_THINGY91X_NRF5340_CPUAPP OR CONFIG_BOARD_THINGY91X_NRF5340_CPUAPP_NS)
  if(CONFIG_BOARD_ENABLE_CPUNET)
    zephyr_library_named(nrf5340_cpunet_reset)
    zephyr_library_sources(nrf5340_cpunet_reset.c)
  endif()
  if (CONFIG_BOARD_WAIT_FOR_FLASH)
    zephyr_library_named(nrf5340_wait_for_ext_flash)
    zephyr_library_sources(nrf5340_wait_for_ext_flash.c)
  endif()

  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf5340_pm_static.yml CACHE INTERNAL "")
endif()
