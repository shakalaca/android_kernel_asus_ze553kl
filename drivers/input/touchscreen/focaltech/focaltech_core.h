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

#ifndef __LINUX_FTS_H__
#define __LINUX_FTS_H__
 /*******************************************************************************
*
* File Name: focaltech.c
*
* Author: mshl
*
* Created: 2014-09
*
* Modify by mshl on 2015-07-06
*
* Abstract:
*
* Reference:
*
*******************************************************************************/

/*******************************************************************************
* Included header files
*******************************************************************************/
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
//#include <linux/sensors.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mount.h>
#include <linux/netdevice.h>
#include <linux/unistd.h>
#include <linux/ioctl.h>
//#include "ft_gesture_lib.h"


/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define FTS_DRIVER_INFO  "Qualcomm_Ver 1.3.1 2015-07-06"
#define FT5X06_ID		0x55
#define FT5X16_ID		0x0A
#define FT5X36_ID		0x14
#define FT6X06_ID		0x06
#define FT6X36_ID       	0x36

#define FT5316_ID		0x0A
#define FT5306I_ID		0x55

#define LEN_FLASH_ECC_MAX 					0xFFFE

#define FTS_MAX_POINTS                        10

#define FTS_INFO_MAX_LEN		512
#define FTS_FW_NAME_MAX_LEN	50

#define FTS_REG_ID		0xA3
#define FTS_REG_FW_VER		0xA6
#define FTS_REG_FW_VENDOR_ID	0xA8
#define FTS_REG_POINT_RATE					0x88

#define FTS_FACTORYMODE_VALUE	0x40
#define FTS_WORKMODE_VALUE	0x00

//#define CONFIG_TOUCHSCREEN_FTS_PSENSOR
#define MSM_NEW_VER	//cotrol new platform


#define FTS_STORE_TS_INFO(buf, id, name, max_tch, group_id, fw_vkey_support, \
			fw_name, fw_maj, fw_min, fw_sub_min) \
			snprintf(buf, FTS_INFO_MAX_LEN, \
				"controller\t= focaltech\n" \
				"model\t\t= 0x%x\n" \
				"name\t\t= %s\n" \
				"max_touches\t= %d\n" \
				"drv_ver\t\t= %s\n" \
				"group_id\t= 0x%x\n" \
				"fw_vkey_support\t= %s\n" \
				"fw_name\t\t= %s\n" \
				"fw_ver\t\t= %d.%d.%d\n", id, name, \
				max_tch, FTS_DRIVER_INFO, group_id, \
				fw_vkey_support, fw_name, fw_maj, fw_min, \
				fw_sub_min)
				

#define FTS_DBG_EN 1
#if FTS_DBG_EN
#define FTS_DBG(fmt, args...) 				printk(KERN_INFO "[FTS]" fmt, ## args)
#else
#define FTS_DBG(fmt, args...) 				do{}while(0)
#endif

//Function Switchs: define to open,  comment to close
#define FTS_GESTRUE_EN 0
#define GTP_ESD_PROTECT 0
#define FTS_APK_DEBUG
#define FTS_SYSFS_DEBUG
#define FTS_CTL_IIC

#define FTS_AUTO_UPGRADE

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define FTS_META_REGS		3
#define FTS_ONE_TCH_LEN		6
#define FTS_TCH_LEN(x)		(FTS_META_REGS + FTS_ONE_TCH_LEN * x)

#define FTS_PRESS		0x7F
#define FTS_MAX_ID		0x0F
#define FTS_TOUCH_X_H_POS	3
#define FTS_TOUCH_X_L_POS	4
#define FTS_TOUCH_Y_H_POS	5
#define FTS_TOUCH_Y_L_POS	6
#define FTS_TOUCH_PRE_POS	7
#define FTS_TOUCH_AREA_POS	8
#define FTS_TOUCH_POINT_NUM		2
#define FTS_TOUCH_EVENT_POS	3
#define FTS_TOUCH_ID_POS		5

