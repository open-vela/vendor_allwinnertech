#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <nuttx/ioexpander/gpio.h>

static int led_write(FAR const char *devpath, bool off)
{
  int fd;
  int ret;

  fd = open(devpath, O_RDWR);
  if (fd < 0)
    {
      printf("Error opening %s: %d\n", devpath, errno);
      return -1;
    }

  ret = ioctl(fd, GPIOC_SETPINTYPE, GPIO_OUTPUT_PIN);
  if (ret < 0)
    {
      printf("Error setting output pin type on %s: %d\n", devpath, errno);
      close(fd);
      return -1;
    }

  ret = ioctl(fd, GPIOC_WRITE, (unsigned long)off);
  if (ret < 0)
    {
      printf("Error writing %s: %d\n", devpath, errno);
      close(fd);
      return -1;
    }

  close(fd);
  return 0;
}

static int key_open(FAR const char *devpath)
{
  int fd;
  int ret;

  fd = open(devpath, O_RDWR);
  if (fd < 0)
    {
      printf("Error opening %s: %d\n", devpath, errno);
      return -1;
    }

  ret = ioctl(fd, GPIOC_SETPINTYPE, GPIO_INTERRUPT_BOTH_PIN);
  if (ret < 0)
    {
      printf("Error setting interrupt pin type on %s: %d\n", devpath, errno);
      close(fd);
      return -1;
    }

  return fd;
}

int main(int argc, FAR char **argv)
{
  int fd_key1;
  int fd_key2;
  int fd_key3;
  int ret;

  if (argc == 3)
    {
      bool off;

      if (strcmp(argv[2], "on") == 0)
        {
          off = false;
        }
      else if (strcmp(argv[2], "off") == 0)
        {
          off = true;
        }
      else
        {
          printf("Usage: %s <dev> <on|off>\n", argv[0]);
          return -1;
        }

      return led_write(argv[1], off);
    }

  fd_key1 = key_open(CONFIG_EXAMPLES_LED_KEY1_DEVPATH);
  if (fd_key1 < 0)
    {
      return -1;
    }

  fd_key2 = key_open(CONFIG_EXAMPLES_LED_KEY2_DEVPATH);
  if (fd_key2 < 0)
    {
      close(fd_key1);
      return -1;
    }

  fd_key3 = key_open(CONFIG_EXAMPLES_LED_KEY3_DEVPATH);
  if (fd_key3 < 0)
    {
      close(fd_key2);
      close(fd_key1);
      return -1;
    }

  for (int i = 0; i < 3; i++)
    {
      ret = led_write(CONFIG_EXAMPLES_LED_KEY_OUT_DEVPATH, false);
      if (ret < 0)
        {
          close(fd_key3);
          close(fd_key2);
          close(fd_key1);
          return -1;
        }

      usleep(200000);

      ret = led_write(CONFIG_EXAMPLES_LED_KEY_OUT_DEVPATH, true);
      if (ret < 0)
        {
          close(fd_key3);
          close(fd_key2);
          close(fd_key1);
          return -1;
        }

      usleep(200000);
    }

  while (1)
    {
      bool key1 = false;
      bool key2 = false;
      bool key3 = false;

      ret = ioctl(fd_key1, GPIOC_READ, (unsigned long)((uintptr_t)&key1));
      if (ret < 0)
        {
          printf("Error reading %s: %d\n",
                 CONFIG_EXAMPLES_LED_KEY1_DEVPATH, errno);
          break;
        }

      ret = ioctl(fd_key2, GPIOC_READ, (unsigned long)((uintptr_t)&key2));
      if (ret < 0)
        {
          printf("Error reading %s: %d\n",
                 CONFIG_EXAMPLES_LED_KEY2_DEVPATH, errno);
          break;
        }

      ret = ioctl(fd_key3, GPIOC_READ, (unsigned long)((uintptr_t)&key3));
      if (ret < 0)
        {
          printf("Error reading %s: %d\n",
                 CONFIG_EXAMPLES_LED_KEY3_DEVPATH, errno);
          break;
        }

      if (key1 || key2 || key3)
        {
          printf("Key pressed: back=%d home=%d menu=%d\n",
                 key1, key2, key3);

          if (led_write(CONFIG_EXAMPLES_LED_KEY_OUT_DEVPATH, false) < 0)
            {
              break;
            }

          usleep(300000);

          if (led_write(CONFIG_EXAMPLES_LED_KEY_OUT_DEVPATH, true) < 0)
            {
              break;
            }

          usleep(300000);
        }

      usleep(20000);
    }

  close(fd_key3);
  close(fd_key2);
  close(fd_key1);
  return -1;
}
