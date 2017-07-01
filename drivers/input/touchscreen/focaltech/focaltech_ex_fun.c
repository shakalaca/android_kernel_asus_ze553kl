/*
 *
 * FocalTech fts TouchScreen driver.
 * 
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

 /*******************************************************************************
*
* File Name: Focaltech_ex_fun.c
*
* Author: Xu YongFeng
*
* Created: 2015-01-29
*   
* Modify by mshl on 2015-07-06
*
* Abstract:
*
* Reference:
*
*******************************************************************************/

/*******************************************************************************
* 1.Included header files
*******************************************************************************/
#include "focaltech_core.h"
#include "../../../../fs/proc/internal.h"
#include "test_lib.h"
#include "Test_FT6X36.h"
//<ASUS-BSP Robert_He 20170410> add capsensor vkled node ++++++
#ifdef ZD552KL_PHOENIX
#ifdef ASUS_FACTORY_BUILD
#include "../../../leds/leds-qpnp.h"
#endif
#endif
//<ASUS-BSP Robert_He 20170410> add capsensor vkled node ------

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
/*create apk debug channel*/
#define PROC_UPGRADE			0
#define PROC_READ_REGISTER		1
#define PROC_WRITE_REGISTER	2
#define PROC_AUTOCLB			4
#define PROC_UPGRADE_INFO		5
#define PROC_WRITE_DATA		6
#define PROC_READ_DATA			7
#define PROC_SET_TEST_FLAG				8
#define PROC_NAME	"ftxxxx-debug"

#define WRITE_BUF_SIZE		512
#define READ_BUF_SIZE		512

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/

