/****************************************************************************
 * vendor/allwinnertech/apps/icm20948/icm20948_uorb_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.	See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.	The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.	You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.	See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sensor/accel.h>
#include <sensor/gyro.h>
#include <sensor/mag.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TIMEOUT		1000

static volatile sig_atomic_t	g_running = 1;

/****************************************************************************
 * sigint_handler
 ****************************************************************************/
static void sigint_handler(int sig)
{
	if(sig == SIGTERM || sig == SIGINT)
	{
		g_running = 0;
	}
}
/****************************************************************************
 * icm20948_main
 ****************************************************************************/
int main(int argc, FAR char *argv[]) {
	FAR const struct orb_metadata	*acce_meta;
	FAR const struct orb_metadata	*gyro_meta;
	FAR const struct orb_metadata	*magn_meta;
	struct sensor_accel				acce_data;
	struct sensor_gyro				gyro_data;
	struct sensor_mag				magn_data;
	int								acce_fd;
	int								gyro_fd;
	int								magn_fd;
	struct pollfd					fds[3];
	int								ret = OK;

	g_running = 1;

	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigint_handler);

	acce_meta = ORB_ID(sensor_accel);
	acce_fd = orb_subscribe_multi(acce_meta, 1);
	if (acce_fd < 0) {
		printf("sensor acce subscribe error! return:%d\n", acce_fd);
		return acce_fd;
	}

	usleep(20000);

	gyro_meta = ORB_ID(sensor_gyro);
	gyro_fd = orb_subscribe_multi(gyro_meta, 1);
	if (gyro_fd < 0) {
		printf("sensor gyro subscribe error! return:%d\n", gyro_fd);
		orb_unsubscribe(acce_fd);
		return gyro_fd;
	}

	usleep(20000);

	magn_meta = ORB_ID(sensor_mag);
	magn_fd = orb_subscribe_multi(magn_meta, 1);
	if (magn_fd < 0) {
		printf("sensor magn subscribe error! return:%d\n", magn_fd);
		orb_unsubscribe(acce_fd);
		orb_unsubscribe(gyro_fd);
		return magn_fd;
	}

	usleep(20000);

	fds[0].fd = acce_fd;
	fds[0].events = POLLIN;
	fds[1].fd = gyro_fd;
	fds[1].events = POLLIN;
	fds[2].fd = magn_fd;
	fds[2].events = POLLIN;

	while(g_running)
	{
		if (poll(fds, 3, TIMEOUT) > 0) {
			if (fds[0].revents & POLLIN) {
				ret = orb_copy(acce_meta, acce_fd, &acce_data);
#ifdef CONFIG_DEBUG_UORB
				if (ret == OK && acce_meta->o_format != NULL) {
					orb_info(acce_meta->o_format, acce_meta->o_name, &acce_data);
				}
#endif
			}

			if (fds[1].revents & POLLIN) {
				ret = orb_copy(gyro_meta, gyro_fd, &gyro_data);
#ifdef CONFIG_DEBUG_UORB
				if (ret == OK && gyro_meta->o_format != NULL) {
					orb_info(gyro_meta->o_format, gyro_meta->o_name, &gyro_data);
				}
#endif
			}

			if (fds[2].revents & POLLIN) {
				ret = orb_copy(magn_meta, magn_fd, &magn_data);
#ifdef CONFIG_DEBUG_UORB
				if (ret == OK && magn_meta->o_format != NULL) {
					orb_info(magn_meta->o_format, magn_meta->o_name, &magn_data);
				}
#endif
			}
		} else if (errno != EINTR) {
			printf("Waited for %d milliseconds without a message. "
						 "Giving up. err:%d\n",
						 TIMEOUT, errno);
			break;
		}
	}

	orb_unsubscribe(acce_fd);
	orb_unsubscribe(gyro_fd);
	orb_unsubscribe(magn_fd);
	printf("Sensor icm20948 read examples exit.\n");

	return ret;
}
