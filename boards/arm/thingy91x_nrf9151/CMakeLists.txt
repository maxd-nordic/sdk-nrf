# Kconfig - nRF91 Thingy:91 X board configuration
#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
zephyr_library()
if(CONFIG_THINGY91X_STATIC_PARTITIONS_FACTORY)
  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_pm_static.yml CACHE INTERNAL "")
endif()