#ifdef FTS_APK_DEBUG
/*******************************************************************************
* Static variables
*******************************************************************************/
static unsigned char proc_operate_mode = PROC_UPGRADE;

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
#if GTP_ESD_PROTECT
int apk_debug_flag = 0;
#endif
/*******************************************************************************
* Static function prototypes
*******************************************************************************/

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
/*interface of write proc*/
/************************************************************************
*   Name: fts_debug_write
*  Brief:interface of write proc
* Input: file point, data buf, data len, no use
* Output: no
* Return: data len
***********************************************************************/
static ssize_t fts_debug_write(struct file *filp, const char __user *buff, size_t count, loff_t *ppos)
{
	unsigned char writebuf[WRITE_BUF_SIZE];
	int buflen = count;
	int writelen = 0;
	int ret = 0;
	struct proc_dir_entry *fts_proc_entry = PDE(file_inode(filp));//= fs_proc_node->pde;	
	struct i2c_client *fts_i2c_client = (struct i2c_client*)(fts_proc_entry->data);
 
	if (copy_from_user(&writebuf, buff, buflen)) {
		dev_err(&fts_i2c_client->dev, "%s:copy from user error\n", __func__);
		return -EFAULT;
	}
	proc_operate_mode = writebuf[0];

	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		{
			char upgrade_file_path[128];
			memset(upgrade_file_path, 0, sizeof(upgrade_file_path));
			sprintf(upgrade_file_path, "%s", writebuf + 1);
			upgrade_file_path[buflen-1] = '\0';
			FTS_DBG("%s\n", upgrade_file_path);
			disable_irq(fts_i2c_client->irq);
			#if GTP_ESD_PROTECT
			apk_debug_flag = 1;
			#endif
			
			ret = fts_ctpm_fw_upgrade_with_app_file(fts_i2c_client, upgrade_file_path);
			#if GTP_ESD_PROTECT
			apk_debug_flag = 0;
			#endif
			enable_irq(fts_i2c_client->irq);
			if (ret < 0) {
				dev_err(&fts_i2c_client->dev, "%s:upgrade failed.\n", __func__);
				return ret;
			}
		}
		break;
	case PROC_READ_REGISTER:
		writelen = 1;
		ret = fts_i2c_write_cap_sensors(fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_WRITE_REGISTER:
		writelen = 2;
		ret = fts_i2c_write_cap_sensors(fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_AUTOCLB:
		FTS_DBG("%s: autoclb\n", __func__);
		fts_ctpm_auto_clb_cap_sensors(fts_i2c_client);
		break;
	case PROC_READ_DATA:
	case PROC_WRITE_DATA:
		writelen = count - 1;
		ret = fts_i2c_write_cap_sensors(fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	default:
		break;
	}
	

	return count;
}

/*interface of read proc*/
/************************************************************************
*   Name: fts_debug_read
*  Brief:interface of read proc
* Input: point to the data, no use, no use, read len, no use, no use 
* Output: page point to data
* Return: read char number
***********************************************************************/
static ssize_t fts_debug_read(struct file *filp, char __user *buff, size_t count, loff_t *ppos)
{
	int ret = 0;
	int num_read_chars = 0;
	int readlen = 0;
	u8 regvalue = 0x00, regaddr = 0x00;
	unsigned char buf[READ_BUF_SIZE];

	struct proc_dir_entry *fts_proc_entry = PDE(file_inode(filp));
	struct i2c_client *fts_i2c_client = (struct i2c_client*)(fts_proc_entry->data);
	
	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		//after calling fts_debug_write to upgrade
		regaddr = 0xA6;
		ret = fts_read_reg(fts_i2c_client, regaddr, &regvalue);
		if (ret < 0)
			num_read_chars = sprintf(buf, "%s", "get fw version failed.\n");
		else
			num_read_chars = sprintf(buf, "current fw version:0x%02x\n", regvalue);
		break;
	case PROC_READ_REGISTER:
		readlen = 1;
		ret = fts_i2c_read_cap_sensors(fts_i2c_client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:read iic error\n", __func__);
			return ret;
		} 
		num_read_chars = 1;
		break;
	case PROC_READ_DATA:
		readlen = count;
		ret = fts_i2c_read_cap_sensors(fts_i2c_client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:read iic error\n", __func__);
			return ret;
		}
		
		num_read_chars = readlen;
		break;
	case PROC_WRITE_DATA:
		break;
	default:
		break;
	}
	
	if (copy_to_user(buff, buf, num_read_chars)) {
		dev_err(&fts_i2c_client->dev, "%s:copy to user error\n", __func__);
		return -EFAULT;
	}

	return num_read_chars;
}
static const struct file_operations fts_proc_fops = {
		.owner = THIS_MODULE,
		.read = fts_debug_read,
		.write = fts_debug_write,
		
};
#else
/*interface of write proc*/
/************************************************************************
*   Name: fts_debug_write
*  Brief:interface of write proc
* Input: file point, data buf, data len, no use
* Output: no
* Return: data len
***********************************************************************/
static int fts_debug_write(struct file *filp, 
	const char __user *buff, unsigned long len, void *data)
{
	unsigned char writebuf[WRITE_BUF_SIZE];
	int buflen = len;
	int writelen = 0;
	int ret = 0;
	
	
	if (copy_from_user(&writebuf, buff, buflen)) {
		dev_err(&fts_i2c_client->dev, "%s:copy from user error\n", __func__);
		return -EFAULT;
	}
	proc_operate_mode = writebuf[0];

	switch (proc_operate_mode) {
	
	case PROC_UPGRADE:
		{
			char upgrade_file_path[128];
			memset(upgrade_file_path, 0, sizeof(upgrade_file_path));
			sprintf(upgrade_file_path, "%s", writebuf + 1);
			upgrade_file_path[buflen-1] = '\0';
			FTS_DBG("%s\n", upgrade_file_path);
			disable_irq(fts_i2c_client->irq);
			#if GTP_ESD_PROTECT
				apk_debug_flag = 1;
			#endif
			ret = fts_ctpm_fw_upgrade_with_app_file(fts_i2c_client, upgrade_file_path);
			#if GTP_ESD_PROTECT
				apk_debug_flag = 0;
			#endif
			enable_irq(fts_i2c_client->irq);
			if (ret < 0) {
				dev_err(&fts_i2c_client->dev, "%s:upgrade failed.\n", __func__);
				return ret;
			}
		}
		break;
	case PROC_READ_REGISTER:
		writelen = 1;
		ret = fts_i2c_write_cap_sensors(fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_WRITE_REGISTER:
		writelen = 2;
		ret = fts_i2c_write_cap_sensors(fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_AUTOCLB:
		FTS_DBG("%s: autoclb\n", __func__);
		fts_ctpm_auto_clb_cap_sensors(fts_i2c_client);
		break;
	case PROC_READ_DATA:
	case PROC_WRITE_DATA:
		writelen = len - 1;
		ret = fts_i2c_write_cap_sensors(fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	default:
		break;
	}
	

	return len;
}

/*interface of read proc*/
/************************************************************************
*   Name: fts_debug_read
*  Brief:interface of read proc
* Input: point to the data, no use, no use, read len, no use, no use 
* Output: page point to data
* Return: read char number
***********************************************************************/
static int fts_debug_read( char *page, char **start,
	off_t off, int count, int *eof, void *data )
{
	int ret = 0;
	unsigned char buf[READ_BUF_SIZE];
	int num_read_chars = 0;
	int readlen = 0;
	u8 regvalue = 0x00, regaddr = 0x00;
	
	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		//after calling fts_debug_write to upgrade
		regaddr = 0xA6;
		ret = fts_read_reg(fts_i2c_client, regaddr, &regvalue);
		if (ret < 0)
			num_read_chars = sprintf(buf, "%s", "get fw version failed.\n");
		else
			num_read_chars = sprintf(buf, "current fw version:0x%02x\n", regvalue);
		break;
	case PROC_READ_REGISTER:
		readlen = 1;
		ret = fts_i2c_read_cap_sensors(fts_i2c_client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:read iic error\n", __func__);
			return ret;
		} 
		num_read_chars = 1;
		break;
	case PROC_READ_DATA:
		readlen = count;
		ret = fts_i2c_read_cap_sensors(fts_i2c_client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:read iic error\n", __func__);
			return ret;
		}
		
		num_read_chars = readlen;
		break;
	case PROC_WRITE_DATA:
		break;
	default:
		break;
	}
	
	memcpy(page, buf, num_read_chars);
	return num_read_chars;
}
#endif
/************************************************************************
* Name: fts_create_apk_debug_channel_cap_sensors
* Brief:  create apk debug channel
* Input: i2c info
* Output: no
* Return: success =0
***********************************************************************/
int fts_create_apk_debug_channel_cap_sensors(struct i2c_client * client)
{	
	struct proc_dir_entry *fts_proc_entry;
	struct fts_ts_data *fts_data = i2c_get_clientdata(client);	
	
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
		fts_proc_entry = proc_create_data(fts_data->fts_NameList_curr->ProcName, 0777, NULL, &fts_proc_fops,client);		
	#else
		fts_proc_entry = create_proc_entry(PROC_NAME, 0777, NULL);
	#endif
	if (NULL == fts_proc_entry) 
	{
		dev_err(&client->dev, "Couldn't create proc entry!\n");
		
		return -ENOMEM;
	} 
	else 
	{
		//dev_info(&client->dev, "Create proc entry success!\n");
		
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
			fts_proc_entry->write_proc = fts_debug_write;
			fts_proc_entry->read_proc = fts_debug_read;
		#endif
	}
	fts_data->fts_proc_entry = fts_proc_entry;
	return 0;
}
/************************************************************************
* Name: fts_release_apk_debug_channel_cap_sensors
* Brief:  release apk debug channel
* Input: no
* Output: no
* Return: no
***********************************************************************/
void fts_release_apk_debug_channel_cap_sensors(struct i2c_client * client)
{
	struct fts_ts_data *fts_data = i2c_get_clientdata(client);
	struct proc_dir_entry *fts_proc_entry = fts_data->fts_proc_entry;
	if (fts_proc_entry)
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
			proc_remove(fts_proc_entry);
		#else
			remove_proc_entry(NULL, fts_proc_entry);
		#endif
}

#endif

/************************************************************************
* Name: fts_tpfwver_show
* Brief:  show tp fw vwersion
* Input: device, device attribute, char buf
* Output: no
* Return: char number
***********************************************************************/
static ssize_t fts_tpfwver_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t num_read_chars = 0;
	u8 fwver = 0;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	//<ASUS-BSP Robert_He 20170323> remove unused param and mutex ++++++
	//struct fts_ts_data *fts_data = i2c_get_clientdata(client);
	//mutex_lock(&fts_data->input_dev->mutex);
	//<ASUS-BSP Robert_He 20170323> remove unused param and mutex ------
	if (fts_read_reg(client, FTS_REG_FW_VER, &fwver) < 0)
		return -1;
	
	
	if (fwver == 255)
		num_read_chars = snprintf(buf, 128,"get tp fw version fail!\n");
	else
	{
		num_read_chars = snprintf(buf, 128, "%02X\n", fwver);
	}

	//<ASUS-BSP Robert_He 20170323> remove unused param and mutex ++++++
	//mutex_unlock(&fts_data->input_dev->mutex);
	//<ASUS-BSP Robert_He 20170323> remove unused param and mutex ------
	return num_read_chars;
}
/************************************************************************
* Name: fts_tpfwver_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_tpfwver_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	/* place holder for future use */
	return -EPERM;
}

//<ASUS-BSP Robert_He 20170323> add capsensor vendor and fwver node ++++++
/************************************************************************
* Name: fts_tpvendor_show
* Brief:  show tp fw vwersion
* Input: device, device attribute, char buf
* Output: no
* Return: char number
***********************************************************************/
static ssize_t fts_tpvendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t num_read_chars = 0;
	u8 fwver = 0;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	//struct fts_ts_data *fts_data = i2c_get_clientdata(client);
	//mutex_lock(&fts_data->input_dev->mutex);
	if (fts_read_reg(client, FTS_REG_FW_VENDOR_ID, &fwver) < 0)
		return -1;
	
	
	if (fwver == 255)
		num_read_chars = snprintf(buf, 128,"get tp fw version fail!\n");
	else
	{
		num_read_chars = snprintf(buf, 128, "%02X\n", fwver);
	}
	
	//mutex_unlock(&fts_data->input_dev->mutex);
	
	return num_read_chars;
}
/************************************************************************
* Name: fts_tpfwver_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_tpvendor_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	/* place holder for future use */
	return -EPERM;
}
//<ASUS-BSP Robert_He 20170323> add capsensor vendor and fwver node ------

