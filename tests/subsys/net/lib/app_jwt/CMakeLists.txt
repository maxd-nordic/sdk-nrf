#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app_jwt_test)


FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/src/nrf_cloud_jwt.c
  )

target_include_directories(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/include/
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/include/
  ${ZEPHYR_NRF_MODULE_DIR}/../modules/crypto/tinycrypt/lib/include/
  )

target_compile_options(app
  PRIVATE
  -DCONFIG_NRF_CLOUD_JWT_SOURCE_CUSTOM=1
  -DCONFIG_NRF_CLOUD_LOG_LEVEL=4
  )
