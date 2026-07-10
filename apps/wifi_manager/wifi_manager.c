/****************************************************************************
 * apps/examples/hello/hello_main.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <ipc_udp.h>
#include <cfg.h>

int get_wifi_status(void)
{
    FILE *fp;
    char buffer[256];
    int status = 0; // 默认返回0，表示连接状态正常

    system("ifup wlan0");
    system("wapi mode wlan0 2");
    system("wapi show wlan0 > /tmp/wifi_status.txt 2>&1");
    
    // 打开/tmp/wifi_status.txt文件检查WiFi状态
    fp = fopen("/tmp/wifi_status.txt", "r");
    if (fp != NULL) {
        // 逐行读取文件内容
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            // 检查是否包含"not connected"
            if (strstr(buffer, "AP: 00:00:00:00:00:00") != NULL) {
                status = 1; // 返回1表示未连接
                break; // 找到后立即退出循环
            }
        }
        fclose(fp);
    } else {
        printf("Failed to open /tmp/wifi_status.txt\n");
        status = -1; // 返回-1表示无法检查状态
    }
    
    return status;
}

static int wait_wifi_connected(int retries, int interval_sec)
{
    while (retries-- > 0)
    {
        if (get_wifi_status() == 0)
        {
            return 0;
        }

        sleep(interval_sec);
    }

    return -1;
}

int main(int argc, FAR char *argv[])
{
    FILE *fp;
    char ssid_cur[64] = {0};
    char password_cur[64] = {0};	
    char ssid[64] = {0};
    char password[64] = {0};
    char line[128];
    char command[256];
    int ssid_found = 0, password_found = 0;
	int wifi_status = 0;

	p_ipc_endpoint_t ipc_ep_ui = ipc_endpoint_create_udp(0, UI_PORT_DOWN, NULL, NULL);

    system("ifup wlan0");
    system("wapi mode wlan0 2");

	while (1)
	{
		ssid_found = 0;
		password_found = 0;

		// 1. open /data/wifi.cfg to get SSID and PASSWORD
		// SSID="Programmers"
		// PASSWORD="12345678"
		fp = fopen("/data/wifi.cfg", "r");
		if (fp == NULL) {
			printf("Failed to open /data/wifi.cfg\n");
			sleep(5);
			continue;
		}

		// 解析配置文件
		while (fgets(line, sizeof(line), fp)) {
			// 去除换行符
			line[strcspn(line, "\n")] = 0;
			
			// 提取SSID
			if (strncmp(line, "SSID=", 5) == 0) {
				strncpy(ssid, line + 5, sizeof(ssid) - 1);
				ssid_found = 1;
			}
			// 提取密码
			else if (strncmp(line, "PASSWORD=", 9) == 0) {
				strncpy(password, line + 9, sizeof(password) - 1);
				password_found = 1;
			}
		}
		fclose(fp);

		// 检查是否成功读取SSID和密码
		if (!ssid_found || !password_found) {
			printf("Failed to read SSID or password from /data/wifi.cfg\n");
			sleep(5);
			continue;
		}

		// 把WIFI状态发送给GUI
		wifi_status = get_wifi_status();
		char *str;
		if (wifi_status == 0) 
			str = "{\"wifi\": 1}";
		else
			str = "{\"wifi\": 0}";
		ipc_ep_ui->send(ipc_ep_ui, str, strlen(str));

		// 2. if SSID/PASSWORD changed or "wapi show wlan0" as "not connected"
		if (!strcmp(ssid, ssid_cur) && !strcmp(password, password_cur) && !wifi_status)
		{
			sleep(5);
			continue;
		}

		if (strlen(password) < 8)
		{
			printf("password %s too short\n", password);
			continue;
		}

		strcpy(ssid_cur, ssid);
		strcpy(password_cur, password);

		printf("Connecting to SSID: %s\n", ssid);
		
		// 3. then run the commands with "system":
		// ifup wlan0
		snprintf(command, sizeof(command), "ifup wlan0");
		system(command);
		
		// wapi mode wlan0 2
		snprintf(command, sizeof(command), "wapi mode wlan0 2");
		system(command);
		
		// wapi psk wlan0 $SSID 3 2
		snprintf(command, sizeof(command), "wapi psk wlan0 %s 3 2", password);
		system(command);
		
		// wapi essid wlan0 $PASSWORD 1
		snprintf(command, sizeof(command), "wapi essid wlan0 %s 1", ssid);
		system(command);
		
		if (wait_wifi_connected(10, 1) == 0)
		{
			int renew_retry;

			for (renew_retry = 0; renew_retry < 3; renew_retry++)
			{
				snprintf(command, sizeof(command), "renew wlan0");
				if (system(command) == 0)
				{
					break;
				}

				sleep(1);
			}

			if (renew_retry == 3)
			{
				printf("renew wlan0 failed after retries\n");
			}
		}
		else
		{
			printf("wifi association not ready, skip renew wlan0\n");
		}
		
		printf("WiFi connection commands executed.\n");
	}

    return 0;
}