/************************************************************************
* Name: fts_tpdriver_version_show
* Brief:  show tp fw vwersion
* Input: device, device attribute, char buf
* Output: no
* Return: char number
***********************************************************************/
static ssize_t fts_tpdriver_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t num_read_chars = 0;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct fts_ts_data *fts_data = i2c_get_clientdata(client);
	mutex_lock(&fts_data->input_dev->mutex);
	
	num_read_chars = snprintf(buf, 128,"%s \n", FTS_DRIVER_INFO);
	
	mutex_unlock(&fts_data->input_dev->mutex);
	
	return num_read_chars;
}
/************************************************************************
* Name: fts_tpdriver_version_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_tpdriver_version_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	/* place holder for future use */
	return -EPERM;
}
/************************************************************************
* Name: fts_tprwreg_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_tprwreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
	return -EPERM;
}
/************************************************************************
* Name: fts_tprwreg_store
* Brief:  read/write register
* Input: device, device attribute, char buf, char count
* Output: print register value
* Return: char count
***********************************************************************/
static ssize_t fts_tprwreg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct fts_ts_data *fts_data = i2c_get_clientdata(client);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg=0;
	u8 regaddr=0xff,regvalue=0xff;
	u8 valbuf[5]={0};

	memset(valbuf, 0, sizeof(valbuf));
	mutex_lock(&fts_data->input_dev->mutex);	
	num_read_chars = count - 1;
	if (num_read_chars != 2) 
	{
		if (num_read_chars != 4) 
		{
			dev_err(dev, "please input 2 or 4 character\n");
			goto error_return;
		}
	}
	memcpy(valbuf, buf, num_read_chars);
	//retval = strict_strtoul(valbuf, 16, &wmreg);
	retval = kstrtoul(valbuf, 16, &wmreg);
	if (0 != retval) 
	{
		dev_err(dev, "%s() - ERROR: Could not convert the given input to a number. The given input was: \"%s\"\n", __FUNCTION__, buf);
		goto error_return;
	}
	if (2 == num_read_chars) 
	{
		/*read register*/
		regaddr = wmreg;
		dev_info(&client->dev,"[focal][test](0x%02x)\n", regaddr);
		if (fts_read_reg(client, regaddr, &regvalue) < 0)
			dev_err(&client->dev,"[Focal] %s : Could not read the register(0x%02x)\n", __func__, regaddr);
		else
			dev_info(&client->dev,"[Focal] %s : the register(0x%02x) is 0x%02x\n", __func__, regaddr, regvalue);
	} 
	else 
	{
		regaddr = wmreg>>8;
		regvalue = wmreg;
		if (fts_write_reg(client, regaddr, regvalue)<0)
			dev_err(dev, "[Focal] %s : Could not write the register(0x%02x)\n", __func__, regaddr);
		else
			dev_dbg(dev, "[Focal] %s : Write 0x%02x into register(0x%02x) successful\n", __func__, regvalue, regaddr);
	}
	error_return:
	mutex_unlock(&fts_data->input_dev->mutex);
	
	return count;
}
/************************************************************************
* Name: fts_fwupdate_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_fwupdate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
	return -EPERM;
}

/************************************************************************
* Name: fts_fwupdate_store
* Brief:  upgrade from *.i
* Input: device, device attribute, char buf, char count
* Output: no
* Return: char count
***********************************************************************/
static ssize_t fts_fwupdate_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	u8 uc_host_fm_ver;
	int i_ret;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct fts_ts_data *fts_data = i2c_get_clientdata(client);
	mutex_lock(&fts_data->input_dev->mutex);
	
	disable_irq(client->irq);
	#if GTP_ESD_PROTECT
		apk_debug_flag = 1;
	#endif
	
	i_ret = fts_ctpm_fw_upgrade_with_i_file(client);
	if (i_ret == 0)
	{
		msleep(300);
		uc_host_fm_ver = fts_ctpm_get_i_file_ver(client);
		dev_dbg(dev, "%s [FTS] upgrade to new version 0x%x\n", __func__, uc_host_fm_ver);
	}
	else
	{
		dev_err(dev, "%s ERROR:[FTS] upgrade failed ret=%d.\n", __func__, i_ret);
	}
	
	#if GTP_ESD_PROTECT
		apk_debug_flag = 0;
	#endif
	enable_irq(client->irq);
	mutex_unlock(&fts_data->input_dev->mutex);
	
	return count;
}
/************************************************************************
* Name: fts_fwupgradeapp_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_fwupgradeapp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
	return -EPERM;
}

/************************************************************************
* Name: fts_fwupgradeapp_store
* Brief:  upgrade from app.bin
* Input: device, device attribute, char buf, char count
* Output: no
* Return: char count
***********************************************************************/
static ssize_t fts_fwupgradeapp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	char fwname[128];
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct fts_ts_data *fts_data = i2c_get_clientdata(client);
	memset(fwname, 0, sizeof(fwname));
	sprintf(fwname, "%s", buf);
	fwname[count-1] = '\0';

	mutex_lock(&fts_data->input_dev->mutex);
	
	disable_irq(client->irq);
	#if GTP_ESD_PROTECT
				apk_debug_flag = 1;
			#endif
	fts_ctpm_fw_upgrade_with_app_file(client, fwname);
	#if GTP_ESD_PROTECT
				apk_debug_flag = 0;
			#endif
	enable_irq(client->irq);
	
	mutex_unlock(&fts_data->input_dev->mutex);
	return count;
}
/************************************************************************
* Name: fts_ftsgetprojectcode_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_getprojectcode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	
	return -EPERM;
}
/************************************************************************
* Name: fts_ftsgetprojectcode_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_getprojectcode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	/* place holder for future use */
	return -EPERM;
}