#define FTS_TOUCH_DOWN		0
#define FTS_TOUCH_UP		1
#define FTS_TOUCH_CONTACT	2

#define POINT_READ_BUF	(3 + FTS_ONE_TCH_LEN * FTS_MAX_POINTS)

/*register address*/
#define FTS_REG_DEV_MODE		0x00
#define FTS_DEV_MODE_REG_CAL	0x02

#define FTS_REG_PMODE		0xA5

#define FTS_REG_POINT_RATE	0x88
#define FTS_REG_THGROUP		0x80

/* power register bits*/
#define FTS_PMODE_ACTIVE		0x00
#define FTS_PMODE_MONITOR	0x01
#define FTS_PMODE_STANDBY	0x02
#define FTS_PMODE_HIBERNATE	0x03

#define FTS_STATUS_NUM_TP_MASK	0x0F

#define FTS_VTG_MIN_UV		2600000
#define FTS_VTG_MAX_UV		3300000
#define FTS_I2C_VTG_MIN_UV		1800000
#define FTS_I2C_VTG_MAX_UV		1800000

#define FTS_COORDS_ARR_SIZE	4
#define MAX_BUTTONS		4

#define FTS_8BIT_SHIFT		8
#define FTS_4BIT_SHIFT		4

/* psensor register address*/
#define FTS_REG_PSENSOR_ENABLE	0xB0
#define FTS_REG_PSENSOR_STATUS	0x01

/* psensor register bits*/
#define FTS_PSENSOR_ENABLE_MASK	0x01
#define FTS_PSENSOR_STATUS_NEAR	0xC0
#define FTS_PSENSOR_STATUS_FAR	0xE0
#define FTS_PSENSOR_FAR_TO_NEAR	0
#define FTS_PSENSOR_NEAR_TO_FAR	1
#define FTS_PSENSOR_ORIGINAL_STATE_FAR	1
#define FTS_PSENSOR_WAKEUP_TIMEOUT	500

#define KEY_BACK_X_AREA 80
#define KEY_BACK_Y_AREA 900

#define KEY_HOME_X_AREA 240
#define KEY_HOME_Y_AREA 900

#define KEY_MENU_X_AREA 400
#define KEY_MENU_Y_AREA 900
/*
#define KEY_BACK_MAX_X_AREA 160
#define KEY_BACK_MIN_X_AREA 160
#define KEY_BACK_MAX_Y_AREA 3000
#define KEY_BACK_MIN_Y_AREA 3000

#define KEY_HOME_MAX_X_AREA 360
#define KEY_HOME_MIN_X_AREA 360
#define KEY_HOME_MAX_Y_AREA 3000
#define KEY_HOME_MIN_Y_AREA 3000

#define KEY_MENU_MAX_X_AREA 560
#define KEY_MENU_MIN_X_AREA 560
#define KEY_MENU_MAX_Y_AREA 3000
#define KEY_MENU_MIN_Y_AREA 3000
*/

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/


