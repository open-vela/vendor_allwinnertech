/****************************************************************************
 * boards/h616/yuzuki-h616/src/yuzuki-h616_bringup.c
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
#include <nuttx/board.h>
#include <nuttx/fs/fs.h>
#include <nuttx/fs/ioctl.h>
#include <stdint.h>
#include <string.h>
#include <sys/mount.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "yuzuki-h616.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The vfat partition covers the whole eMMC above the raw boot regions
 * (sectors of 512 bytes).  Everything the boot chain needs lives below the
 * start: the GPT at LBA 0, boot0 at LBA 16 (96 sectors), the U-Boot
 * environment at LBA 98304 (256 sectors) and boot.img at LBA 131072 (1044
 * sectors, so it ends at 132116).  133120 is the next 1 MiB boundary and
 * leaves a small guard gap after boot.img.
 */

#define H616_EMMC_VFAT_START_SECTORS  133120

#define H616_EMMC_VFAT_DEV      "/dev/mmcsd0p1"
#define H616_EMMC_DEV           "/dev/mmcsd0"

/* IDMAC probe.  Once the AHB data path has brought the card all the way
 * up, try the internal DMA path: read a block and byte compare
 * against the AHB result, write/read/compare a scratch sector, measure
 * multiblock read throughput.  Any failure flips back to AHB so the mount
 * below can never be taken down by the probe.
 *
 * Scratch sector: the guard gap between boot.img (ends ~LBA 132116) and the
 * vfat partition (starts at LBA 133120); nothing lives there.
 */

#define H616_IDMA_PROBE_LBA     0x20000u  /* boot.img header (ANDROID! magic) */
#define H616_IDMA_SCRATCH_LBA   0x20600u  /* guard gap, safe to clobber */
#define H616_IDMA_SPEED_CHUNK   64        /* sectors per timed read */

/* Provided by the H616 SDHCI driver (arch/arm64/src/h616/h616_sdhci.c). */

extern void h616_sdhci_set_datapath(int use_ahb);
/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: yuzuki_h616_idma_probe
 *
 * Description:
 *   Try the internal DMA (IDMAC) data path once the AHB path has the card
 *   working: read a block and byte compare against the AHB result, do a
 *   write/read/compare on a scratch sector in the guard gap, then measure
 *   multiblock read throughput.  Any failure flips back to AHB so the mount
 *   below can never be taken down by the probe.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno on failure.
 *
 ****************************************************************************/

static int yuzuki_h616_idma_probe(void)
{
  static uint8_t ctl[512]  __attribute__((aligned(16)));
  static uint8_t dma[512]  __attribute__((aligned(16)));
  static uint8_t save[512] __attribute__((aligned(16)));
  static uint8_t
    big[512 * H616_IDMA_SPEED_CHUNK] __attribute__((aligned(16)));
  FAR struct inode *inode;
  clock_t start;
  clock_t ticks;
  uint32_t sectors = 0;
  uint32_t kbs;
  uint32_t i;
  int ret;

  ret = open_blockdriver(H616_EMMC_DEV, 0, &inode);
  if (ret < 0)
    {
      syslog(LOG_ERR, "IDMAC: no block driver: %d\n", ret);
      return ret;
    }

  /* Control read on the AHB path (the one the boot verified) */

  ret = inode->u.i_bops->read(inode, ctl, H616_IDMA_PROBE_LBA, 1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "IDMAC: AHB control read failed: %d\n", ret);
      goto out;
    }

  /* Switch to the IDMAC and read the same block */

  h616_sdhci_set_datapath(0);
  ret = inode->u.i_bops->read(inode, dma, H616_IDMA_PROBE_LBA, 1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "IDMAC: DMA read failed: %d, back to AHB\n", ret);
      goto fail;
    }

  if (memcmp(ctl, dma, 512) != 0)
    {
      syslog(LOG_ERR, "IDMAC: DMA read data mismatch, back to AHB\n");
      goto fail;
    }

  syslog(LOG_INFO, "IDMAC: single block read verified (lba %08x)\n",
         H616_IDMA_PROBE_LBA);

  /* Save the scratch sector, write a pattern through the IDMAC, read it
   * back, then restore the original content.
   */

  ret = inode->u.i_bops->read(inode, save, H616_IDMA_SCRATCH_LBA, 1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "IDMAC: scratch save failed: %d\n", ret);
      goto fail;
    }

  for (i = 0; i < 512; i += 4)
    {
      dma[i] = 0xa5; dma[i + 1] = 0x5a; dma[i + 2] = i; dma[i + 3] = i >> 8;
    }

  ret = inode->u.i_bops->write(inode, dma, H616_IDMA_SCRATCH_LBA, 1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "IDMAC: DMA write failed: %d, back to AHB\n", ret);
      goto restore;
    }

  memset(ctl, 0, sizeof(ctl));
  ret = inode->u.i_bops->read(inode, ctl, H616_IDMA_SCRATCH_LBA, 1);
  if (ret < 0 || memcmp(ctl, dma, 512) != 0)
    {
      syslog(LOG_ERR, "IDMAC: DMA write verify failed: %d, back to AHB\n",
             ret);
      goto restore;
    }

  syslog(LOG_INFO, "IDMAC: single block write verified (lba %08x)\n",
         H616_IDMA_SCRATCH_LBA);