//#define FTS_CFG_FILEPATH "/data/"

static void fts_sw_reset(struct fts_ts_data *fts_data)
{
    int i;
    u8 auc_i2c_write_buf[10];
    u8 buf[2] = {0};
    u8 reg_val[2] = {0};
    
    for(i =0;i<5;i++) {
        /*********Step 1:Reset  CTPM *****/
        buf[0] = FTS_RST_CMD_REG2;
        buf[1] = FTS_UPGRADE_AA;
        fts_i2c_write_cap_sensors(fts_data->client, buf, 2);
        msleep(fts_data->fts_updateinfo_curr.delay_aa);
        
        buf[0] = FTS_RST_CMD_REG2;
        buf[1] = FTS_UPGRADE_55;
        fts_i2c_write_cap_sensors(fts_data->client, buf, 2);
        msleep(fts_data->fts_updateinfo_curr.delay_55);
        /*********Step 2:Enter upgrade mode *****/
        auc_i2c_write_buf[0] = FTS_UPGRADE_55;
        fts_i2c_write_cap_sensors(fts_data->client, auc_i2c_write_buf, 1);
        auc_i2c_write_buf[0] = FTS_UPGRADE_AA;
        fts_i2c_write_cap_sensors(fts_data->client, auc_i2c_write_buf, 1);
        msleep(10);    
        
        /*********Step 3:check READ-ID***********************/		
		auc_i2c_write_buf[0] = FTS_READ_ID_REG;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =0x00;
		reg_val[0] = 0x00;
		reg_val[1] = 0x00;
		fts_i2c_read_cap_sensors(fts_data->client, auc_i2c_write_buf, 4, reg_val, 2);

		if (reg_val[0] == fts_data->fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_data->fts_updateinfo_curr.upgrade_id_2) 
		{
			dev_info(&fts_data->client->dev,"%s [FTS] : GET CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",
				__func__, reg_val[0], reg_val[1]);
			break;
		} 
		else 
		{
			dev_err(&fts_data->client->dev,"%s [FTS] : GET CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",
				__func__, reg_val[0], reg_val[1]);
		}
    }
    
    /*********Step 7: reset the new FW***********************/
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_write_cap_sensors(fts_data->client, auc_i2c_write_buf, 1);
	//msleep(200);
}


