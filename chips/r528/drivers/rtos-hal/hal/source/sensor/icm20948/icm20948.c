/****************************************************************************
 * drivers/sensors/icm20948.c
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

#include <nuttx/config.h>

#ifdef CONFIG_SENSORS_ICM20948

#include <debug.h>
#include <errno.h>
#include <nuttx/arch.h>
#include <nuttx/clock.h>
#include <nuttx/compiler.h>
#include <nuttx/fs/fs.h>
#include <nuttx/kmalloc.h>
#include <nuttx/sensors/sensor.h>
#include <nuttx/wqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <uORB/uORB.h>

#include "sunxi_hal_spi.h"

#ifndef container_of
#define container_of(ptr, type, member)                                        \
  ((type *)(((char *)(ptr)) - offsetof(type, member)))
#endif

#define ICM20948_BANK0					0x00
#define ICM20948_BANK1					0x01
#define ICM20948_BANK2					0x02
#define ICM20948_BANK3					0x03

#define ICM20948_ID_VALUE				0xEA

#define ICM20948_WHO_AM_I				0x00
#define ICM20948_USER_CTRL				0x03
#define ICM20948_PWR_MGMT_1				0x06
#define ICM20948_REG_BANK_SEL			0x7F
#define ICM20948_EXT_SLV_SENS_DATA_00	0x3B

#define ICM20948_GYRO_CONFIG_1			0x01
#define ICM20948_ACCEL_CONFIG			0x14
#define ICM20948_INT_PIN_CFG			0x0F
#define ICM20948_INT_ENABLE				0x10
#define ICM20948_INT_ENABLE_1			0x11
#define ICM20948_INT_ENABLE_2			0x12
#define ICM20948_INT_ENABLE_3			0x13
#define ICM20948_INT_STATUS				0x19
#define ICM20948_INT_STATUS_1			0x1A
#define ICM20948_INT_STATUS_2			0x1B
#define ICM20948_INT_STATUS_3			0x1C
#define ICM20948_ACCEL_XOUT_H			0x2D
#define ICM20948_GYRO_XOUT_H			0x33
#define ICM20948_TEMP_XOUT_H			0x39

#define ICM20948_I2C_MST_ODR_CFG		0x00
#define ICM20948_I2C_MST_CTRL			0x01
#define ICM20948_I2C_MST_DELAY_CTRL		0x02
#define ICM20948_I2C_SLV0_ADDR			0x03
#define ICM20948_I2C_SLV0_REG			0x04
#define ICM20948_I2C_SLV0_CTRL			0x05
#define ICM20948_I2C_SLV0_DO			0x06
#define ICM20948_I2C_SLV1_ADDR			0x07
#define ICM20948_I2C_SLV1_REG			0x08
#define ICM20948_I2C_SLV1_CTRL			0x09
#define ICM20948_I2C_SLV1_DO			0x0A
#define ICM20948_I2C_SLV4_CTRL			0x15

#define AK09916_ID_VALUE				0x09

#define AK09916_ADDRESS					0x0C
#define AK09916_WHO_AM_I				0x01
#define AK09916_STATUS_1				0x10
#define AK09916_MGTR_XOUTL				0x11
#define AK09916_MGTR_XOUTH				0x12
#define AK09916_MGTR_YOUTL				0x13
#define AK09916_MGTR_YOUTH				0x14
#define AK09916_MGTR_ZOUTL				0x15
#define AK09916_MGTR_ZOUTH				0x16
#define AK09916_STATUS_2				0x18
#define AK09916_CNTL_2					0x31
#define AK09916_CNTL_3					0x32

#define ICM20948_DEFAULT_INTERVAL_US	1000000
#define ICM20948_MIN_INTERVAL_US		50000
#define ICM20948_MAX_INTERVAL_US		10000000

typedef enum {
	ACCE_FS_2G = 0,
	ACCE_FS_4G = 1,
	ACCE_FS_8G = 2,
	ACCE_FS_16G = 3,
} icm20948_acce_fs_t;

typedef enum {
	GYRO_FS_250DPS = 0,
	GYRO_FS_500DPS = 1,
	GYRO_FS_1000DPS = 2,
	GYRO_FS_2000DPS = 3,
} icm20948_gyro_fs_t;

enum icm20948_idx_e
{
  ICM20948_SENSOR_ACCE = 0,
  ICM20948_SENSOR_GYRO,
  ICM20948_SENSOR_MAGN,
  ICM20948_SENSOR_MAX
};

typedef struct {
	int16_t raw_acce_x;
	int16_t raw_acce_y;
	int16_t raw_acce_z;
} icm20948_raw_acce_value_t;

typedef struct {
	int16_t raw_gyro_x;
	int16_t raw_gyro_y;
	int16_t raw_gyro_z;
} icm20948_raw_gyro_value_t;

typedef struct {
	float acce_x;
	float acce_y;
	float acce_z;
} icm20948_acce_value_t;

typedef struct {
	float gyro_x;
	float gyro_y;
	float gyro_z;
} icm20948_gyro_value_t;

typedef struct {
	float magn_x;
	float magn_y;
	float magn_z;
} icm20948_magn_value_t;

struct icm20948_data_s {
	icm20948_acce_value_t	acce;
	icm20948_gyro_value_t	gyro;
	icm20948_magn_value_t	magn;
};

static int icm20948_open(FAR struct file *filep);
static int icm20948_close(FAR struct file *filep);
static ssize_t icm20948_read(FAR struct file *filep, FAR char *buffer, size_t len);
static int icm20948_ioctl(FAR struct file *filep, int cmd, unsigned long arg);

struct icm20948_sensor_s {

  struct sensor_lowerhalf_s	lower;
  bool						enabled;

  FAR struct icm20948_dev_s *dev;
  uint8_t					type;
};

static const struct file_operations g_icm20948fops __attribute__((unused)) = {
    icm20948_open,
    icm20948_close,
    icm20948_read,
    NULL,
    NULL,
    icm20948_ioctl,
};

struct icm20948_dev_s {

  FAR struct icm20948_sensor_s	sensors[ICM20948_SENSOR_MAX];

  FAR struct spi_dev_s			*spi;
  int							port;

  struct work_s					acce_work;
  struct work_s					gyro_work;
  struct work_s					magn_work;

  bool							acce_work_started;
  bool							gyro_work_started;
  bool							magn_work_started;

  uint32_t						acce_interval;
  uint32_t						gyro_interval;
  uint32_t						magn_interval;

  bool							acce_enabled;
  bool							gyro_enabled;
  bool							magn_enabled;
};

static int icm20948_set_interval(FAR struct sensor_lowerhalf_s *lower,
                              FAR struct file *filep, FAR uint32_t *period_us);
static int icm20948_fetch(FAR struct sensor_lowerhalf_s *lower,
                       FAR struct file *filep, FAR char *buffer, size_t buflen);
static int icm20948_acce_activate(FAR struct sensor_lowerhalf_s *lower,
                               FAR struct file *filep, bool enable);
static int icm20948_gyro_activate(FAR struct sensor_lowerhalf_s *lower,
                               FAR struct file *filep, bool enable);
static int icm20948_magn_activate(FAR struct sensor_lowerhalf_s *lower,
                               FAR struct file *filep, bool enable);
static void icm20948_acce_worker(FAR void *arg);
static void icm20948_gyro_worker(FAR void *arg);
static void icm20948_magn_worker(FAR void *arg);

static int icm20948_spi_exchange(hal_spi_master_port_t port,
                               const void *txbuffer,
                               void *rxbuffer,
                               uint32_t nwords);
static uint8_t icm20948_write_reg(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t value);
static uint8_t icm20948_write_regs(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t *value, uint8_t len);
static uint8_t icm20948_read_reg(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t *value);
static uint8_t icm20948_read_regs(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t *value, uint8_t len);

static uint8_t icm20948_init(FAR struct icm20948_dev_s *priv);
static uint8_t icm20948_wakeup(FAR struct icm20948_dev_s *priv);
static uint8_t icm20948_sleep(FAR struct icm20948_dev_s *priv);
static uint8_t icm20948_set_bank(FAR struct icm20948_dev_s *priv, uint8_t bank);
static uint8_t icm20948_get_deviceid(FAR struct icm20948_dev_s *priv, uint8_t *id);

static uint8_t icm20948_set_gyro_fs(FAR struct icm20948_dev_s *priv, icm20948_gyro_fs_t gyro_fs);
static uint8_t icm20948_get_gyro_fs(FAR struct icm20948_dev_s *priv, icm20948_gyro_fs_t *gyro_fs);
static uint8_t icm20948_get_gyro_sensitivity(FAR struct icm20948_dev_s *priv, float *const gyro_sensitivity);
static uint8_t icm20948_get_raw_gyro(FAR struct icm20948_dev_s *priv, icm20948_raw_gyro_value_t *raw_gyro_value);
static uint8_t icm20948_get_gyro(FAR struct icm20948_dev_s *priv, icm20948_gyro_value_t *gyro_value);

static uint8_t icm20948_set_acce_fs(FAR struct icm20948_dev_s *priv, icm20948_acce_fs_t acce_fs);
static uint8_t icm20948_get_acce_fs(FAR struct icm20948_dev_s *priv, icm20948_acce_fs_t *acce_fs);
static uint8_t icm20948_get_acce_sensitivity(FAR struct icm20948_dev_s *priv, float *const acce_sensitivity);
static uint8_t icm20948_get_raw_acce(FAR struct icm20948_dev_s *priv, icm20948_raw_acce_value_t *raw_acce_value);
static uint8_t icm20948_get_acce(FAR struct icm20948_dev_s *priv, icm20948_acce_value_t *acce_value);

static uint8_t icm20948_get_magn(FAR struct icm20948_dev_s *priv, icm20948_magn_value_t *magn_value);

static uint8_t icm20948_get_temperature(FAR struct icm20948_dev_s *priv, float *temperature);

static uint8_t ak09916_write_reg(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t value);
static uint8_t ak09916_read_reg(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t *value);
static uint8_t ak09916_read_regs(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t *value, uint8_t len);
static uint8_t ak09916_init(FAR struct icm20948_dev_s *priv);

static const struct sensor_ops_s g_icm20948_aac_ops = {
    .activate = icm20948_acce_activate,
    .set_interval = icm20948_set_interval,
    .batch = NULL,
    .fetch = icm20948_fetch,
    .control = NULL,
};

static const struct sensor_ops_s g_icm20948_gyr_ops = {
    .activate = icm20948_gyro_activate,
    .set_interval = icm20948_set_interval,
    .batch = NULL,
    .fetch = icm20948_fetch,
    .control = NULL,
};

static const struct sensor_ops_s g_icm20948_mag_ops = {
    .activate = icm20948_magn_activate,
    .set_interval = icm20948_set_interval,
    .batch = NULL,
    .fetch = icm20948_fetch,
    .control = NULL,
};

static uint64_t g_acce_last_enable_time = 0;
static uint64_t g_gyro_last_enable_time = 0;
static uint64_t g_magn_last_enable_time = 0;

static int icm20948_open(FAR struct file *filep) {
	FAR struct inode *inode = filep->f_inode;
	FAR struct icm20948_dev_s *priv = inode->i_private;
	uint8_t	ret;

	ret = icm20948_wakeup(priv);
	if (ret) {
		syslog(LOG_INFO, "icm20948_wakeup error.\n");
		return -1;
	}
	return OK;
}

static int icm20948_close(FAR struct file *filep) {
	FAR struct inode *inode = filep->f_inode;
	FAR struct icm20948_dev_s *priv = inode->i_private;
	uint8_t	ret;

	ret = icm20948_sleep(priv);
	if (ret) {
		syslog(LOG_INFO, "icm20948_sleep error.\n");
		return -1;
	}
	return OK;
}

static ssize_t icm20948_read(FAR struct file *filep, FAR char *buffer,
                          size_t len) {
	FAR struct inode *inode = filep->f_inode;
	FAR struct icm20948_dev_s *priv = inode->i_private;
	FAR struct icm20948_data_s *data = (FAR struct icm20948_data_s *)buffer;
	int ret;

	if (len < sizeof(struct icm20948_data_s)) {
		syslog(LOG_ERR, "Buffer size is insufficient, requires %d bytes\n",
			sizeof(struct icm20948_data_s));
		return -EINVAL;
	}

	icm20948_acce_value_t	acce;
	icm20948_gyro_value_t	gyro;
	icm20948_magn_value_t	magn;

	ret = icm20948_get_acce(priv, &acce);
	if (ret)
	{
		syslog(LOG_INFO, "icm20948_get_acce error.\n");
		return ret;
	}

	ret = icm20948_get_gyro(priv, &gyro);
	if (ret)
	{
		syslog(LOG_INFO, "icm20948_get_gyro error.\n");
		return ret;
	}

	ret = icm20948_get_magn(priv, &magn);
	if (ret)
	{
		syslog(LOG_INFO, "icm20948_get_magn error.\n");
		return ret;
	}

	memcpy(&data->acce, &acce, sizeof(acce));
	memcpy(&data->gyro, &gyro, sizeof(gyro));
	memcpy(&data->magn, &magn, sizeof(magn));

	return sizeof(struct icm20948_data_s);
}

static int icm20948_ioctl(FAR struct file *filep, int cmd, unsigned long arg) {
	FAR struct inode *inode = filep->f_inode;
	FAR struct icm20948_dev_s *priv __attribute__((unused)) = inode->i_private;
	int ret = OK;

	switch (cmd) {
		default:
			ret = -ENOTTY;
			break;
	}

	return ret;
}

static int icm20948_spi_exchange(hal_spi_master_port_t port,
                               const void *txbuffer,
                               void *rxbuffer,
                               uint32_t nwords)
{
	hal_spi_master_transfer_t tr;

	memset(&tr, 0, sizeof(tr));

	tr.tx_buf = (uint8_t *)txbuffer;
	tr.rx_buf = rxbuffer;
	tr.tx_len = nwords;
	tr.rx_len = nwords;
    tr.rx_nbits = SPI_NBITS_SINGLE;
    tr.tx_nbits = SPI_NBITS_SINGLE;
	tr.tx_single_len = nwords;
	tr.dummy_byte = 0;

	return hal_spi_xfer(port, &tr);
}

static uint8_t icm20948_write_reg(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t value) {
	uint8_t	tx[2];
	uint8_t	rx[2] = {0};

    tx[0] = 0x7F & reg ;

    tx[1] = value;

	return icm20948_spi_exchange(priv->port, tx, rx, 2);
}

static uint8_t icm20948_write_regs(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t *value, uint8_t len) {
	uint8_t	tx[len + 1];
	uint8_t	rx[len + 1];

    tx[0] = 0x7F & reg ;

    memcpy(&tx[1], value, len);

	return icm20948_spi_exchange(priv->port, tx, rx, len + 1);
}

static uint8_t icm20948_read_reg(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t *value) {
	uint8_t	tx[2];
	uint8_t	rx[2] = {0};

    tx[0] = 0x80 | reg;

    tx[1] = 0x00;

	int	ret = icm20948_spi_exchange(priv->port, tx, rx, 2);

	*value = rx[1];

	return ret;
}

static uint8_t icm20948_read_regs(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t *value, uint8_t len) {
	uint8_t	tx[len + 1];
	uint8_t	rx[len + 1];

	memset(rx, 0, sizeof(rx));

    tx[0] = 0x80 | reg;

    memset(&tx[1], 0x00, len);

	int	ret = icm20948_spi_exchange(priv->port, tx, rx, len + 1);

	memcpy(value, &rx[1], len);

	return ret;
}

static uint8_t icm20948_reset(FAR struct icm20948_dev_s *priv) {
	uint8_t	ret;
	uint8_t tmp;

	ret = icm20948_read_reg(priv, ICM20948_PWR_MGMT_1, &tmp);
	if (ret) {
		return -1;
	}
	tmp |= 0x80;
	ret = icm20948_write_reg(priv, ICM20948_PWR_MGMT_1, tmp);
	if (ret) {
		return -1;
	}
	return OK;
}

static uint8_t icm20948_wakeup(FAR struct icm20948_dev_s *priv) {
	uint8_t	ret;
	uint8_t tmp;

	ret = icm20948_read_reg(priv, ICM20948_PWR_MGMT_1, &tmp);
	if (ret) {
		return -1;
	}
	tmp &= (~0x40);
	ret = icm20948_write_reg(priv, ICM20948_PWR_MGMT_1, tmp);
	if (ret) {
		return -1;
	}

	return OK;
}

static uint8_t icm20948_sleep(FAR struct icm20948_dev_s *priv) {
	uint8_t	ret;
	uint8_t tmp;

	ret = icm20948_read_reg(priv, ICM20948_PWR_MGMT_1, &tmp);
	if (ret) {
		return -1;
	}
	tmp |= 0x40;
	ret = icm20948_write_reg(priv, ICM20948_PWR_MGMT_1, tmp);
	if (ret) {
		return -1;
	}
	return OK;
}

static uint8_t icm20948_set_bank(FAR struct icm20948_dev_s *priv, uint8_t bank) {
	uint8_t	ret;

	if (bank > 3)
		return -1;
	bank = (bank << 4) & 0x30;
	ret = icm20948_write_reg(priv, ICM20948_REG_BANK_SEL, bank);
	if (ret) {
		return -1;
	}
	return OK;
}

static uint8_t icm20948_get_deviceid(FAR struct icm20948_dev_s *priv, uint8_t *id) {
	uint8_t	ret;

	ret = icm20948_read_reg(priv, ICM20948_WHO_AM_I, id);
	if (ret) {
		return -1;
	}
	return OK;
}

static uint8_t icm20948_set_gyro_fs(FAR struct icm20948_dev_s *priv, icm20948_gyro_fs_t gyro_fs)
{
	uint8_t ret;
	uint8_t tmp;

	ret = icm20948_set_bank(priv, ICM20948_BANK2);
	if (ret) {
		return -1;
	}

	ret = icm20948_read_reg(priv, ICM20948_GYRO_CONFIG_1, &tmp);
	if (ret) {
		return -1;
	}

	tmp &= 0x09;
	tmp |= (gyro_fs << 1);

	ret = icm20948_write_reg(priv, ICM20948_GYRO_CONFIG_1, tmp);
	if (ret) {
		return -1;
	}
	return OK;
}

static uint8_t icm20948_get_gyro_fs(FAR struct icm20948_dev_s *priv, icm20948_gyro_fs_t *gyro_fs)
{
	uint8_t	ret;
	uint8_t tmp;

	ret = icm20948_set_bank(priv, ICM20948_BANK2);
	if (ret) {
		return -1;
	}

	ret = icm20948_read_reg(priv, ICM20948_GYRO_CONFIG_1, &tmp);
	if (ret) {
		return -1;
	}

	tmp &= 0x06;
	tmp >>= 1;
	*gyro_fs = tmp;
	return OK;
}

static uint8_t icm20948_get_gyro_sensitivity(FAR struct icm20948_dev_s *priv, float *const gyro_sensitivity)
{
	uint8_t				ret;
	icm20948_gyro_fs_t	gyro_fs;
	ret = icm20948_get_gyro_fs(priv, &gyro_fs);
	if (ret) {
		return -1;
	}

	ret = OK;

	switch (gyro_fs) {
	case GYRO_FS_250DPS:
		*gyro_sensitivity = 131;
		break;

	case GYRO_FS_500DPS:
		*gyro_sensitivity = 65.5;
		break;

	case GYRO_FS_1000DPS:
		*gyro_sensitivity = 32.8;
		break;

	case GYRO_FS_2000DPS:
		*gyro_sensitivity = 16.4;
		break;

	default:
		ret = -1;
		break;
	}
	return ret;
}

static uint8_t icm20948_get_raw_gyro(FAR struct icm20948_dev_s *priv, icm20948_raw_gyro_value_t *raw_gyro_value)
{
	uint8_t	data_rd[6];
	uint8_t ret = icm20948_read_regs(priv, ICM20948_GYRO_XOUT_H, data_rd, sizeof(data_rd));
	if (ret) {
		return -1;
	}

	raw_gyro_value->raw_gyro_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
	raw_gyro_value->raw_gyro_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
	raw_gyro_value->raw_gyro_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));

	return OK;
}

static uint8_t icm20948_get_gyro(FAR struct icm20948_dev_s *priv, icm20948_gyro_value_t *gyro_value)
{
	uint8_t						ret;
	float						gyro_sensitivity;
	icm20948_raw_gyro_value_t	raw_gyro;

	ret = icm20948_get_gyro_sensitivity(priv, &gyro_sensitivity);
	if (ret) {
		return -1;
	}

	ret = icm20948_set_bank(priv, ICM20948_BANK0);
	if (ret) {
		return -1;
	}

	ret = icm20948_get_raw_gyro(priv, &raw_gyro);
	if (ret) {
		return -1;
	}

	gyro_value->gyro_x = raw_gyro.raw_gyro_x / gyro_sensitivity;
	gyro_value->gyro_y = raw_gyro.raw_gyro_y / gyro_sensitivity;
	gyro_value->gyro_z = raw_gyro.raw_gyro_z / gyro_sensitivity;

	return OK;
}

static uint8_t icm20948_set_acce_fs(FAR struct icm20948_dev_s *priv, icm20948_acce_fs_t acce_fs)
{
	uint8_t	ret;
	uint8_t tmp;

	ret = icm20948_set_bank(priv, ICM20948_BANK2);
	if (ret) {
		return -1;
	}

	ret = icm20948_read_reg(priv, ICM20948_ACCEL_CONFIG, &tmp);
	if (ret) {
		return -1;
	}

	tmp &= 0x09;
	tmp |= (acce_fs << 1);

	ret = icm20948_write_reg(priv, ICM20948_ACCEL_CONFIG, tmp);
	if (ret) {
		return -1;
	}
	return OK;
}

static uint8_t icm20948_get_acce_fs(FAR struct icm20948_dev_s *priv, icm20948_acce_fs_t *acce_fs)
{
	uint8_t	ret;
	uint8_t tmp;

	ret = icm20948_set_bank(priv, ICM20948_BANK2);
	if (ret) {
		return -1;
	}

	ret = icm20948_read_reg(priv, ICM20948_ACCEL_CONFIG, &tmp);
	if (ret) {
		return -1;
	}

	tmp &= 0x06;
	tmp >>= 1;
	*acce_fs = tmp;
	return OK;
}

static uint8_t icm20948_get_acce_sensitivity(FAR struct icm20948_dev_s *priv, float *const acce_sensitivity)
{
	uint8_t				ret;
	icm20948_acce_fs_t	acce_fs;
	ret = icm20948_get_acce_fs(priv, &acce_fs);
	if (ret) {
		return -1;
	}

	ret = OK;

	switch (acce_fs) {
	case ACCE_FS_2G:
		*acce_sensitivity = 16384;
		break;

	case ACCE_FS_4G:
		*acce_sensitivity = 8192;
		break;

	case ACCE_FS_8G:
		*acce_sensitivity = 4096;
		break;

	case ACCE_FS_16G:
		*acce_sensitivity = 2048;
		break;

	default:
		ret = -1;
		break;
	}
	return ret;
}

static uint8_t icm20948_get_raw_acce(FAR struct icm20948_dev_s *priv, icm20948_raw_acce_value_t *raw_acce_value)
{
	uint8_t data_rd[6];
	uint8_t ret = icm20948_read_regs(priv, ICM20948_ACCEL_XOUT_H, data_rd, sizeof(data_rd));

	if (ret) {
		return -1;
	}

	raw_acce_value->raw_acce_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
	raw_acce_value->raw_acce_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
	raw_acce_value->raw_acce_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));

	return OK;
}

static uint8_t icm20948_get_acce(FAR struct icm20948_dev_s *priv, icm20948_acce_value_t *acce_value)
{
	uint8_t						ret;
	float						acce_sensitivity;
	icm20948_raw_acce_value_t	raw_acce;

	ret = icm20948_get_acce_sensitivity(priv, &acce_sensitivity);
	if (ret) {
		return -1;
	}

	ret = icm20948_set_bank(priv, ICM20948_BANK0);
	if (ret) {
		return -1;
	}

	ret = icm20948_get_raw_acce(priv, &raw_acce);
	if (ret) {
		return -1;
	}

	acce_value->acce_x = raw_acce.raw_acce_x / acce_sensitivity;
	acce_value->acce_y = raw_acce.raw_acce_y / acce_sensitivity;
	acce_value->acce_z = raw_acce.raw_acce_z / acce_sensitivity;
	return OK;
}

static uint8_t icm20948_get_magn(FAR struct icm20948_dev_s *priv, icm20948_magn_value_t *magn_value)
{
	uint8_t	ret;
	uint8_t	data_rd[9] = {0};

	ret = ak09916_read_regs(priv, AK09916_STATUS_1, data_rd, sizeof(data_rd));
	if (ret) {
		return -1;
	}

	if(data_rd[8] & 0x08) {
		return -1;
	}

	magn_value->magn_x = 0.15f * (float)(int16_t)((data_rd[2] << 8) | data_rd[1]);
	magn_value->magn_y = 0.15f * (float)(int16_t)((data_rd[4] << 8) | data_rd[3]);
	magn_value->magn_z = 0.15f * (float)(int16_t)((data_rd[6] << 8) | data_rd[5]);

	return OK;
}

static uint8_t icm20948_get_temperature(FAR struct icm20948_dev_s *priv, float *temperature)
{
	uint8_t	ret;
	uint8_t	data_rd[2];
    int16_t raw;

	ret = icm20948_set_bank(priv, ICM20948_BANK0);
	if (ret) {
		return -1;
	}

	ret = icm20948_read_regs(priv, ICM20948_TEMP_XOUT_H, data_rd, sizeof(data_rd));
	if (ret) {
		return -1;
	}

    raw = ((int16_t)data_rd[0] << 8) | data_rd[1];
    *temperature = 21.0f + ((float)raw) / 333.87f;

	return OK;
}

static uint8_t icm20948_init(FAR struct icm20948_dev_s *priv) {
	uint8_t	ret;
	uint8_t id;

	ret = icm20948_reset(priv);
	if (ret) {
		syslog(LOG_INFO, "icm20948_reset error.\n");
		return -1;
	}

	up_mdelay(100);

	ret = icm20948_wakeup(priv);
	if (ret) {
		syslog(LOG_INFO, "icm20948_wakeup error.\n");
		return -1;
	}

	ret = icm20948_set_bank(priv, ICM20948_BANK0);
	if (ret) {
		syslog(LOG_INFO, "icm20948_set_bank error.\n");
		return -1;
	}

	ret = icm20948_get_deviceid(priv, &id);
	if (ret) {
		syslog(LOG_INFO, "icm20948_get_deviceid error.\n");
		return -1;
	}

	syslog(LOG_INFO, "===============ICM20948 ID: 0x%02X===============\n", id);

	if (id != ICM20948_ID_VALUE) {
		syslog(LOG_INFO, "Wrong Device ID! 0x%02X\n", id);
		return -1;
	}

	ret = icm20948_set_gyro_fs(priv, GYRO_FS_1000DPS);
	if (ret) {
		syslog(LOG_INFO, "icm20948_set_gyro_fs error.\n");
		return -1;
	}

	ret = icm20948_set_acce_fs(priv, ACCE_FS_2G);
	if (ret) {
		syslog(LOG_INFO, "icm20948_set_acce_fs error.\n");
		return -1;
	}

	return OK;
}

static int icm20948_set_interval(FAR struct sensor_lowerhalf_s *lower,
                              FAR struct file *filep, FAR uint32_t *period_us)
{
	FAR struct icm20948_sensor_s *sensor =
	  container_of(lower, struct icm20948_sensor_s, lower);
	FAR struct icm20948_dev_s *priv = sensor->dev;
	uint32_t interval = *period_us;

	sninfo("ICM20948: Set interval called for sensor type %d, interval=%lu us\n",
		 sensor->type, interval);

	if (interval < ICM20948_MIN_INTERVAL_US) {
		sninfo("ICM20948: Interval too small, setting to minimum %d us\n",
			   ICM20948_MIN_INTERVAL_US);
		interval = ICM20948_MIN_INTERVAL_US;
	} else if (interval > ICM20948_MAX_INTERVAL_US) {
		sninfo("ICM20948: Interval too large, setting to maximum %d us\n",
			   ICM20948_MAX_INTERVAL_US);
		interval = ICM20948_MAX_INTERVAL_US;
	}

	if (sensor->type == ICM20948_SENSOR_ACCE) {
		priv->acce_interval = interval;
		sninfo("ICM20948: Accelerometer sensor interval set to %lu us\n", interval);
	} else if (sensor->type == ICM20948_SENSOR_GYRO) {
		priv->gyro_interval = interval;
		sninfo("ICM20948: Gyroscope sensor interval set to %lu us\n", interval);
	} else if (sensor->type == ICM20948_SENSOR_MAGN) {
		priv->magn_interval = interval;
		sninfo("ICM20948: Magnetometer sensor interval set to %lu us\n", interval);
	} else {
		sninfo("ICM20948: Unknown sensor type: %d\n", sensor->type);
		return -EINVAL;
	}

	*period_us = interval;

	return OK;
}

static int icm20948_fetch(FAR struct sensor_lowerhalf_s *lower,
                       FAR struct file *filep, FAR char *buffer, size_t buflen)

{
	FAR struct icm20948_sensor_s *sensor =
		container_of(lower, struct icm20948_sensor_s, lower);
	FAR struct icm20948_dev_s *priv = sensor->dev;
	int			ret;
	uint64_t	timestamp;
	int			len;

	if (!buffer || buflen == 0) {
		return -EINVAL;
	}

	float	temperature;
	ret = icm20948_get_temperature(priv, &temperature);
	if (ret) {
		return -1;
	}

	timestamp = sensor_get_timestamp();

	if (sensor->type == ICM20948_SENSOR_ACCE) {
		len = sizeof(struct sensor_accel);
		if (buflen < len) {
			return -ENOMEM;
		}

		icm20948_acce_value_t acce;
		ret = icm20948_get_acce(priv, &acce);
		if (ret) {
			return -1;
		}

		struct sensor_accel *acce_data = (struct sensor_accel *)buffer;
		acce_data->timestamp = timestamp;
		acce_data->x = acce.acce_x;
		acce_data->y = acce.acce_y;
		acce_data->z = acce.acce_z;
		acce_data->z = acce.acce_z;
		acce_data->temperature = temperature;
		acce_data->status = 0;
	} else if (sensor->type == ICM20948_SENSOR_GYRO){
		len = sizeof(struct sensor_gyro);
		if (buflen < len) {
			return -ENOMEM;
		}

		icm20948_gyro_value_t gyro;
		ret = icm20948_get_gyro(priv, &gyro);
		if (ret) {
			return -1;
		}

		struct sensor_gyro *gyro_data = (struct sensor_gyro *)buffer;
		gyro_data->timestamp = timestamp;
		gyro_data->x = gyro.gyro_x;
		gyro_data->y = gyro.gyro_y;
		gyro_data->z = gyro.gyro_z;
		gyro_data->temperature = temperature;
		gyro_data->status = 0;
	} else {
		len = sizeof(struct sensor_mag);
		if (buflen < len) {
			return -ENOMEM;
		}

		icm20948_magn_value_t magn;
		ret = icm20948_get_magn(priv, &magn);
		if (ret) {
			return -1;
		}

		struct sensor_mag *magn_data = (struct sensor_mag *)buffer;
		magn_data->timestamp = timestamp;
		magn_data->x = magn.magn_x;
		magn_data->y = magn.magn_y;
		magn_data->z = magn.magn_z;
		magn_data->temperature = temperature;
		magn_data->status = 0;
	}
	return len;
}

static int icm20948_acce_activate(FAR struct sensor_lowerhalf_s *lower,
                               FAR struct file *filep, bool enable)
{
	FAR struct icm20948_sensor_s *sensor =
		container_of(lower, struct icm20948_sensor_s, lower);
	FAR struct icm20948_dev_s *priv = sensor->dev;

	if (enable) {
		if (priv->acce_enabled) {
			return OK;
		}

		int ret = icm20948_wakeup(priv);
		priv->acce_enabled = true;

		if (!priv->acce_work_started) {
			priv->acce_work_started = true;
			ret = work_queue(HPWORK, &priv->acce_work, icm20948_acce_worker, priv, 0);
			if (ret < 0) {
				priv->acce_work_started = false;
				priv->acce_enabled = false;
				return ret;
			}
		}
		g_acce_last_enable_time = sensor_get_timestamp();
	}
	else
	{
		uint64_t current_time = sensor_get_timestamp();

		if (g_acce_last_enable_time > 0 &&
			(current_time - g_acce_last_enable_time) < 500000) {
		  sninfo("ICM20948: Ignoring early disable (time: %llu us)\n",
				 current_time - g_acce_last_enable_time);
			return OK;
		}
		priv->acce_enabled = false;
		priv->acce_work_started = false;
		work_cancel(HPWORK, &priv->acce_work);
	}
	return OK;
}

static int icm20948_gyro_activate(FAR struct sensor_lowerhalf_s *lower,
                               FAR struct file *filep, bool enable)
{
	FAR struct icm20948_sensor_s *sensor =
		container_of(lower, struct icm20948_sensor_s, lower);
	FAR struct icm20948_dev_s *priv = sensor->dev;

	if (enable) {
		if (priv->gyro_enabled) {
			return OK;
		}

		int ret = icm20948_wakeup(priv);
		priv->gyro_enabled = true;

		if (!priv->gyro_work_started) {
			priv->gyro_work_started = true;
			ret = work_queue(HPWORK, &priv->gyro_work, icm20948_gyro_worker, priv, 0);
			if (ret < 0) {
				priv->gyro_work_started = false;
				priv->gyro_enabled = false;
				return ret;
			}
		}
		g_gyro_last_enable_time = sensor_get_timestamp();
	}
	else
	{
		uint64_t current_time = sensor_get_timestamp();

		if (g_gyro_last_enable_time > 0 &&
			(current_time - g_gyro_last_enable_time) < 500000) {
		  sninfo("ICM20948: Ignoring early disable (time: %llu us)\n",
				 current_time - g_gyro_last_enable_time);
			return OK;
		}
		priv->gyro_enabled = false;
		priv->gyro_work_started = false;
		work_cancel(HPWORK, &priv->gyro_work);
	}
	return OK;
}

static int icm20948_magn_activate(FAR struct sensor_lowerhalf_s *lower,
                               FAR struct file *filep, bool enable)
{
	FAR struct icm20948_sensor_s *sensor =
		container_of(lower, struct icm20948_sensor_s, lower);
	FAR struct icm20948_dev_s *priv = sensor->dev;

	if (enable) {
		if (priv->magn_enabled) {
			return OK;
		}

		int ret = icm20948_wakeup(priv);
		priv->magn_enabled = true;

		if (!priv->magn_work_started) {
			priv->magn_work_started = true;
			ret = work_queue(HPWORK, &priv->magn_work, icm20948_magn_worker, priv, 0);
			if (ret < 0) {
				priv->magn_work_started = false;
				priv->magn_enabled = false;
				return ret;
			}
		}
		g_magn_last_enable_time = sensor_get_timestamp();
	}
	else
	{
		uint64_t current_time = sensor_get_timestamp();

		if (g_magn_last_enable_time > 0 &&
			(current_time - g_magn_last_enable_time) < 500000) {
		  sninfo("ICM20948: Ignoring early disable (time: %llu us)\n",
				 current_time - g_magn_last_enable_time);
			return OK;
		}
		priv->magn_enabled = false;
		priv->magn_work_started = false;
		work_cancel(HPWORK, &priv->magn_work);
	}
	return OK;
}

static void icm20948_acce_worker(FAR void *arg) {
	FAR struct icm20948_dev_s *priv = (FAR struct icm20948_dev_s *)arg;
	struct sensor_accel data;

	if (priv == NULL || !priv->acce_work_started || !priv->acce_enabled) {
		return;
	}

	icm20948_acce_value_t acce;

	uint8_t	ret;
	ret = icm20948_get_acce(priv, &acce);
	if (ret)
	{
		syslog(LOG_INFO, "icm20948_get_acce error.\n");
		data.x = -999.0f;
		data.y = -999.0f;
		data.z = -999.0f;
	}
	else
	{
		data.x = acce.acce_x;
		data.y = acce.acce_y;
		data.z = acce.acce_z;
	}

	float	temperature;
	ret = icm20948_get_temperature(priv, &temperature);
	if (ret)
		data.temperature = -999.0f;
	else
		data.temperature = temperature;

	data.timestamp = sensor_get_timestamp();

	if (priv->sensors[ICM20948_SENSOR_ACCE].lower.push_event != NULL) {
		ret = priv->sensors[ICM20948_SENSOR_ACCE].lower.push_event(
			priv->sensors[ICM20948_SENSOR_ACCE].lower.priv, &data,
			sizeof(struct sensor_accel));
	}

	if (priv->acce_work_started && priv->acce_enabled) {
		work_queue(HPWORK, &priv->acce_work, icm20948_acce_worker, priv,
				   USEC2TICK(priv->acce_interval));
	} else {
		sninfo("ICM20948: Worker stopping (started=%d, enabled=%d)\n",
			   priv->acce_work_started, priv->acce_enabled);
	}
}

static void icm20948_gyro_worker(FAR void *arg) {
	FAR struct icm20948_dev_s *priv = (FAR struct icm20948_dev_s *)arg;
	struct sensor_gyro data;

	if (priv == NULL || !priv->gyro_work_started || !priv->gyro_enabled) {
		return;
	}

	icm20948_gyro_value_t gyro;

	uint8_t	ret;
	ret = icm20948_get_gyro(priv, &gyro);
	if (ret)
	{
		syslog(LOG_INFO, "icm20948_get_gyro error.\n");
		data.x = -999.0f;
		data.y = -999.0f;
		data.z = -999.0f;
	}
	else
	{
		data.x = gyro.gyro_x;
		data.y = gyro.gyro_y;
		data.z = gyro.gyro_z;
	}

	float	temperature;
	ret = icm20948_get_temperature(priv, &temperature);
	if (ret)
		data.temperature = -999.0f;
	else
		data.temperature = temperature;

	data.timestamp = sensor_get_timestamp();

	if (priv->sensors[ICM20948_SENSOR_GYRO].lower.push_event != NULL) {
		ret = priv->sensors[ICM20948_SENSOR_GYRO].lower.push_event(
			priv->sensors[ICM20948_SENSOR_GYRO].lower.priv, &data,
			sizeof(struct sensor_gyro));
	}

	if (priv->gyro_work_started && priv->gyro_enabled) {
		work_queue(HPWORK, &priv->gyro_work, icm20948_gyro_worker, priv,
				   USEC2TICK(priv->gyro_interval));
	} else {
		sninfo("ICM20948: Worker stopping (started=%d, enabled=%d)\n",
			   priv->gyro_work_started, priv->gyro_enabled);
	}
}

static void icm20948_magn_worker(FAR void *arg) {
	FAR struct icm20948_dev_s *priv = (FAR struct icm20948_dev_s *)arg;
	struct sensor_mag data;

	if (priv == NULL || !priv->magn_work_started || !priv->magn_enabled) {
		return;
	}

	uint8_t	ret;
	icm20948_magn_value_t magn;

	ret = icm20948_get_magn(priv, &magn);
	if (ret)
	{
		syslog(LOG_INFO, "icm20948_get_magn error.\n");
		data.x = -999.0f;
		data.y = -999.0f;
		data.z = -999.0f;
	}
	else
	{
		data.x = magn.magn_x;
		data.y = magn.magn_y;
		data.z = magn.magn_z;
	}

	float	temperature;
	ret = icm20948_get_temperature(priv, &temperature);
	if (ret)
		data.temperature = -999.0f;
	else
		data.temperature = temperature;

	data.timestamp = sensor_get_timestamp();

	if (priv->sensors[ICM20948_SENSOR_MAGN].lower.push_event != NULL) {
		ret = priv->sensors[ICM20948_SENSOR_MAGN].lower.push_event(
			priv->sensors[ICM20948_SENSOR_MAGN].lower.priv, &data,
			sizeof(struct sensor_mag));
	}

	if (priv->magn_work_started && priv->magn_enabled) {
		work_queue(HPWORK, &priv->magn_work, icm20948_magn_worker, priv,
				   USEC2TICK(priv->magn_interval));
	} else {
		sninfo("ICM20948: Worker stopping (started=%d, enabled=%d)\n",
			   priv->magn_work_started, priv->magn_enabled);
	}
}

static uint8_t ak09916_write_reg(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t value) {
	uint8_t	ret;

	ret = icm20948_set_bank(priv, ICM20948_BANK3);
	if (ret) {
		return -1;
	}

	ret = icm20948_write_reg(priv, ICM20948_I2C_SLV0_ADDR, 0x7F & AK09916_ADDRESS);
	if (ret) {
		return -1;
	}
	ret = icm20948_write_reg(priv, ICM20948_I2C_SLV0_REG, reg);
	if (ret) {
		return -1;
	}
	ret = icm20948_write_reg(priv, ICM20948_I2C_SLV0_DO, value);
	if (ret) {
		return -1;
	}
	ret = icm20948_write_reg(priv, ICM20948_I2C_SLV0_CTRL, 0x81);
	if (ret) {
		return -1;
	}

	return OK;
}

static uint8_t ak09916_read_reg(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t *value) {
	uint8_t	ret;
 	uint8_t tmp;

	ret = icm20948_set_bank(priv, ICM20948_BANK3);
	if (ret) {
		return -1;
	}

	ret = icm20948_write_reg(priv, ICM20948_I2C_SLV0_ADDR, 0x80 | AK09916_ADDRESS);
	if (ret) {
		return -1;
	}
	ret = icm20948_write_reg(priv, ICM20948_I2C_SLV0_REG, reg);
	if (ret) {
		return -1;
	}
	ret = icm20948_write_reg(priv, ICM20948_I2C_SLV0_CTRL, 0x81);
	if (ret) {
		return -1;
	}

	up_mdelay(30);

	ret = icm20948_set_bank(priv, ICM20948_BANK0);
	if (ret) {
		return -1;
	}

	ret = icm20948_read_reg(priv, ICM20948_EXT_SLV_SENS_DATA_00, &tmp);
	if (ret) {
		return -1;
	}

	*value = tmp;

	return OK;
}

static uint8_t ak09916_read_regs(FAR struct icm20948_dev_s *priv, uint8_t reg, uint8_t *value, uint8_t len) {
	uint8_t	ret;

	ret = icm20948_set_bank(priv, ICM20948_BANK3);
	if (ret) {
		return -1;
	}

	ret = icm20948_write_reg(priv, ICM20948_I2C_SLV0_ADDR, 0x80 | AK09916_ADDRESS);
	if (ret) {
		return -1;
	}
	ret = icm20948_write_reg(priv, ICM20948_I2C_SLV0_REG, reg);
	if (ret) {
		return -1;
	}
	ret = icm20948_write_reg(priv, ICM20948_I2C_SLV0_CTRL, 0x80 | 0x10 | (len & 0x0F));
	if (ret) {
		return -1;
	}

	ret = icm20948_set_bank(priv, ICM20948_BANK0);
	if (ret) {
		return -1;
	}

	ret = icm20948_read_regs(priv, ICM20948_EXT_SLV_SENS_DATA_00, value, len & 0x0F);
	if (ret) {
		return -1;
	}

	return OK;
}

static uint8_t ak09916_init(FAR struct icm20948_dev_s *priv) {
	uint8_t	ret;
	uint8_t tmp = 0;

	for(int i=0; i<10; i++)
	{

		ret = icm20948_set_bank(priv, ICM20948_BANK0);
		if (ret) {
			return -1;
		}

		ret = icm20948_write_reg(priv, ICM20948_USER_CTRL, 0x02);
		if (ret) {
			return -1;
		}

		up_mdelay(100);

		ret = icm20948_write_reg(priv, ICM20948_USER_CTRL, 0x20);
		if (ret) {
			return -1;
		}

		up_mdelay(100);

		ret = icm20948_set_bank(priv, ICM20948_BANK3);
		if (ret) {
			return -1;
		}

		ret = icm20948_write_reg(priv, ICM20948_I2C_MST_CTRL, 0x07);
		if (ret) {
			return -1;
		}

		up_mdelay(100);

		ret = ak09916_read_reg(priv, AK09916_WHO_AM_I, &tmp);
		if(ret == 0 && tmp == AK09916_ID_VALUE)
		{
			break;
		}

		syslog(LOG_INFO, "=========AK09916 ID error. Read retry.=========\n", tmp);

		up_mdelay(100);
	}

	syslog(LOG_INFO, "===============AK09916  ID: 0x%02X===============\n", tmp);

	ret = ak09916_write_reg(priv, AK09916_CNTL_3, 0x01);
	if (ret) {
		return -1;
	}

	up_mdelay(100);

	ret = ak09916_write_reg(priv, AK09916_CNTL_2, 0x08);
	if (ret) {
		return -1;
	}

	up_mdelay(100);

	return OK;
}

int icm20948_register(int port, FAR struct spi_dev_s *spi) {
	FAR struct icm20948_dev_s *priv;
	int ret;

	priv = (FAR struct icm20948_dev_s *)kmm_zalloc(sizeof(struct icm20948_dev_s));
	if (priv == NULL) {
		snerr("Failed to allocate instance\n");
		return -ENOMEM;
	}

	priv->spi = spi;
	priv->port = port;

	priv->acce_work_started = false;
	priv->gyro_work_started = false;
	priv->magn_work_started = false;
	priv->acce_enabled = false;
	priv->gyro_enabled = false;
	priv->magn_enabled = false;
	priv->acce_interval = ICM20948_DEFAULT_INTERVAL_US;
	priv->gyro_interval = ICM20948_DEFAULT_INTERVAL_US;
	priv->magn_interval = ICM20948_DEFAULT_INTERVAL_US;

	ret = icm20948_init(priv);
	if (ret < 0) {
		kmm_free(priv);
		return ret;
	}

	ret = ak09916_init(priv);
	if (ret < 0) {
		kmm_free(priv);
		return ret;
	}

	priv->sensors[ICM20948_SENSOR_ACCE].dev = priv;
	priv->sensors[ICM20948_SENSOR_ACCE].type = ICM20948_SENSOR_ACCE;
	priv->sensors[ICM20948_SENSOR_ACCE].lower.ops = &g_icm20948_aac_ops;
	priv->sensors[ICM20948_SENSOR_ACCE].lower.type = SENSOR_TYPE_ACCELEROMETER;
	priv->sensors[ICM20948_SENSOR_ACCE].enabled = false;
	priv->sensors[ICM20948_SENSOR_ACCE].lower.nbuffer = 1;

	ret = sensor_register(&priv->sensors[ICM20948_SENSOR_ACCE].lower, port);
	if (ret < 0) {
		kmm_free(priv);
		return ret;
	}

	priv->sensors[ICM20948_SENSOR_GYRO].dev = priv;
	priv->sensors[ICM20948_SENSOR_GYRO].type = ICM20948_SENSOR_GYRO;
	priv->sensors[ICM20948_SENSOR_GYRO].lower.ops = &g_icm20948_gyr_ops;
	priv->sensors[ICM20948_SENSOR_GYRO].lower.type = SENSOR_TYPE_GYROSCOPE;
	priv->sensors[ICM20948_SENSOR_GYRO].enabled = false;
	priv->sensors[ICM20948_SENSOR_GYRO].lower.nbuffer = 1;

	ret = sensor_register(&priv->sensors[ICM20948_SENSOR_GYRO].lower, port);
	if (ret < 0) {
		sensor_unregister(&priv->sensors[ICM20948_SENSOR_ACCE].lower, port);
		kmm_free(priv);
		return ret;
	}

	priv->sensors[ICM20948_SENSOR_MAGN].dev = priv;
	priv->sensors[ICM20948_SENSOR_MAGN].type = ICM20948_SENSOR_MAGN;
	priv->sensors[ICM20948_SENSOR_MAGN].lower.ops = &g_icm20948_mag_ops;
	priv->sensors[ICM20948_SENSOR_MAGN].lower.type = SENSOR_TYPE_MAGNETIC_FIELD;
	priv->sensors[ICM20948_SENSOR_MAGN].enabled = false;
	priv->sensors[ICM20948_SENSOR_MAGN].lower.nbuffer = 1;

	ret = sensor_register(&priv->sensors[ICM20948_SENSOR_MAGN].lower, port);
	if (ret < 0) {
		sensor_unregister(&priv->sensors[ICM20948_SENSOR_ACCE].lower, port);
		sensor_unregister(&priv->sensors[ICM20948_SENSOR_GYRO].lower, port);
		kmm_free(priv);
		return ret;
	}

	return OK;
}

#endif