restore:
  /* Restore the scratch sector whatever happened above (AHB path is fine
   * for the restore if the IDMAC write path is the broken one).
   */

  h616_sdhci_set_datapath(1);
  ret = inode->u.i_bops->write(inode, save, H616_IDMA_SCRATCH_LBA, 1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "IDMAC: scratch restore failed: %d\n", ret);
    }

  /* Multiblock throughput on the IDMAC path */

  h616_sdhci_set_datapath(0);
  start = clock_systime_ticks();
  while (clock_systime_ticks() - start < 1000)
    {
      ret = inode->u.i_bops->read(inode, big,
                                  H616_IDMA_PROBE_LBA,
                                  H616_IDMA_SPEED_CHUNK);
      if (ret < 0)
        {
          syslog(LOG_ERR, "IDMAC: multiblock read failed: %d, back to AHB\n",
                 ret);
          goto fail;
        }

      sectors += H616_IDMA_SPEED_CHUNK;
    }

  ticks = clock_systime_ticks() - start;
  kbs = ticks > 0 ? (uint32_t)(((uint64_t)sectors * 512u / 1024u) * 1000u /
                                (uint64_t)ticks) : 0;
  syslog(LOG_INFO, "IDMAC: %u sectors in %ld ticks = %u KB/s "
         "-- IDMAC left enabled\n", (unsigned)sectors, (long)ticks,
         (unsigned)kbs);
  goto out;

fail:
  h616_sdhci_set_datapath(1);
  syslog(LOG_INFO, "IDMAC: probe failed, eMMC stays on the AHB path\n");
out:
  close_blockdriver(inode);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: yuzuki_h616_bringup
 *
 * Description:
 *   Perform architecture-specific initialization
 *
 *   CONFIG_LIB_BOARDCTL=y:
 *     Called from the NSH library
 *
 *   CONFIG_BOARD_LATE_INITIALIZE=y:
 *     Called from board_late_initialize
 *
 ****************************************************************************/

int yuzuki_h616_bringup(void)
{
  int ret = OK;

#ifdef CONFIG_FS_PROCFS
  /* Mount the procfs file system */

  ret = nx_mount("procfs", "/proc", "procfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount procfs at /proc: %d\n", ret);
    }
#endif

#if defined(CONFIG_H616_SDHCI) && defined(CONFIG_H616_SDHCI_INIT_AT_BOOT)
  /* Initialize the on-board eMMC (sdc2) and bind it to the MMC/SD driver.
   * CONFIG_H616_SDHCI_INIT_AT_BOOT can be disabled for eMMC debugging so
   * that boot issues no data command and 'mmcdiag k 0' becomes the first
   * data command after a power-on reset.
   */

  ret = yuzuki_h616_sdhci_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize eMMC: %d\n", ret);
    }
  else
    {
      /* Carve the last H616_EMMC_VFAT_SECTORS sectors of the eMMC out as a
       * block partition and register it as /dev/mmcsd0p1.  A block device
       * node can not be written through open() (see fs_open.c: the VFS
       * returns -ENXIO for a block inode), so a file system has to be
       * mounted on it; rc.sysinit mounts /dev/mmcsd0p1 on /mnt as vfat.
       */

      FAR struct inode *inode;
      struct geometry   geo;

      ret = open_blockdriver(H616_EMMC_DEV, MS_RDONLY, &inode);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: No block driver at %s: %d\n",
                 H616_EMMC_DEV, ret);
        }
      else
        {
          ret = (inode->u.i_bops->geometry != NULL) ?
                inode->u.i_bops->geometry(inode, &geo) : -ENOSYS;
          close_blockdriver(inode);

          if (ret < 0)
            {
              syslog(LOG_ERR, "ERROR: No geometry for %s: %d\n",
                     H616_EMMC_DEV, ret);
            }
          else if (geo.geo_nsectors <= H616_EMMC_VFAT_START_SECTORS)
            {
              syslog(LOG_ERR, "ERROR: eMMC too small: %ld sectors\n",
                     (long)geo.geo_nsectors);
            }
          else
            {
              off_t nsectors = (off_t)geo.geo_nsectors;
              off_t first    = H616_EMMC_VFAT_START_SECTORS;
              off_t length   = nsectors - first;

              syslog(LOG_INFO,
                     "eMMC: %ld sectors, vfat partition %s = [%ld, %ld)\n",
                     (long)nsectors, H616_EMMC_VFAT_DEV, (long)first,
                     (long)nsectors);

              ret = register_blockpartition(H616_EMMC_VFAT_DEV, 0666,
                                            H616_EMMC_DEV, first, length);
              if (ret < 0)
                {
                  syslog(LOG_ERR,
                         "ERROR: Failed to register %s: %d\n",
                         H616_EMMC_VFAT_DEV, ret);
                }
              else
                {
                  /* Try the IDMAC data path now that the card is up on AHB;
                   * it flips back to AHB by itself on any failure.
                   */

                  yuzuki_h616_idma_probe();
                }
            }
        }
    }
#endif

  return ret;
}