static mm_segment_t oldfs;
static struct file *fts_selftest_file_open(void)
{

	struct file *filp = NULL;
	char filepath[128];
	int err = 0;

	memset(filepath, 0, sizeof(filepath));
//<ASUS-BSP Robert_He 20170315> change filepath for factory test result +++++
	sprintf(filepath, "%s%s", FTS_RESULT_CAP_PATH, "CapSelfResult.csv");
//<ASUS-BSP Robert_He 20170315> change filepath for factory test result -----

	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(filepath, O_WRONLY|O_CREAT, 0644);
	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		return NULL;
	}

	return filp;
}

int fts_selftest_file_write(struct file *file, unsigned char *data, int len)
{

	int ret;

	ret = file->f_op->write(file, data, len, &file->f_pos);

	return ret;
}

void fts_selftest_file_close(struct file *file)
{

	set_fs(oldfs);
	filp_close(file, NULL);
}



static int fts_ReadInIData(char *config_name, char *config_buf)
{
	struct file *pfile = NULL;
	struct inode *inode;
	unsigned long magic;
	off_t fsize;
	char filepath[128];
	loff_t pos;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s%s", FTXXXX_INI_FILEPATH_CONFIG, config_name);
	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(pfile)) {
		pr_err("[FTS][TOUCH_ERR] %s : error occured while opening file %s.\n", __func__, filepath);
		return -EIO;
	}
	inode = pfile->f_dentry->d_inode;
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_read(pfile, config_buf, fsize, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);
	return 0;
}

