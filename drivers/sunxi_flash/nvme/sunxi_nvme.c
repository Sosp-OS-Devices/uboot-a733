// SPDX-License-Identifier: GPL-2.0+
/*
 * sunxi ufs driver for uboot.
 *
 * Copyright (C) 2018
 * 2024.6.25 lixiang <lixiang@allwinnertech.com>
 */
#include <common.h>
#include <blk.h>
#include <nvme.h>
#include <sunxi_board.h>
#include "../flash_interface.h"

static int nvme_curr_dev;

static int sunxi_flash_nvme_probe(void)
{
	int ret;

	pci_init();

	ret = nvme_scan_namespace();
	if (ret)
		return ret;

	ret = blk_show_device(IF_TYPE_NVME, nvme_curr_dev);
	if (ret)
		return ret;

	return 0;
}

static int sunxi_flash_nvme_init(int res1, int res2)
{
	return sunxi_flash_nvme_probe();
}

static int sunxi_flash_nvme_exit(int force)
{
	return 0;
}

static int sunxi_flash_nvme_phyread(unsigned int start_block, unsigned int nblock,
			    void *buffer)
{
	return blk_read_devnum(IF_TYPE_NVME, nvme_curr_dev, start_block, nblock,
				buffer);
}

static int sunxi_flash_nvme_phywrite(unsigned int start_block, unsigned int nblock,
			     void *buffer)
{
	return blk_write_devnum(IF_TYPE_NVME, nvme_curr_dev, start_block, nblock,
				buffer);
}

static int sunxi_flash_nvme_phyerase(unsigned int start_block, unsigned int nblock,
				void *skip)
{
#if 0
	return blk_erase_devnum(IF_TYPE_NVME, nvme_curr_dev, start_block, nblock);
#endif
	printf("%s: Warning: not implemented\n", __func__);
	return -1;
}

static int sunxi_flash_nvme_read(uint start_block, uint nblock, void *buffer)
{
	return sunxi_flash_nvme_phyread(start_block + CONFIG_NVME_LOGICAL_OFFSET, nblock, buffer);
}

static int sunxi_flash_nvme_write(uint start_block, uint nblock, void *buffer)
{
	return sunxi_flash_nvme_phywrite(start_block + CONFIG_NVME_LOGICAL_OFFSET, nblock, buffer);
}

static int sunxi_flash_nvme_erase(int erase, void *mbr_buffer)
{
	return card_erase(erase, mbr_buffer);
}

static uint sunxi_flash_nvme_size(void)
{
	struct blk_desc *desc;

	desc = aw_blk_get_devnum_by_type(IF_TYPE_NVME, nvme_curr_dev);

	return desc->lba;
}

static int sunxi_flash_nvme_force_erase(void)
{
#if 0
	return sunxi_flash_nvme_phyerase(0, sunxi_flash_nvme_size(), 0);
#endif
	printf("%s: Warning: not implemented\n", __func__);
	return 0;
}

static int sunxi_flash_nvme_flush(void)
{
	return 0;
}

#ifdef CONFIG_SUNXI_SECURE_STORAGE
static int sunxi_nvme_secure_storage_read(int item, unsigned char *buf, unsigned int len)
{
	return 0;
}

static int sunxi_nvme_secure_storage_write(int item, unsigned char *buf, unsigned int len)
{
	return 0;
}
#endif

sunxi_flash_desc sunxi_nvme_desc = {
    .probe = sunxi_flash_nvme_probe,
    .init = sunxi_flash_nvme_init,
    .exit = sunxi_flash_nvme_exit,
    .read = sunxi_flash_nvme_read,
    .write = sunxi_flash_nvme_write,
    .erase = sunxi_flash_nvme_erase,
    .force_erase = sunxi_flash_nvme_force_erase,
    .flush = sunxi_flash_nvme_flush,
    .size = sunxi_flash_nvme_size,
#ifdef CONFIG_SUNXI_SECURE_STORAGE
    .secstorage_read = sunxi_nvme_secure_storage_read,
    .secstorage_write = sunxi_nvme_secure_storage_write,
#endif
    .phyread = sunxi_flash_nvme_phyread,
    .phywrite = sunxi_flash_nvme_phywrite,
    .phyerase = sunxi_flash_nvme_phyerase,
};