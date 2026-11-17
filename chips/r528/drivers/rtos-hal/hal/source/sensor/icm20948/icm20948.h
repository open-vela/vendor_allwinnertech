/****************************************************************************
 * vendor/allwinnertech/chips/r528/drivers/rtos-hal/hal/source/sensor/icm20948/icm20948.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __CHIPS_R528_DRIVERS_RTOS_HAL_HAL_SOURCE_SENSOR_ICM20948_ICM20948_H
#define __CHIPS_R528_DRIVERS_RTOS_HAL_HAL_SOURCE_SENSOR_ICM20948_ICM20948_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/spi/spi.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: icm20948_register
 *
 * Description:
 *   Register the ICM20948 sensor driver on the given SPI bus.
 *
 * Input Parameters:
 *   port - SPI bus number the device is attached to
 *   spi  - SPI device instance for that bus
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int icm20948_register(int port, FAR struct spi_dev_s *spi);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __CHIPS_R528_DRIVERS_RTOS_HAL_HAL_SOURCE_SENSOR_ICM20948_ICM20948_H */