/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define FTS_REG_FW_MAJ_VER	0xB1
#define FTS_REG_FW_MIN_VER	0xB2
#define FTS_REG_FW_SUB_MIN_VER	0xB3
#define FTS_FW_MIN_SIZE		8
#define FTS_FW_MAX_SIZE		(54 * 1024)
/* Firmware file is not supporting minor and sub minor so use 0 */
#define FTS_FW_FILE_MAJ_VER(x)	((x)->data[(x)->size - 2])
#define FTS_FW_FILE_MIN_VER(x)	0
#define FTS_FW_FILE_SUB_MIN_VER(x) 0
#define FTS_FW_FILE_VENDOR_ID(x)	((x)->data[(x)->size - 1])
#define FTS_FW_FILE_MAJ_VER_FT6X36(x)	((x)->data[0x10a])
#define FTS_FW_FILE_VENDOR_ID_FT6X36(x)	((x)->data[0x108])
#define FTS_MAX_TRIES		5
#define FTS_RETRY_DLY		20
#define FTS_MAX_WR_BUF		10
#define FTS_MAX_RD_BUF		2
#define FTS_FW_PKT_META_LEN	6
#define FTS_FW_PKT_DLY_MS	20
#define FTS_FW_LAST_PKT		0x6ffa
#define FTS_EARSE_DLY_MS		100
#define FTS_55_AA_DLY_NS		5000
#define FTS_CAL_START		0x04
#define FTS_CAL_FIN		0x00
#define FTS_CAL_STORE		0x05
#define FTS_CAL_RETRY		100
#define FTS_REG_CAL		0x00
#define FTS_CAL_MASK		0x70
#define FTS_BLOADER_SIZE_OFF	12
#define FTS_BLOADER_NEW_SIZE	30
#define FTS_DATA_LEN_OFF_OLD_FW	8
#define FTS_DATA_LEN_OFF_NEW_FW	14
#define FTS_FINISHING_PKT_LEN_OLD_FW	6
#define FTS_FINISHING_PKT_LEN_NEW_FW	12
#define FTS_MAGIC_BLOADER_Z7	0x7bfa
#define FTS_MAGIC_BLOADER_LZ4	0x6ffa
#define FTS_MAGIC_BLOADER_GZF_30	0x7ff4
#define FTS_MAGIC_BLOADER_GZF	0x7bf4
#define FTS_REG_ECC		0xCC
#define FTS_RST_CMD_REG2		0xBC
#define FTS_READ_ID_REG		0x90
#define FTS_ERASE_APP_REG	0x61
#define FTS_ERASE_PARAMS_CMD	0x63
#define FTS_FW_WRITE_CMD		0xBF
#define FTS_REG_RESET_FW		0x07
#define FTS_RST_CMD_REG1		0xFC
#define FTS_FACTORYMODE_VALUE				0x40
#define FTS_WORKMODE_VALUE				0x00
#define FTS_APP_INFO_ADDR					0xd7f8

#define	BL_VERSION_LZ4        0
#define   BL_VERSION_Z7        1
#define   BL_VERSION_GZF        2
#define   FTS_REG_FW_VENDOR_ID 0xA8

#define FTS_PACKET_LENGTH      	128
#define FTS_SETTING_BUF_LEN      	128

#define FTS_UPGRADE_LOOP		30
#define FTS_MAX_POINTS_2                        		2
#define FTS_MAX_POINTS_5                        		5
#define FTS_MAX_POINTS_10                        		10
#define AUTO_CLB_NEED                           		1
#define AUTO_CLB_NONEED                         		0
#define FTS_UPGRADE_AA		0xAA
#define FTS_UPGRADE_55		0x55
#define HIDTOI2C_DISABLE					0
//<ASUS-BSP Robert_He 20170314>change file path for factory tool +++++
#define FTXXXX_INI_FILEPATH_CONFIG "/system/bin/"
#define FTS_RESULT_CAP_PATH "/data/data/"
//<ASUS-BSP Robert_He 20170314>change file path for factory tool -----

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/

//add by Holt++++++++++++++++++++++++++++++++++++++++++++++

#define ID_UNKONE 0
#define ID_FT3517 1
#define ID_FT3267 2

#define TOUCH_IC_NAME 	"ft3517"
#define TOUCH_IC_NAME_LEN 6

#define VIRTUAL_IC_NAME "ft3267"
#define VIRTUAL_IC_NAME_LEN 6

struct fts_NameList
{
	const char *InputName;
	const char *Pinctrl_Active;
	const char *Pinctrl_Suspend;
	const char *Pinctrl_Release;
	const char *IrqWorkQueueName;
	const char *fwWorkQueueName;
	const char *IrqName;
	const char *DebugDirName;
	const char *ResetGpio;
	const char *IrqGpio;
	const char *ProcName;	// Proc node
	const char *ClassName;	// sys/class/
	const char *DevName;	// dev/
	const char *pmWorkQueueName;
};

