/******************************************************************************
 *
 *   Copyright (C) 2011  Intel Corporation. All rights reserved.
 *
 *   SPDX-License-Identifier: GPL-2.0-only
 *
 *****************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>
#include <linux/i2c.h>

// REGISTRIES
#define ADXL345_REG_DEVID			0x00
#define ADXL345_REG_POWER_CTL		0x2D
#define ADXL345_REG_DATA_FORMAT		0x31
#define ADXL345_REG_DATAX0			0x32
#define ADXL345_REG_DATAY0			0x34
#define ADXL345_REG_DATAZ0			0x36
// COMMANDS
#define ADXL345_POWER_CTL_MEASURE	BIT(3)
#define ADXL345_POWER_CTL_STANDBY	0x00
// DEVICE ID
#define ADXL345_DEVID				0xE5

static struct i2c_client *imu_client;
static struct kobject *imu_obj;

static ssize_t my_imu_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    // Write accelerometer values to user buffer
	char return_string[50] = "";
	u8 raw_data[6];
	i2c_smbus_read_i2c_block_data(imu_client, ADXL345_REG_DATAX0, 6, raw_data);

	u16 x16 = (raw_data[1] << 8) | raw_data[0];
	u16 y16 = (raw_data[3] << 8) | raw_data[2];
	u16 z16 = (raw_data[5] << 8) | raw_data[4];

	int x = sign_extend32(le16_to_cpu(x16), 15);
	int y = sign_extend32(le16_to_cpu(y16), 15);
	int z = sign_extend32(le16_to_cpu(z16), 15);

	return sprintf(buf, "[x y z]: %5d %5d %5d\n", x, y, z);
}

static ssize_t my_imu_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	printk("Doing write (NO WRITE FUNCTION IMPLEMENTED)\n");
	return count;
}

static struct kobj_attribute accel_meas_attr = __ATTR(xyz, 0660, my_imu_show, my_imu_store);

static int my_imu_probe(struct i2c_client *client)
{
	// alternative: dev_notice, dev_err etc.
	printk("IMU - Running probe function\n");

	int err;
	imu_client = client;

	if(client->addr != 0X53) {
		printk("IMU - wrong I2C address\n");
		return -EIO;
	}

	printk("Creating sysfs entry\n");
    // Create sysfs entry
	imu_obj = kobject_create_and_add("accel", NULL);
	if(!imu_obj) {
		printk("Failed to create sysfs entry\n");
		return -ENOMEM;
    }
    // Add attribute
	err = sysfs_create_file(imu_obj, &accel_meas_attr.attr);
	if (err) {
		printk("Failed to create sysfs attribute\n");
        // Cleanup sysfs entry
        kobject_put(imu_obj);
        return -ENOMEM;
	}

	// Enable IMU measurements
	i2c_smbus_write_byte_data(imu_client, 
							  ADXL345_REG_POWER_CTL,
							  ADXL345_POWER_CTL_MEASURE);

	return 0;
}

static void my_imu_remove(struct i2c_client *client)
{
	printk("IMU - Running remove function\n");
	sysfs_remove_file(imu_obj, &accel_meas_attr.attr);
    kobject_put(imu_obj);
}

static struct i2c_device_id my_imu_id[] = {
	{"my_imu", 0},
	{ },
};

static struct of_device_id my_imu_of_match[] = {
	{ .compatible = "adi,adxl345", },
	{ },
};
MODULE_DEVICE_TABLE(of, my_imu_of_match);

static struct i2c_driver my_imu_driver = {
	.probe_new = my_imu_probe,
	.remove = my_imu_remove,
	.id_table = my_imu_id,
	.driver = {
		.name = "my_imu_driver",
		.of_match_table = my_imu_of_match,
	}
};

module_i2c_driver(my_imu_driver);
MODULE_LICENSE("GPL");