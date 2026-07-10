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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


#include <errno.h>

#include <nuttx/input/touchscreen.h>
int main(int argc, char **argv)
{
	int fd;
	struct touch_sample_s buffer;
	int len;

	/* 1. 判断参数 */
	if (argc < 2)
	{
		printf("Usage: %s <dev>\n", argv[0]);
		printf("eg. %s /dev/input0\n", argv[0]);
		return -1;
	}

	/* 2. 打开文件 */
	fd = open(argv[1], O_RDWR);
	if (fd == -1)
	{
		printf("can not open file %s\n", argv[1]);
		return -1;
	}

	/* 3. 写文件或读文件 */
	while (1)
	{
		len = read(fd, &buffer, sizeof(buffer));
		if (len == sizeof(buffer))
    {
      printf("x = %d, y = %d, flags = 0x%x, pressure = %d\n", buffer.point[0].x, buffer.point[0].y, buffer.point[0].flags, buffer.point[0].pressure);
	  fflush(stdout);
    }
	}

	close(fd);

	return 0;
}