static int fts_GetInISize(char *config_name)
{
	struct file *pfile = NULL;
	struct inode *inode;
	unsigned long magic;
	off_t fsize = 0;
	char filepath[128];

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s%s", FTXXXX_INI_FILEPATH_CONFIG, config_name);
	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(pfile)) {
		pr_err("[FTS][TOUCH_ERR] %s : error occured while opening file %s.\n", __func__, filepath);
		return -EIO;
	}
	inode = pfile->f_dentry->d_inode;
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	filp_close(pfile, NULL);
	return fsize;
}

static int fts_get_testparam_from_ini(char *config_name)
{
	char *config_data = NULL;
	int file_size;

	file_size = fts_GetInISize(config_name);

	pr_err("[FTS][Touch] %s : inisize = %d\n ", __func__, file_size);
	if (file_size <= 0) {
		pr_err("[FTS][TOUCH_ERR] %s : ERROR : Get firmware size failed\n", __func__);
		return -EIO;
	}

	config_data = kmalloc(file_size + 1, GFP_KERNEL);

	if (fts_ReadInIData(config_name, config_data)) {
		pr_err("[FTS][TOUCH_ERR] %s() - ERROR: request_firmware failed\n", __func__);
		kfree(config_data);
		return -EIO;
	} else {
		pr_info("[FTS][Touch] %s : fts_ReadInIData successful\n", __func__);
	}

	set_param_data_cap_sensors(config_data);

	return 0;
}

/************************************************************************
* Name: fts_ftsselftest_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
int selft_test_result = 0;
static ssize_t fts_ftsselftest_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        if (selft_test_result) {
            dev_info(dev, "[FTS] %s: Selftest FAIL\n", __func__);
        } else {
            dev_info(dev, "[FTS] %s : Selftest PASS\n", __func__);
        }
//<ASUS-BSP Robert_He 20170314> show result for need ++++++
        return sprintf(buf, "%s\n", selft_test_result ? "FAIL" : "PASS");	
//<ASUS-BSP Robert_He 20170314> show result for need ------

}
/************************************************************************
* Name: fts_ftsgetprojectcode_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
struct i2c_client *selftest_client;
static ssize_t fts_ftsselftest_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct fts_ts_data *fts_data = i2c_get_clientdata(client);

	char config_file[128], result_buf[64];
	char *w_buf;
	struct file *w_file;
	int w_len;

	if(fts_data->IC_ID != ID_FT3267)
	{
		dev_err(&client->dev,"[FTS][TOUCH_ERR] This code base only support FT3267.\n");
		return count;
	}	

	selftest_client = client;
	//mutex_lock(&g_device_mutex);

        disable_irq(client->irq);

        selft_test_result = 0;

        memset(config_file, 0, sizeof(config_file));
        sprintf(config_file, "%s", buf);
        config_file[count-1] = '\0';
        
        if (fts_get_testparam_from_ini(config_file) < 0) {
        }        

        if (fts_get_testparam_from_ini(config_file) < 0) {
            dev_err(&client->dev,"[FTS][TOUCH_ERR] %s : get testparam from ini failure\n", __func__);
        } else {
            dev_info(&client->dev, "[FTS][Touch] %s : tp test Start...\n", __func__);

            if (start_test_tp_cap_sensors()) {
                dev_info(&client->dev, "[FTS][Touch] %s : tp test pass\n", __func__);
                selft_test_result = 0;
            } else {
                dev_err(&client->dev, "[FTS][Touch] %s : tp test failure\n", __func__);
                selft_test_result = 1;
            }

            /*for (i = 0; i < 3; i++) {
                if (cap_i2c_write(rmi4_data, 0x00, 1) >= 0)
                    break;
                else
                    msleep(200);
            }*/

            w_file = fts_selftest_file_open();
            if (!w_file)
                dev_err(&client->dev,"[FTS][Touch] %s : Open log file fail !\n", __func__);
            else{
                w_buf =kmalloc(1024*80,  GFP_KERNEL);
                if (!w_buf)
                    dev_err(&client->dev,"[FTS][Touch] %s : allocate memory fail !\n", __func__);
                else{
                    w_len = get_test_data_cap_sensors(w_buf);
                    fts_selftest_file_write(w_file, w_buf, w_len);
                    w_len = sprintf(result_buf, "[FTS] : Selftest %s\n", selft_test_result ? "FAIL" : "PASS");
                    fts_selftest_file_write(w_file, result_buf, w_len);

                    kfree(w_buf);
                }
                fts_selftest_file_close(w_file);
                FreeStoreAllData();
            }

            free_test_param_data();
        }
        fts_sw_reset(fts_data);
        enable_irq(client->irq);
        //mutex_unlock(&g_device_mutex);
        
    return count;
}

