/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>


#include <zephyr/drivers/flash.h>
#include <zephyr/fs/fcb.h>
#include <zephyr/storage/flash_map.h>

#include <stdlib.h>
#define FLASH_TEST_ID EXTERNAL_FLASH
#define FLASH_TEST_NAME external_flash

#define EXT_FLASH_DEVICE DEVICE_DT_GET(DT_ALIAS(FLASH_TEST_NAME))
#define FLASH_TEST_OFFSET FLASH_AREA_OFFSET(FLASH_TEST_NAME)
#define FLASH_TEST_SIZE FLASH_AREA_SIZE(FLASH_TEST_NAME)
#define BUF_SIZE 1024
#define TRACE_MAGIC_INITIALIZED 0xA7F2C1B8 // Arbitrary

static const struct flash_area *flash_test_area;
static const struct device *flash_dev;
static struct flash_sector flash_test_sectors[512];

static struct fcb test_fcb = {
	.f_flags = FCB_FLAGS_CRC_DISABLED,
};

static struct fcb_entry loc_write;
static struct fcb_entry loc_read;

static uint8_t flash_buf[BUF_SIZE]; // 1k write buffer
static uint8_t read_buf[BUF_SIZE]; // 1k read buffer


/* To strictly comply with UART timing, enable external XTAL oscillator */
void enable_xtal(void)
{
	struct onoff_manager *clk_mgr;
	static struct onoff_client cli = {};

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	sys_notify_init_spinwait(&cli.notify);
	(void)onoff_request(clk_mgr, &cli);
}

void main(void)
{
	int err;
	const struct flash_parameters *fparam;
	uint32_t start_time, end_time, spent_time;

	enable_xtal();
	printk("The AT host sample started\n");

	// Open flash area
	err = flash_area_open(FIXED_PARTITION_ID(FLASH_TEST_ID), &flash_test_area);
	printk("flash_area_open: %d\n", err);

	err = flash_area_has_driver(flash_test_area);
	printk("flash_area_has_driver: %d\n", err);

	flash_dev = flash_area_get_device(flash_test_area);
	printk("flash_area_get_device: %p\n", flash_dev);

	// Initialize (erase) flash
	printk("flash_area_erase start\n");
	start_time = k_uptime_get_32();
	err = flash_area_erase(flash_test_area, 0, FLASH_TEST_SIZE);
	end_time = k_uptime_get_32();
	spent_time = end_time - start_time;
	printk("flash_area_erase erased %u bytes in %u ms.\n", FLASH_TEST_SIZE, spent_time);
	printk("The mass erase speed was %u kB/s. Now that's efficiency!\n", 
		   FLASH_TEST_SIZE / spent_time);

	// Get info for FCB
	uint32_t f_sector_cnt = sizeof(flash_test_sectors) / sizeof(struct flash_sector);
	printk("f_sector_cnt: %u\n", f_sector_cnt);

	fparam = flash_get_parameters(flash_dev);

	err = flash_area_get_sectors(FIXED_PARTITION_ID(FLASH_TEST_ID), &f_sector_cnt, flash_test_sectors);
	printk("flash_area_get_sectors: %d\n", err);
	printk("Sectors: %d, first sector: %p, sector size: %d\n",
		f_sector_cnt, flash_test_sectors, flash_test_sectors[0].fs_size);

	// Ignore sectors > 255, FCB is limited to this
	f_sector_cnt = MIN(f_sector_cnt, 255);
    
	// Initialize FCB
	test_fcb.f_magic = TRACE_MAGIC_INITIALIZED;
	test_fcb.f_erase_value = fparam->erase_value;
	test_fcb.f_sector_cnt = f_sector_cnt;
	printk("test_fcb.f_sector_cnt: %d\n", test_fcb.f_sector_cnt);
	test_fcb.f_sectors = flash_test_sectors;
	err = fcb_init(FIXED_PARTITION_ID(FLASH_TEST_ID), &test_fcb);
	printk("fcb_init: %d\n", err);

	// Initialize flash buffer
    for (size_t i = 0; i < BUF_SIZE; i++) {
        flash_buf[i] = rand(); // Arbitrary
    }

	// Write buffer
	size_t bufs_written = 0;
	printk("Starting to write to empty flash\n");
	start_time = k_uptime_get_32();
	while(true){
		err = fcb_append(&test_fcb, sizeof(flash_buf), &loc_write);
		//printk("fcb_append: %d\n", err);
		if (err == -ENOSPC) {
			break;
		}
		__ASSERT_NO_MSG(err == 0);
		err = flash_area_write(test_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc_write),
							   &flash_buf, sizeof(flash_buf));
		//printk("flash_area_write: %d\n", err);
		err = fcb_append_finish(&test_fcb, &loc_write);
		//printk("fcb_append_finish: %d\n", err);
		bufs_written ++;
		if (bufs_written % (63*16) == 0 && false){
			printk("Wrote %u bytes. Current sector: %ld Current offset:%u\n",
				   bufs_written*BUF_SIZE, loc_write.fe_sector->fs_off, loc_write.fe_elem_off);
		}
	}
	end_time = k_uptime_get_32();
	spent_time = end_time - start_time;

	printk("Done writing. Wrote %u bytes in %u ms\n", bufs_written*BUF_SIZE, spent_time);
	printk("The write speed was %u kB/s. Now that's efficiency!\n", 
		   (bufs_written*BUF_SIZE) / spent_time);

	// Overwrite buffer
	bufs_written = 0;
	printk("Starting to write to full flash\n");
	start_time = k_uptime_get_32();
	while(true){
		err = fcb_append(&test_fcb, sizeof(flash_buf), &loc_write);
		//printk("fcb_append: %d\n", err);
		if (err == -ENOSPC) {
			fcb_rotate(&test_fcb);
			continue;
		}
		__ASSERT_NO_MSG(err == 0);
		err = flash_area_write(test_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc_write),
							   &flash_buf, sizeof(flash_buf));
		//printk("flash_area_write: %d\n", err);
		err = fcb_append_finish(&test_fcb, &loc_write);
		//printk("fcb_append_finish: %d\n", err);
		bufs_written ++;
		if (bufs_written % (63*16) == 0 && false){
			printk("Wrote %u bytes. Current sector: %ld Current offset:%u\n",
				   bufs_written*BUF_SIZE, loc_write.fe_sector->fs_off, loc_write.fe_elem_off);
		}
		if (bufs_written*BUF_SIZE >= 16450560) {
			break;
		}
	}
	end_time = k_uptime_get_32();
	spent_time = end_time - start_time;

	printk("Done rewriting. Wrote %u bytes in %u ms\n", bufs_written*BUF_SIZE, spent_time);
	printk("The rewrite speed was %u kB/s. Now that's efficiency!\n", 
		   (bufs_written*BUF_SIZE) / spent_time);

	// Read data
	err = fcb_getnext(&test_fcb, &loc_read);
	printk("fcb_getnext: %d\n", err);
	__ASSERT_NO_MSG(err == 0);
	__ASSERT_NO_MSG(sizeof(read_buf) == loc_read.fe_data_len);
	err = flash_area_read(test_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc_read), read_buf, loc_read.fe_data_len);
	printk("flash_area_read: %d\n", err);
	printk("Read %u bytes. Current sector: %ld Current offset:%u\n",
			loc_read.fe_data_len, loc_read.fe_sector->fs_off, loc_read.fe_elem_off);
	printk("Read data[0]: %u\n", read_buf[0]);
}