//add by Holt--------------------------------------------

struct fts_rw_i2c_dev 
{
	struct cdev cdev;
	struct mutex fts_rw_i2c_mutex;
	struct i2c_client *client;
};

struct fts_Upgrade_Info 
{
        u8 CHIP_ID;
        u8 TPD_MAX_POINTS;
        u8 AUTO_CLB;
	 u16 delay_aa;						/*delay of write FT_UPGRADE_AA */
	 u16 delay_55;						/*delay of write FT_UPGRADE_55 */
	 u8 upgrade_id_1;					/*upgrade id 1 */
	 u8 upgrade_id_2;					/*upgrade id 2 */
	 u16 delay_readid;					/*delay of read id */
	 u16 delay_erase_flash; 				/*delay of earse flash*/
};

struct fts_ts_platform_data {
	struct fts_Upgrade_Info info;
	const char *name;
	const char *fw_name;
	u32 irqflags;
	u32 irq_gpio;
	u32 irq_gpio_flags;
	u32 reset_gpio;
	u32 reset_gpio_flags;
	u32 family_id;
	u32 x_max;
	u32 y_max;
	u32 x_min;
	u32 y_min;
	u32 panel_minx;
	u32 panel_miny;
	u32 panel_maxx;
	u32 panel_maxy;
	u32 group_id;
	u32 hard_rst_dly;
	u32 soft_rst_dly;
	u32 num_max_touches;
	bool fw_vkey_support;
	bool no_force_update;
	bool i2c_pull_up;
	bool ignore_id_check;
	bool psensor_support;
	int (*power_init) (bool);
	int (*power_on) (bool);
};

struct ts_event {
	u16 au16_x[FTS_MAX_POINTS];	/*x coordinate */
	u16 au16_y[FTS_MAX_POINTS];	/*y coordinate */
	u16 pressure[FTS_MAX_POINTS];
	u8 au8_touch_event[FTS_MAX_POINTS];	/*touch event:
					0 -- down; 1-- up; 2 -- contact */
	u8 au8_finger_id[FTS_MAX_POINTS];	/*touch ID */
	u8 area[FTS_MAX_POINTS];
	u8 touch_point;
	u8 point_num;
};

struct fts_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct ts_event event;
	const struct fts_ts_platform_data *pdata;
	struct fts_psensor_platform_data *psensor_pdata;
	struct work_struct 	touch_event_work;
	struct workqueue_struct *ts_workqueue;
	struct regulator *vdd;
	struct regulator *vcc_i2c;
	char fw_name[FTS_FW_NAME_MAX_LEN];
	bool loading_fw;
	u8 family_id;
	struct dentry *dir;
	u16 addr;
	bool suspended;
	char *ts_info;
	u8 *tch_data;
	u32 tch_data_len;
	u8 fw_ver[3];
	u8 fw_vendor_id;
	int touchs;
#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
#ifdef MSM_NEW_VER
	struct pinctrl *ts_pinctrl;
	struct pinctrl_state *pinctrl_state_active;
	struct pinctrl_state *pinctrl_state_suspend;
	struct pinctrl_state *pinctrl_state_release;
#endif

//Holt_Hu 20160913+

	unsigned int buttonmode;
	unsigned int touch_enable;
	unsigned int glovemode;
//for diffrent function IC
	unsigned int IC_ID;
	struct fts_NameList* fts_NameList_curr;

//for main function, report data buff
        unsigned int buf_count_add;
        unsigned int buf_count_neg;
	u8 buf_touch_data[30*POINT_READ_BUF];

	struct workqueue_struct *suspend_resume_wq;
	struct work_struct suspend_work;
	struct work_struct resume_work;