//add by Holt 20160825+
/************************************************************************
* Name: fts_buttonmode_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_buttonmode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct fts_ts_data *data = i2c_get_clientdata(client);
	
	return snprintf(buf,PAGE_SIZE, "%u\n",data->buttonmode);
}
/************************************************************************
* Name: fts_buttonmode_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_buttonmode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct fts_ts_data *data = i2c_get_clientdata(client);
	unsigned int input;

	if(sscanf(buf,"%u",&input) != 1)
		return -EINVAL;
	data->buttonmode = input;
	dev_info(&client->dev, "[Focal][Virtual] button_mode: %d\n",data->buttonmode);
	return count;
}
//add by Holt 20160825-



/************************************************************************
* Name: fts_glovemode_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_glovemode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct fts_ts_data *data = i2c_get_clientdata(client);
	
	return snprintf(buf,PAGE_SIZE, "%u\n",data->glovemode);
}
/************************************************************************
* Name: fts_glovemode_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_glovemode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct fts_ts_data *data = i2c_get_clientdata(client);
	unsigned int input;
	u8 reg_val[2] = {0};

	if(sscanf(buf,"%u",&input) != 1)
		return -EINVAL;
	data->glovemode = input;

        reg_val[0] = 0xC0;				//ID_G_GLOVE_MODE_EN
        reg_val[1] = data->glovemode;
        fts_i2c_write_cap_sensors(data->client, reg_val, 2);	

	dev_info(&client->dev, "[Focal][Virtual] glovemode: %d\n",data->glovemode);
	return count;
}


/************************************************************************
* Name: fts_touch_status_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_touch_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	int touch_status = 1,err;
	unsigned char reg_addr,reg_value;
	/* check the controller id */
	reg_addr = FTS_REG_ID;
	err = fts_i2c_read_cap_sensors(client, &reg_addr, 1, &reg_value, 1);
	if (err < 0) {
		dev_err(&client->dev, "[touch_status] version read failed");
		touch_status = 0;
	}	
	
	return snprintf(buf,PAGE_SIZE, "%u\n",touch_status);
}
/************************************************************************
* Name: fts_touch_status_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
/*
static ssize_t fts_touch_status_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	return -EPERM;
}
*/
/************************************************************************
* Name: fts_touch_function_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_touch_function_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);	
	struct fts_ts_data *data = i2c_get_clientdata(client);

	return snprintf(buf,PAGE_SIZE, "%u\n",data->touch_enable);
}
/************************************************************************
* Name: fts_touch_function_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_touch_function_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct fts_ts_data *data = i2c_get_clientdata(client);
	unsigned int input;

	if(sscanf(buf,"%u",&input) != 1)
		return -EINVAL;
	data->touch_enable = input;
	dev_info(&client->dev, "[Focal][Virtual] touch_enable: %d\n",data->touch_enable);
	return count;	
}

/************************************************************************
* Name: fts_vendorid_show
* Brief:  show tp fw vwersion
* Input: device, device attribute, char buf
* Output: no
* Return: char number
***********************************************************************/
static ssize_t fts_vendorid_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t num_read_chars = 0;

	//if(asus_lcd_id == 3)
		num_read_chars = snprintf(buf, 128,"EDO\n");
	//else
	//	num_read_chars = snprintf(buf, 128,"Unkown\n");

	return num_read_chars;
}
//<ASUS-BSP Robert_He 20170410> add capsensor vkled node ++++++
#ifdef ZD552KL_PHOENIX
#ifdef ASUS_FACTORY_BUILD
static ssize_t focal_cap_led_show(struct device *dev,struct device_attribute *attr,char *buf)
{
	return 0;
}
static ssize_t focal_cap_led_store(struct device *dev,struct device_attribute *attr,const char *buf,size_t count)
{
	if (strnicmp(buf, "1", 1)  == 0)
    {
       set_button_backlight(true);
	   msleep(10);
    }
	else
	{
	   set_button_backlight(false);
	   msleep(10);
	}
	return count;
}
#endif
#endif


/****************************************/

static DEVICE_ATTR(ftscapvendor, S_IRUGO|S_IWUSR, fts_tpvendor_show, fts_tpvendor_store);
#ifdef ZD552KL_PHOENIX
#ifdef ASUS_FACTORY_BUILD
static DEVICE_ATTR(ftscapled, S_IRUGO|S_IWUSR, focal_cap_led_show, focal_cap_led_store);

#endif
#endif
//<ASUS-BSP Robert_He 20170410> add capsensor vkled node ------

/* sysfs */
/*get the fw version
*example:cat ftstpfwver
*/
static DEVICE_ATTR(ftstpfwver, S_IRUGO|S_IWUSR, fts_tpfwver_show, fts_tpfwver_store);
//<ASUS-BSP Robert_He 20170323> add capsensor vendor and fwver node ------

static DEVICE_ATTR(ftstpdriverver, S_IRUGO|S_IWUSR, fts_tpdriver_version_show, fts_tpdriver_version_store);
/*upgrade from *.i
*example: echo 1 > ftsfwupdate
*/
static DEVICE_ATTR(ftsfwupdate, S_IRUGO|S_IWUSR, fts_fwupdate_show, fts_fwupdate_store);
/*read and write register
*read example: echo 88 > ftstprwreg ---read register 0x88
*write example:echo 8807 > ftstprwreg ---write 0x07 into register 0x88
*
*note:the number of input must be 2 or 4.if it not enough,please fill in the 0.
*/
static DEVICE_ATTR(ftstprwreg, S_IRUGO|S_IWUSR, fts_tprwreg_show, fts_tprwreg_store);
/*upgrade from app.bin
*example:echo "*_app.bin" > ftsfwupgradeapp
*/
static DEVICE_ATTR(ftsfwupgradeapp, S_IRUGO|S_IWUSR, fts_fwupgradeapp_show, fts_fwupgradeapp_store);
static DEVICE_ATTR(ftsgetprojectcode, S_IRUGO|S_IWUSR, fts_getprojectcode_show, fts_getprojectcode_store);
static DEVICE_ATTR(ftsselftest, S_IRUGO|S_IWUSR, fts_ftsselftest_show, fts_ftsselftest_store);
static DEVICE_ATTR(touch_status, S_IRUGO, fts_touch_status_show, NULL);
static DEVICE_ATTR(touch_function, S_IRUGO|S_IWUSR, fts_touch_function_show, fts_touch_function_store);
static DEVICE_ATTR(ftsvendorid, S_IRUGO, fts_vendorid_show, NULL);
//add by Holt 20160825+
/*
	buttonmode = 0 back,home,recent
	buttonmode = 1 recent,home,back
*/

static DEVICE_ATTR(buttonmode, S_IRUGO|S_IWUSR, fts_buttonmode_show, fts_buttonmode_store);
static DEVICE_ATTR(glovemode, S_IRUGO|S_IWUSR, fts_glovemode_show, fts_glovemode_store);
//add by Holt 20160825-

/*add your attr in here*/
static struct attribute *fts_attributes[] = {
//<ASUS-BSP Robert_He 20170323> add capsensor vendor and fwver node and support two project++++++
//<ASUS-BSP Robert_He 20170410> add capsensor vkled node ++++++
#ifdef ZD552KL_PHOENIX
#ifdef ASUS_FACTORY_BUILD
    &dev_attr_ftscapled.attr,
#endif
#endif
//<ASUS-BSP Robert_He 20170410> add capsensor vkled node ------

	&dev_attr_ftscapvendor.attr,
	&dev_attr_ftstpfwver.attr,
//<ASUS-BSP Robert_He 20170323> add capsensor vendor and fwver node ++++++
	&dev_attr_ftstpdriverver.attr,
	&dev_attr_ftsfwupdate.attr,
	&dev_attr_ftstprwreg.attr,
	&dev_attr_ftsfwupgradeapp.attr,
	&dev_attr_ftsgetprojectcode.attr,
//add by Holt 20160825+
	&dev_attr_buttonmode.attr,
	&dev_attr_ftsselftest.attr,
	&dev_attr_touch_status.attr,
	&dev_attr_touch_function.attr,
	&dev_attr_ftsvendorid.attr,
	&dev_attr_glovemode.attr,
//add by Holt 20160825-

	NULL
};

static struct attribute_group fts_attribute_group = {
	.attrs = fts_attributes
};

/************************************************************************
* Name: fts_create_sysfs_cap_sensors
* Brief:  create sysfs for debug
* Input: i2c info
* Output: no
* Return: success =0
***********************************************************************/
int fts_create_sysfs_cap_sensors(struct i2c_client * client)
{
	int err;
	
	err = sysfs_create_group(&client->dev.kobj, &fts_attribute_group);
	if (0 != err) 
	{
		dev_err(&client->dev, "%s() - ERROR: sysfs_create_group() failed.\n", __func__);
		sysfs_remove_group(&client->dev.kobj, &fts_attribute_group);
		return -EIO;
	} 
	/*else 
	{
		dev_info(&client->dev, "sysfs_create_group() succeeded.\n");
	}*/
	return err;
}
/************************************************************************
* Name: fts_remove_sysfs_cap_sensors
* Brief:  remove sys
* Input: i2c info
* Output: no
* Return: no
***********************************************************************/
int fts_remove_sysfs_cap_sensors(struct i2c_client * client)
{
	sysfs_remove_group(&client->dev.kobj, &fts_attribute_group);
	return 0;
}