//for updatefw(focaltech_flash.c)
	struct fts_Upgrade_Info fts_updateinfo_curr;
	struct workqueue_struct *update_fw_wq;
	struct delayed_work update_fw_work;
	unsigned char *CTPM_FW;
	unsigned int CTPM_FW_SIZE;

//for apk_debug /proc/node (focaltech_ex_fun.c)
	struct proc_dir_entry *fts_proc_entry;

//for debug /dev/node (focaltech_ctl.c)
	int fts_rw_iic_drv_major;
	struct class *fts_class;
	struct fts_rw_i2c_dev *fts_rw_i2c_dev_tt;

//Holt_Hu 20160913-
//<ASUS-BSP Robert_He 20170405> add capsensor vkled function ++++++
#ifdef ZD552KL_PHOENIX
#ifdef ASUS_FACTORY_BUILD
	struct delayed_work led_delay_work;
	struct workqueue_struct *led_wq;
#endif
#endif
//<ASUS-BSP Robert_He 20170405> add capsensor vkled function ------


};

#ifdef CONFIG_TOUCHSCREEN_FTS_PSENSOR
struct fts_psensor_platform_data {
	struct input_dev *input_psensor_dev;
	struct sensors_classdev ps_cdev;
	int tp_psensor_opened;
	char tp_psensor_data; /* 0 near, 1 far */
	struct fts_ts_data *data;
};
#endif

/*******************************************************************************
* Static variables
*******************************************************************************/


/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
extern struct fts_Upgrade_Info fts_updateinfo_curr;
extern struct i2c_client *fts_i2c_client;
extern struct fts_ts_data *fts_wq_data;
extern struct input_dev *fts_input_dev;

static DEFINE_MUTEX(i2c_rw_access);

//Getstre functions
extern int fts_Gesture_init(struct input_dev *input_dev);
extern int fts_read_Gestruedata(void);
extern int fetch_object_sample(unsigned char *buf,short pointnum);
extern void init_para(int x_pixel,int y_pixel,int time_slot,int cut_x_pixel,int cut_y_pixel);

//upgrade functions
extern void fts_update_fw_vendor_id_cap_sensors(struct fts_ts_data *data);
extern void fts_update_fw_ver_cap_sensors(struct fts_ts_data *data);
extern void fts_get_upgrade_array(struct fts_ts_data *data);
extern int fts_ctpm_auto_upgrade_cap_sensors(struct i2c_client *client);
extern int fts_fw_upgrade(struct device *dev, bool force);
extern int fts_ctpm_auto_clb_cap_sensors(struct i2c_client *client);
extern int fts_ctpm_fw_upgrade_with_app_file(struct i2c_client *client, char *firmware_name);
extern int fts_ctpm_fw_upgrade_with_i_file(struct i2c_client *client);
extern int fts_ctpm_get_i_file_ver(struct i2c_client *client);

#ifdef FTS_APK_DEBUG
//Apk and functions
extern int fts_create_apk_debug_channel_cap_sensors(struct i2c_client * client);
extern void fts_release_apk_debug_channel_cap_sensors(struct i2c_client * client);
#endif

//ADB functions
extern int fts_create_sysfs_cap_sensors(struct i2c_client *client);
extern int fts_remove_sysfs_cap_sensors(struct i2c_client *client);

//char device for old apk
extern int fts_rw_iic_drv_init(struct i2c_client *client);
extern void  fts_rw_iic_drv_exit(struct i2c_client *client);

//Base functions
extern int fts_i2c_read_cap_sensors(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen);
extern int fts_i2c_write_cap_sensors(struct i2c_client *client, char *writebuf, int writelen);
extern int fts_read_reg(struct i2c_client *client, u8 addr, u8 *val);
extern int fts_write_reg(struct i2c_client *client, u8 addr, const u8 val);

/*******************************************************************************
* Static function prototypes
*******************************************************************************/

/*******************************************************************************
* Asus Project Parameter
*******************************************************************************/
//extern int asus_lcd_id;
#endif
