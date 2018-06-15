/*
 * Copyright (C) Fuzhou Rockchip Electronics Co.Ltd
 * Author: Tusson <dusong@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
//asus bsp ralf>>
#include "../common/msm_camera_io_util.h"
#include "spi2apb.h"
#include <linux/wakelock.h>//ASUS_BSP Zhengwei "prevent enter suspend when resuming"
//ASUS_BSP +++ Zhengwei "late resume preisp"
#ifdef ZD552KL_PHOENIX
#include <linux/notifier.h>
#include <linux/fb.h>
static int fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data);
#endif
//ASUS_BSP --- Zhengwei "late resume preisp"
extern int msm_camera_pinctrl_init(
	struct msm_pinctrl_info *sensor_pctrl, struct device *dev);
extern int msm_camera_clk_enable(struct device *dev,
		struct msm_cam_clk_info *clk_info,
		struct clk **clk_ptr, int num_clk, int enable);
extern int msm_camera_put_clk_info_internal(struct device *dev,
				struct msm_cam_clk_info **clk_info,
				struct clk ***clk_ptr, int cnt);
extern int msm_camera_get_clk_info_internal(struct device *dev,
			struct msm_cam_clk_info **clk_info,
			struct clk ***clk_ptr,
			size_t *num_clk);

extern int asus_hw_id;

//asus bsp ralf<<
#include "spi-rkpreisp.h"
#include "spi2apb.h"
#include "msg-queue.h"
#include "isp-fw.h"
#include "ap-i2c.h"

#define DEBUG_DUMP_ALL_SEND_RECV_MSG 0

#define DEBUG_MSG_LOOP_TEST 0

#define INVALID_ID -1

#define PREISP_MCLK_RATE 24*1000*1000ul

#define PREISP_WAKEUP_TIMEOUT_MS 100
#define PREISP_REQUEST_SLEEP_TIMEOUT_MS 100

#ifdef ZD552KL_PHOENIX
#define PREISP_MAX_CORE_VOLTAGE    1200000
#define PREISP_ZZHDR_CORE_VOLTAGE  1150000
#define PREISP_NORMAL_CORE_VOLTAGE 1100000
uint32_t g_cur_core_voltage=PREISP_NORMAL_CORE_VOLTAGE;
extern int ncp6335d_set_voltage_for_preisp(int voltage);
#endif

typedef struct {
    int8_t id;
    struct msg_queue q;
    struct list_head list;
    wait_queue_head_t wait;
    void *private_data;
} preisp_client;

typedef struct {
    struct mutex mutex;
    struct list_head list;
} preisp_client_list;

struct spi_rk_preisp_data {
    struct miscdevice misc;
    struct spi_device *spi;
    struct device *dev;
    preisp_client_list clients;
    int reset_gpio;
    int reset_active;
    int irq_gpio;
	int irq_active;
    int irq;
	//asus bsp ralf>>
	bool do_force_sleep;
#ifdef ZD552KL_PHOENIX
	bool do_force_wakeup;
#endif
	int vio_en_gpio;
	int snapshot_gpio;
	int snapshot_active;
	struct camera_vreg_t *cam_vreg;
	int vdd_gpio;
	struct msm_cam_clk_info *clk_info;
	struct clk **clk_ptr;
	size_t num_clk;
	struct msm_pinctrl_info pinctrl_info;
	int pinctrl_status;
	int power_state;
	bool is_fw_loaded;
	bool is_core_supply_enabled;
	bool is_ddr_supply_enabled;
#ifdef ZD552KL_PHOENIX
	bool is_ext_buck_supply_enabled;
	bool is_standby_sleep_mode;
	bool is_bypass_sleep_mode;
	bool is_suspended;
	char fw_version[32];
#endif
	int dsp_rt_status;
	bool is_suspend_sleep;
	int sleep_mode;
	//asus bsp ralf<<
	int sleepst_gpio;
	int sleepst_active;
    int sleepst_irq;
    int wakeup_gpio;
    int wakeup_active;
    int powerdown_gpio;
    int powerdown_active;
    struct clk *mclk;
    atomic_t power_on_cnt;
    atomic_t wake_sleep_cnt;
    struct mutex send_msg_lock;
    struct mutex power_lock;
    struct mutex wake_sleep_lock;
#ifdef ZD552KL_PHOENIX
    struct mutex resume_lock;//ASUS_BSP Zhengwei "synchronize resume/power up"
#endif
    //ASUS_BSP +++ Zhengwei "prevent enter suspend when resuming"
    struct wake_lock resume_wake_lock;
    bool is_resume_processing;
    //ASUS_BSP --- Zhengwei "prevent enter suspend when resuming"
#ifdef ZD552KL_PHOENIX
    struct notifier_block fb_notif;//ASUS_BSP Zhengwei "late resume preisp"
#endif
    uint32_t max_speed_hz;
    uint32_t min_speed_hz;
    uint32_t fw_speed_hz;
    uint32_t fw_nowait_mode;
    int log_level;
    int sleep_state_flag;
};

struct spi_rk_preisp_data *g_preisp_data;

enum {
    AUTO_ARG_TYPE_STR,
    AUTO_ARG_TYPE_INT32,
};

struct auto_arg {
    int type;
    union {
        int32_t m_int32;
        const char * m_str;
    };
};

struct auto_args {
    int argc;
    struct auto_arg *argv;
};
enum sleep_rt_status{
	DSP_STATUS_RUNNING,
	DSP_STATUS_PROCESSING,
	DSP_STATUS_SLEEPING,
	DSP_STATUS_POWER_OFF,
	};
int asus_power_config( struct spi_rk_preisp_data *data,int type,int value);
void spi_cs_set_value(struct spi_rk_preisp_data *pdata, int value);
void preisp_set_spi_speed(struct spi_rk_preisp_data *pdata, uint32_t hz);
void rkpreisp_hw_init(struct spi_device *spi);
static int spi_rk_preisp_remove(struct spi_device *spi);
int rkpreisp_set_log_level(struct spi_rk_preisp_data *pdata, int level);
int rkpreisp_request_sleep(struct spi_rk_preisp_data *pdata, int32_t mode);
int rkpreisp_power_on_dsp(int is_download_fw);
int rkpreisp_power_off_dsp(void);
static int rkpreisp_power_on(struct spi_rk_preisp_data *pdata,int is_download_fw);
static int rkpreisp_power_off(struct spi_rk_preisp_data *pdata);
static int asus_core_power_supply_control(struct spi_rk_preisp_data *data,int value);
static int asus_ddr_power_supply_control(struct spi_rk_preisp_data *data,int value);

#ifdef ZD552KL_PHOENIX
static int asus_ext_buck_power_supply_control(struct spi_rk_preisp_data *data,int value);
static int asus_vio_en_gpio_control(struct spi_rk_preisp_data *data,int value);
static int asus_sleep_power_operation(struct spi_rk_preisp_data *data);
static int asus_wakeup_power_operation(struct spi_rk_preisp_data *data);

typedef struct
{
	struct spi_rk_preisp_data *pdata;
	struct work_struct work;
}sleep_work_t;
static struct workqueue_struct *sleep_wq;
static sleep_work_t sleep_work;
static void sleep_operation(struct work_struct *work);

#endif
int rkpreisp_wakeup(struct spi_rk_preisp_data *pdata);






//asus bsp ralf>>
//struct camera_vreg_t *cam_vreg
int preisp_config_single_vreg(struct device *dev,char *vreg_name,struct regulator **reg_ptr
,uint32_t min_voltage,uint32_t max_voltage,uint32_t op_mode,int config)
{
	int rc = 0;
	

	if (!dev) {
		pr_err("%s: get failed NULL parameter\n", __func__);
		goto vreg_get_fail;
	}
	if (config) {
		//printk("%s enable %s\n", __func__, vreg_name);
		*reg_ptr = regulator_get(dev, vreg_name);
		if (IS_ERR(*reg_ptr)) {
			pr_err("%s: %s get failed\n", __func__, vreg_name);
			*reg_ptr = NULL;
			goto vreg_get_fail;
		}
		if (regulator_count_voltages(*reg_ptr) > 0) {
			//printk("%s: voltage min=%d, max=%d\n",__func__, min_voltage, max_voltage);
			rc = regulator_set_voltage(*reg_ptr,min_voltage,max_voltage);
			if (rc < 0) {
				pr_err("%s: %s set voltage failed\n",__func__, vreg_name);
				goto vreg_set_voltage_fail;
			}
			if (op_mode >= 0) {
				rc = regulator_set_optimum_mode(*reg_ptr,op_mode);
				if (rc < 0) {
					pr_err("%s: %s set optimum mode failed\n",__func__, vreg_name);
					goto vreg_set_opt_mode_fail;
				}
			}
		}
		rc = regulator_enable(*reg_ptr);
		if (rc < 0) {
			pr_err("%s: %s regulator_enable failed\n", __func__,
				vreg_name);
			goto vreg_unconfig;
		}
	} else {
		//printk("%s disable %s\n", __func__, vreg_name);
		if (*reg_ptr) {
			//printk("%s disable %s\n", __func__, vreg_name);
			regulator_disable(*reg_ptr);
			if (regulator_count_voltages(*reg_ptr) > 0) {
				if (op_mode >= 0)
					regulator_set_optimum_mode(*reg_ptr, 0);
				regulator_set_voltage(*reg_ptr, 0, max_voltage);
			}
			regulator_put(*reg_ptr);
			*reg_ptr = NULL;
		} else {
			pr_err("%s can't disable %s\n", __func__, vreg_name);
		}
	}
	return 0;

vreg_unconfig:
if (regulator_count_voltages(*reg_ptr) > 0)
	regulator_set_optimum_mode(*reg_ptr, 0);

vreg_set_opt_mode_fail:
if (regulator_count_voltages(*reg_ptr) > 0)
	regulator_set_voltage(*reg_ptr, 0, max_voltage);

vreg_set_voltage_fail:
	regulator_put(*reg_ptr);
	*reg_ptr = NULL;

vreg_get_fail:
	return -ENODEV;
}
struct regulator *g_ddr_vdd_vreg;
struct regulator *g_core_vdd_vreg;
#ifdef ZD552KL_PHOENIX
struct regulator *g_ext_buck_vreg;
#endif
int rkpreisp_gpio_set(int gpio_num,int flags,const char *label)
{//flags:GPIOF_DIR_IN | GPIOF_INIT_HIGH
	int rc=0;
	rc=gpio_request_one(gpio_num,flags,label);
	if(!rc)
	{
		gpio_free(gpio_num);
	}
	return rc;
}
#define CHECK_GPIO(__x__)  do{if((__x__)){\
	printk("%s:%d gpio2=%d\n",__func__,__LINE__,gpio_get_value(02));\
	printk("%s:%d gpio40=%d\n",__func__,__LINE__,gpio_get_value(40));\
	printk("%s:%d gpio141=%d\n",__func__,__LINE__,gpio_get_value(141));}}while(0);
#if 0	
void rkpreisp_init_hw(struct spi_rk_preisp_data *pdata)
{
	if(pdata)
	{
		spi_cs_set_value(pdata, 0);
		if (pdata->reset_gpio > 0) {
            gpio_set_value(pdata->reset_gpio, !pdata->reset_active);
        }
        mdelay(5);
		//do power/clk on
        if (pdata->mclk != NULL) {
            clk_prepare_enable(pdata->mclk);
            clk_set_rate(pdata->mclk, PREISP_MCLK_RATE);
        }
		//if(asus_hw_id>=ASUS_ER)
		{
			asus_power_config(pdata,3,1);
		}
		if (pdata->powerdown_gpio > 0) {
            gpio_set_value(pdata->powerdown_gpio, pdata->powerdown_active);
        }
        if (pdata->wakeup_gpio > 0) {
            gpio_set_value(pdata->wakeup_gpio, pdata->wakeup_active);
        }
		if (pdata->reset_gpio > 0) {
            gpio_set_value(pdata->reset_gpio, pdata->reset_active);
        }
		mdelay(5);
		spi_cs_set_value(pdata, 1);		
		spi2apb_switch_to_msb(pdata->spi);
	    spi2apb_safe_w32(pdata->spi, 0x11010000, 0x0fff0555);
	}
}

static int rkpreisp_download_fw_late(struct spi_rk_preisp_data *pdata)
{
	int ret=0;
	//printk("%s E \n",__func__);
	if(!pdata)return -1;
	if (pdata->powerdown_gpio > 0) {
        gpio_set_value(pdata->powerdown_gpio, pdata->powerdown_active);
    }
    if (pdata->wakeup_gpio > 0) {
        gpio_set_value(pdata->wakeup_gpio, pdata->wakeup_active);
    }
    mdelay(3);
    if (pdata->reset_gpio > 0) {
        gpio_set_value(pdata->reset_gpio, pdata->reset_active);
    }
    mdelay(5);
    spi_cs_set_value(pdata, 1);
    if(pdata->min_speed_hz)
		preisp_set_spi_speed(pdata, pdata->min_speed_hz);
    spi2apb_switch_to_msb(pdata->spi);
	rkpreisp_hw_init(pdata->spi);
    preisp_set_spi_speed(pdata, pdata->max_speed_hz);
    ret = spi_download_fw(pdata->spi, NULL,pdata->fw_speed_hz,pdata->max_speed_hz);
    if (ret) {
        dev_err(pdata->dev, "download firmware failed!");
    } else {
        dev_info(pdata->dev, "download firmware success!");
		pdata->is_fw_loaded=true;
    }
	//printk("%s X \n",__func__);
	return ret;
}

static int rkpreisp_suspend(struct spi_device *spi, pm_message_t mesg)
{
	struct spi_rk_preisp_data *data;
	if(spi)
	{
		data=(struct spi_rk_preisp_data *)spi_get_drvdata(spi);
		if(data)
		{
			data->is_suspend_sleep=true;
			rkpreisp_power_off(data);
			/////////////////////
			//printk("%s gpios[0,1,2,3,%d,%d,%d,%d,%d]=[%d,%d,%d,%d,%d,%d,%d,%d,%d]\n",__func__
			//	,data->irq_gpio,data->wakeup_gpio,data->sleepst_gpio,data->snapshot_gpio,data->reset_gpio
			//	,gpio_get_value(0),gpio_get_value(1),gpio_get_value(2),gpio_get_value(3)
			//	,gpio_get_value(data->irq_gpio),gpio_get_value(data->wakeup_gpio),gpio_get_value(data->sleepst_gpio)
			//	,gpio_get_value(data->snapshot_gpio),gpio_get_value(data->reset_gpio)
			//	);
		}
	}
	return 0;
}
static int rkpreisp_resume(struct spi_device *spi)
{
	struct spi_rk_preisp_data *data;
	if(spi)
	{
		data=(struct spi_rk_preisp_data *)spi_get_drvdata(spi);
		if(data)
		{
			//printk("%s gpios[0,1,2,3,%d,%d,%d,%d,%d]=[%d,%d,%d,%d,%d,%d,%d,%d,%d]\n",__func__
			//	,data->irq_gpio,data->wakeup_gpio,data->sleepst_gpio,data->snapshot_gpio,data->reset_gpio
			//	,gpio_get_value(0),gpio_get_value(1),gpio_get_value(2),gpio_get_value(3)
			//	,gpio_get_value(data->irq_gpio),gpio_get_value(data->wakeup_gpio),gpio_get_value(data->sleepst_gpio)
			//	,gpio_get_value(data->snapshot_gpio),gpio_get_value(data->reset_gpio)
			//	);
			//if(asus_hw_id<ASUS_ER)
			//	asus_power_config(data,3,1);
		}
	}
	return 0;
}
#endif

static int asus_core_power_supply_control(struct spi_rk_preisp_data *data,int value)
{
	int ret=0;
	if(!data)ret=-EFAULT;
	else{
		if((!data->is_core_supply_enabled  &&  value)  ||  (data->is_core_supply_enabled  &&  !value)){
			data->is_core_supply_enabled=(value>0);
			ret=preisp_config_single_vreg(data->dev,"core_vdd",&g_core_vdd_vreg,1100000,1100000,80000,(value>0));
			if(ret<0)
				dev_err(data->dev,"%s:%d set core_vdd failed\n",__func__,__LINE__);
		}
		else
		{
			dev_info(data->dev, "sz_cam_rk_drv core_supply_enabled=%d,value=%d !\n",data->is_core_supply_enabled,value);
		}
	}
	return ret;
}
static int asus_ddr_power_supply_control(struct spi_rk_preisp_data *data,int value)
{
	int ret=0;
	if(!data)ret=-EFAULT;
	else{
		if((!data->is_ddr_supply_enabled  &&  value)  ||  (data->is_ddr_supply_enabled  &&  !value)){
			data->is_ddr_supply_enabled=(value>0);
			ret=preisp_config_single_vreg(data->dev,"ddr_vdd",&g_ddr_vdd_vreg,1200000,1200000,80000,(value>0));
			if(ret<0)
				dev_err(data->dev,"%s:%d set ddr_vdd failed\n",__func__,__LINE__);		
		}
		else
		{
			dev_info(data->dev, "sz_cam_rk_drv ddr_supply_enabled=%d,value=%d !\n",data->is_ddr_supply_enabled,value);
		}
	}
	return ret;
}
#ifdef ZD552KL_PHOENIX
static int asus_ext_buck_power_supply_control(struct spi_rk_preisp_data *data,int value)
{
	int ret=0;
	if(!data)ret=-EFAULT;
	else{
		if((!data->is_ext_buck_supply_enabled  &&  value)  ||  (data->is_ext_buck_supply_enabled  &&  !value)){
			data->is_ext_buck_supply_enabled=(value>0);
			ret=preisp_config_single_vreg(data->dev,"ext_buck",&g_ext_buck_vreg,PREISP_NORMAL_CORE_VOLTAGE,PREISP_MAX_CORE_VOLTAGE,80000,(value>0));
			if(ret<0)
				dev_err(data->dev,"%s:%d set ext_buck failed\n",__func__,__LINE__);
		}
		else
		{
			dev_info(data->dev, "sz_cam_rk_drv ext_buck_supply_enabled=%d,value=%d !\n",data->is_ext_buck_supply_enabled,value);
		}
	}
	return ret;
}

static int asus_vio_en_gpio_control(struct spi_rk_preisp_data *data,int value)
{
	int err=0;
	int flag=GPIOF_DIR_OUT;

	if(value)
		flag|=GPIOF_INIT_HIGH;
	else
		flag|=GPIOF_INIT_LOW;

	err = rkpreisp_gpio_set(data->vio_en_gpio,flag,"GPIO40");
	if(err<0)
	{
		dev_err(data->dev,"%s:%d enable VDD rc=%d FAIL!\n",__func__,__LINE__,err);
	}
	return err;
}

#endif
static int asus_power_control(struct spi_rk_preisp_data *data,int value)
{
	int err=0;
	int flag=GPIOF_DIR_OUT;
	//printk("%s E value=%d power_state=0x%x\n",__func__,value,data->power_state);
	if(  ((data->power_state&2)==2 && value==0)//for power off
	   ||((data->power_state&2)==0 && value!=0)//for power on
	   )
	{
		//asus bsp ralf:add for enter deep sleep mode when camera close>>
		//if(value>0){
			asus_core_power_supply_control(data,value>0);
#ifdef ZD552KL_PHOENIX
			asus_ext_buck_power_supply_control(data,value>0);
#endif
			asus_ddr_power_supply_control(data,value>0);
			if(value)
				flag=GPIOF_DIR_OUT|GPIOF_INIT_HIGH;
			else
			    //flag=GPIOF_DIR_IN;//hades laser require i2c is high, otherwise laser power consumption will be high
				flag=GPIOF_DIR_OUT|GPIOF_INIT_LOW;//asus bsp ralf,comfirmed with jerry with this
			err = rkpreisp_gpio_set(data->vio_en_gpio,flag,"GPIO40");
			if(err<0)
			{
				printk("%s:%d enable VDD rc=%d FAIL!!!!!!!!!!\n",__func__,__LINE__,err);
			}
		//}
		//asus bsp ralf:add for enter deep sleep mode when camera close<<
		
	}
	if(value)
		data->power_state|=2;
	else
		data->power_state&=~2;
	//printk("%s X value=%d power_state=0x%x\n",__func__,value,data->power_state);
	return err;
}
static int asus_clk_control(struct spi_rk_preisp_data *data,int value)
{
	int err=0;
	//printk("%s E value=%d power_state=0x%x\n",__func__,value,data->power_state);
	if(value)
	{
		if((data->power_state&1)==0 )
		{
			if(!data->pinctrl_info.pinctrl  ||  data->pinctrl_status == 0)
				printk("%s:%d pinctrl is null\n",__func__,__LINE__);
			else
				err = pinctrl_select_state(data->pinctrl_info.pinctrl,data->pinctrl_info.gpio_state_active);

			if(data->clk_info &&  data->clk_ptr)
				err=msm_camera_clk_enable(data->dev,data->clk_info,data->clk_ptr,data->num_clk,(value&1));
			else
				printk("%s clk info is null\n",__func__);
			mdelay(5);
		}
		else
		{
			dev_info(data->dev, "clk state %d, value %d!\n",data->power_state&1,value);
		}
	}
	else
	{
		if((data->power_state&1)==1 )
		{
			if(data->clk_info &&  data->clk_ptr)
				err=msm_camera_clk_enable(data->dev,data->clk_info,data->clk_ptr,data->num_clk,(value&1));
			else
				printk("%s clk info is null\n",__func__);

			if(!data->pinctrl_info.pinctrl  ||  data->pinctrl_status == 0)
				printk("%s:%d pinctrl is null\n",__func__,__LINE__);
			else
				err = pinctrl_select_state(data->pinctrl_info.pinctrl,data->pinctrl_info.gpio_state_suspend);
		}
		else
		{
			dev_info(data->dev, "clk state %d, value %d!\n",data->power_state&1,value);
		}
	}
	if (err)
		printk("%s:%d cannot set pinctrl state FAIL!!!!!!!!!!\n",__func__, __LINE__);

	if(err<0)
	{
		printk("%s:%d set mclk is failed !!!!!!!!!!!! rc=%d\n",__func__,__LINE__,err);
	}
	if(value)
		data->power_state|=1;
	else
		data->power_state&=~1;
	//printk("%s X value=%d power_state=0x%x\n",__func__,value,data->power_state);
	return err;
}
#ifdef ZD552KL_PHOENIX

static int asus_sleep_power_operation(struct spi_rk_preisp_data *pdata)
{
	//off, gpio first, clk sencond, power last
	int rc=0;
	if(!pdata)
	{
		dev_err(pdata->dev,"pdata is NULL!\n");
		return -1;
	}
	//dev_info(pdata->dev,"%s E\n",__func__);

	rc = asus_clk_control(pdata,0);
	if(rc == 0)
		rc = asus_ext_buck_power_supply_control(pdata,0);

	//dev_info(pdata->dev,"%s X\n",__func__);
	return rc;
}

static int asus_wakeup_power_operation(struct spi_rk_preisp_data *pdata)
{
	int rc = 0;
	if(!pdata)
	{
		dev_err(pdata->dev,"pdata is NULL!\n");
		return -1;
	}
	//dev_info(pdata->dev,"%s E\n",__func__);
	//on, power first, clk second, gpio last
	rc = asus_ext_buck_power_supply_control(pdata,1);
	if(rc == 0)
		rc = asus_clk_control(pdata,1);

	//dev_info(pdata->dev,"%s X\n",__func__);
	return rc;
}

static void sleep_operation(struct work_struct *work)
{
	sleep_work_t *sleep_work = container_of(work, sleep_work_t, work);
	struct spi_rk_preisp_data * pdata = sleep_work->pdata;

	if(pdata != g_preisp_data)
	{
		printk("preisp, pdata get not right!\n");
		return;
	}

	if(!pdata)
	{
		dev_err(pdata->dev,"pdata is NULL!\n");
		return;
	}

	mutex_lock(&pdata->wake_sleep_lock);

	if(pdata->is_standby_sleep_mode == 1)
	{
		asus_sleep_power_operation(pdata);
	}
	else
	{
		dev_info(pdata->dev, "not power off ncp, sleep flag in sleep_operation, bypass %d, standby %d\n",pdata->is_bypass_sleep_mode,pdata->is_standby_sleep_mode);
	}

	mutex_unlock(&pdata->wake_sleep_lock);
}
#endif

int asus_power_config( struct spi_rk_preisp_data *data,int type,int value)
{
	int err=0;
	if(!data)return err;
	//printk("%s E type=%d,value=%d power_state=0x%x\n",__func__,type,value,data->power_state);
	//on, power first, clk second
	//off, clk first, power second
	//type 2 power, type 1 clk

	if(value)//on
	{
		if((type&2)==2)
		{
			err = asus_power_control(data,1);
		}
		if((type&1)==1)
		{
			err = asus_clk_control(data,1);
		}
	}
	else//off
	{
		if((type&1)==1)
		{
			err = asus_clk_control(data,0);
		}

		if((type&2)==2)
		{
			err = asus_power_control(data,0);
		}
	}

	//printk("%s X type=%d,value=%d power_state=0x%x\n",__func__,type,value,data->power_state);
	return err;
}


int rkpreisp_power_on_dsp(int is_download_fw)
{
	return rkpreisp_power_on(g_preisp_data,is_download_fw);
}

int rkpreisp_power_off_dsp(void)
{
	return rkpreisp_power_off(g_preisp_data);
}

void preisp_set_spi_speed(struct spi_rk_preisp_data *pdata, uint32_t hz)
{
    pdata->spi->max_speed_hz = hz;
}

void preisp_client_list_init(preisp_client_list *s)
{
    mutex_init(&s->mutex);
    INIT_LIST_HEAD(&s->list);
}

preisp_client * preisp_client_new(void)
{
    preisp_client *c =
        (preisp_client *)kzalloc(sizeof(preisp_client), GFP_KERNEL);
    c->id = INVALID_ID;
    INIT_LIST_HEAD(&c->list);
    msq_init(&c->q, MSG_QUEUE_DEFAULT_SIZE);
    init_waitqueue_head(&c->wait);
    return c;
}

void preisp_client_release(preisp_client *c)
{
    msq_release(&c->q);
    kfree(c);
}

preisp_client* preisp_client_find(preisp_client_list *s, preisp_client *c)
{
    preisp_client *client = NULL;

    list_for_each_entry(client, &s->list, list) {
        if (c == client) {
            return c;
        }
    }
    return NULL;
}

int preisp_client_connect(struct spi_rk_preisp_data *pdata, preisp_client *c)
{
    preisp_client_list *s = &pdata->clients;;
    mutex_lock(&s->mutex);
    if (preisp_client_find(s, c)) {
        mutex_unlock(&s->mutex);
        return -1;
    }

    list_add_tail(&c->list, &s->list);
    mutex_unlock(&s->mutex);

    return 0;
}

void preisp_client_disconnect(struct spi_rk_preisp_data *pdata, preisp_client *c)
{
    preisp_client_list *s = &pdata->clients;;
    mutex_lock(&s->mutex);
    if (preisp_client_find(s, c)) {
        list_del_init(&c->list);
    }
    mutex_unlock(&s->mutex);
}

void spi_cs_set_value(struct spi_rk_preisp_data *pdata, int value)
{
    int8_t null_cmd = 0;
    struct spi_transfer null_cmd_packet = {
        .tx_buf = &null_cmd,
        .len    = sizeof(null_cmd),
        .cs_change = !value,
    };
    struct spi_message  m;

    spi_message_init(&m);
    spi_message_add_tail(&null_cmd_packet, &m);
    spi_sync(pdata->spi, &m);
}

void rkpreisp_hw_init(struct spi_device *spi)
{
    //spi2apb_safe_w32(spi, 0x11010000, 0x0fff0555);
	spi2apb_safe_w32(spi, 0x12008098, 0xff004000);
}


int preisp_send_msg_to_dsp(struct spi_rk_preisp_data *pdata, const struct msg *m)
{
    int ret = 0;
#ifdef ZD552KL_PHOENIX
    dev_dbg(pdata->dev, "send msg type is 0x%x, camera id %d\n",m->type,m->camera_id);
    if(m->type == id_msg_set_rt_zzhdr_on_t)
    {
	    g_cur_core_voltage = PREISP_ZZHDR_CORE_VOLTAGE;
	    ncp6335d_set_voltage_for_preisp(g_cur_core_voltage);
    }
    else if(m->type == id_msg_set_rt_zzhdr_off_t)
    {
	    g_cur_core_voltage = PREISP_NORMAL_CORE_VOLTAGE;
	    ncp6335d_set_voltage_for_preisp(g_cur_core_voltage);
    }
#endif
    mutex_lock(&pdata->send_msg_lock);
    ret = dsp_msq_send_msg(pdata->spi, m);
    mutex_unlock(&pdata->send_msg_lock);
    return ret;
}

int rkpreisp_set_log_level(struct spi_rk_preisp_data *pdata, int level)
{
    int ret = 0;
    MSG(msg_set_log_level_t, m);

    m.log_level = level;
    ret = preisp_send_msg_to_dsp(pdata, (struct msg*)&m);
    return ret;
}

int rkpreisp_request_sleep(struct spi_rk_preisp_data *pdata, int32_t mode)
{
    int ret;
    int try = 0;

    MSG(msg_set_sys_mode_standby_t, m);

    mutex_lock(&pdata->wake_sleep_lock);
    if (atomic_dec_return(&pdata->wake_sleep_cnt) == 0  ||  pdata->do_force_sleep) {
	if(pdata->do_force_sleep){
               atomic_inc_return(&pdata->wake_sleep_cnt);
               pdata->do_force_sleep=false;
       }
        if (mode >= PREISP_SLEEP_MODE_MAX || mode < 0) {
            dev_warn(pdata->dev, "Unkown sleep mode %d\n", mode);
            mutex_unlock(&pdata->wake_sleep_lock);
            return -1;
        }
        dev_info(pdata->dev,"request sleep\n");

        if (mode == PREISP_SLEEP_MODE_BYPASS) {
            m.type = id_msg_set_sys_mode_bypass_t;
        }

        if (pdata->wakeup_gpio > 0) {
            gpio_set_value(pdata->wakeup_gpio, !pdata->wakeup_active);
        }
        ret = preisp_send_msg_to_dsp(pdata, (struct msg*)&m);
#ifdef ZD552KL_PHOENIX
		if(mode == PREISP_SLEEP_MODE_STANDBY)
			pdata->is_standby_sleep_mode = 1;
		else
			pdata->is_bypass_sleep_mode = 1;
#endif

        do {
            if (pdata->sleep_state_flag) {
                ret = 0;
                pdata->sleep_state_flag = 0;
                mdelay(5);
                break;
            }
            if (try++ == PREISP_REQUEST_SLEEP_TIMEOUT_MS) {
                ret = -1;
                dev_err(pdata->dev, "request sleep timeout\n");
                break;
            }
            mdelay(1);
        } while (1);
        dev_info(pdata->dev, "request dsp enter %s mode. ret:%d",
                mode == PREISP_SLEEP_MODE_BYPASS ? "bypass" : "sleep", ret);
	
    } else if (atomic_read(&pdata->wake_sleep_cnt) < 0) {
        atomic_set(&pdata->wake_sleep_cnt, 0);
    }
    mutex_unlock(&pdata->wake_sleep_lock);

    return ret;
}

static int rkpreisp_restart(struct spi_rk_preisp_data *pdata)
{
    int ret = 0;

	dev_info(pdata->dev, "%s E",__func__);

    disable_irq(pdata->irq);
    if (pdata->sleepst_irq > 0) {
        disable_irq(pdata->sleepst_irq);
    }
    if (pdata->reset_gpio > 0) {
        gpio_set_value(pdata->reset_gpio, !pdata->reset_active);
    }
    mdelay(3);

    //request rkpreisp enter slave mode
    spi_cs_set_value(pdata, 0);
    if (pdata->wakeup_gpio > 0) {
        gpio_set_value(pdata->wakeup_gpio, pdata->wakeup_active);
    }
    mdelay(3);
    if (pdata->reset_gpio > 0) {
        gpio_set_value(pdata->reset_gpio, pdata->reset_active);
    }
    mdelay(5);
    spi_cs_set_value(pdata, 1);
    preisp_set_spi_speed(pdata, pdata->min_speed_hz);
    spi2apb_switch_to_msb(pdata->spi);
    rkpreisp_hw_init(pdata->spi);

    preisp_set_spi_speed(pdata, pdata->max_speed_hz);
    //download system firmware
    ret = spi_download_fw(pdata->spi, NULL,pdata->fw_speed_hz,pdata->max_speed_hz);
    if (ret) {
        dev_err(pdata->dev, "download firmware failed!");
    } else {
        dev_info(pdata->dev, "download firmware success!");
    }

    enable_irq(pdata->irq);
    if (pdata->sleepst_irq > 0) {
        enable_irq(pdata->sleepst_irq);
    }

	dev_info(pdata->dev, "%s X",__func__);

	return ret;

}

int rkpreisp_wakeup(struct spi_rk_preisp_data *pdata)
{
    int32_t reg = 0;
    int try = 0, ret = 0;
    mutex_lock(&pdata->wake_sleep_lock);
#ifdef ZD552KL_PHOENIX
    if (atomic_inc_return(&pdata->wake_sleep_cnt) == 1   ||  pdata->do_force_wakeup) {
	if(pdata->do_force_wakeup){
               atomic_dec_return(&pdata->wake_sleep_cnt);
               pdata->do_force_wakeup=false;
       }
#else
    if (atomic_inc_return(&pdata->wake_sleep_cnt) == 1   ||  pdata->do_force_sleep) {
	if(pdata->do_force_sleep){
               atomic_dec_return(&pdata->wake_sleep_cnt);
               pdata->do_force_sleep=false;
       }
#endif

#ifdef ZD552KL_PHOENIX
		if(pdata->is_standby_sleep_mode == 1)
		{
			asus_wakeup_power_operation(pdata);
			pdata->is_standby_sleep_mode = 0;
		}
		else if(pdata->is_bypass_sleep_mode == 1)
		{
			pdata->is_bypass_sleep_mode = 0;
		}
		//dev_info(pdata->dev, "sleep flag after wakeup, bypass %d, standby %d\n",pdata->is_bypass_sleep_mode,pdata->is_standby_sleep_mode);
#endif
        if (pdata->powerdown_gpio > 0) {
            gpio_set_value(pdata->powerdown_gpio, pdata->powerdown_active);
        }
        if (pdata->wakeup_gpio > 0) {
            gpio_set_value(pdata->wakeup_gpio, pdata->wakeup_active);
        } else {
            dev_info(pdata->dev, "please config wakeup gpio first!");
        }

        do {
            ret = spi2apb_safe_r32(pdata->spi, DSP_PMU_SYS_REG0, &reg);

            if (!ret && ((reg & DSP_MSG_QUEUE_OK_MASK) == DSP_MSG_QUEUE_OK_TAG)) {
                dev_info(pdata->dev, "wakeup dsp, times %d\n",try);
                break;
            }

            if (try++ == PREISP_WAKEUP_TIMEOUT_MS) {
				dev_err(pdata->dev, "wakeup timeout, restart preisp\n");
				ret = rkpreisp_restart(pdata);
                break;
            }
            mdelay(1);
        } while (1);
	
    }
    mutex_unlock(&pdata->wake_sleep_lock);

    return ret;
}

static int rkpreisp_power_on(struct spi_rk_preisp_data *pdata,int is_download_fw)
{
    int ret = 0;
	//dev_info(pdata->dev, "%s E poweron_count=%d,sleep_count=%d",__func__
	//	,atomic_read(&pdata->power_on_cnt)
	//	,atomic_read(&pdata->wake_sleep_cnt));
    mutex_lock(&pdata->power_lock);

    if (atomic_inc_return(&pdata->power_on_cnt) == 1) {
        dev_info(pdata->dev, "dsp power on!");
        //do power/clk on
        if (pdata->mclk != NULL) {
            clk_prepare_enable(pdata->mclk);
            clk_set_rate(pdata->mclk, PREISP_MCLK_RATE);
        }
        //request rkpreisp enter slave mode
        spi_cs_set_value(pdata, 0);
        /*
            TODO: config regulator
        */
		asus_power_config(pdata,3,1);
        if (pdata->powerdown_gpio > 0) {
            gpio_set_value(pdata->powerdown_gpio, pdata->powerdown_active);
        }
        if (pdata->wakeup_gpio > 0) {
            gpio_set_value(pdata->wakeup_gpio, pdata->wakeup_active);
        }
        mdelay(3);
        if (pdata->reset_gpio > 0) {
            gpio_set_value(pdata->reset_gpio, pdata->reset_active);
        }
        mdelay(5);
        spi_cs_set_value(pdata, 1);
        if(pdata->min_speed_hz)
			preisp_set_spi_speed(pdata, pdata->min_speed_hz);
        spi2apb_switch_to_msb(pdata->spi);
        rkpreisp_hw_init(pdata->spi);

        preisp_set_spi_speed(pdata, pdata->max_speed_hz);
        //download system firmware
        if(is_download_fw){
	        ret = spi_download_fw(pdata->spi, NULL,pdata->fw_speed_hz,pdata->max_speed_hz);
	        if (ret) {
	            dev_err(pdata->dev, "download firmware failed!");
	        } else {
	            dev_info(pdata->dev, "download firmware success!");
	            pdata->is_fw_loaded = true;
	        }
        }

        enable_irq(pdata->irq);
        if (pdata->sleepst_irq > 0) {
            enable_irq(pdata->sleepst_irq);
        }
        rkpreisp_set_log_level(pdata, pdata->log_level);
#ifdef ZD552KL_PHOENIX
        pdata->is_suspended = false;
#endif
        pdata->sleep_state_flag = 0;
        atomic_set(&pdata->wake_sleep_cnt, 1);
    } else {
        rkpreisp_wakeup(pdata);
    }
    mutex_unlock(&pdata->power_lock);
	//dev_info(pdata->dev, "%s X poweron_count=%d,sleep_count=%d",__func__
	//		,atomic_read(&pdata->power_on_cnt)
	//		,atomic_read(&pdata->wake_sleep_cnt));

    return ret;
}

static int rkpreisp_power_off(struct spi_rk_preisp_data *pdata)
{
    int ret = 0;
	//dev_info(pdata->dev, "%s E poweron_count=%d,sleep_count=%d",__func__
	//	,atomic_read(&pdata->power_on_cnt)
	//	,atomic_read(&pdata->wake_sleep_cnt));
    mutex_lock(&pdata->power_lock);
    if (atomic_dec_return(&pdata->power_on_cnt) == 0) {
        //do power/clk off
        dev_info(pdata->dev, "dsp power off!");
        disable_irq(pdata->irq);
        if (pdata->sleepst_irq > 0) {
            disable_irq(pdata->sleepst_irq);
        }

        if (pdata->powerdown_gpio > 0) {
            gpio_set_value(pdata->powerdown_gpio, !pdata->powerdown_active);
        }
        if (pdata->wakeup_gpio > 0) {
            gpio_set_value(pdata->wakeup_gpio, !pdata->wakeup_active);
        }
        if (pdata->reset_gpio > 0) {
            gpio_set_value(pdata->reset_gpio, !pdata->reset_active);
        }
        spi_cs_set_value(pdata, 0);
		asus_power_config(pdata,3,0);
        if (pdata->mclk != NULL) {
            clk_disable_unprepare(pdata->mclk);
        }
        atomic_set(&pdata->wake_sleep_cnt, 0);
        pdata->is_fw_loaded = false;
#ifdef ZD552KL_PHOENIX
        set_fw_revision("NotLoad");
        pdata->is_standby_sleep_mode = 0;
        pdata->is_bypass_sleep_mode = 0;
#endif
    } else if (atomic_read(&pdata->power_on_cnt) < 0) {
        atomic_set(&pdata->power_on_cnt, 0);
    } else {
        rkpreisp_request_sleep(pdata, PREISP_SLEEP_MODE_STANDBY);
    }
    mutex_unlock(&pdata->power_lock);
	//dev_info(pdata->dev, "%s X poweron_count=%d,sleep_count=%d",__func__
	//	,atomic_read(&pdata->power_on_cnt)
	//	,atomic_read(&pdata->wake_sleep_cnt));
    return ret;
}

static void fw_nowait_power_on(const struct firmware *fw, void *context)
{
    int ret = 0;
    struct spi_rk_preisp_data *pdata = context;
	//dev_info(pdata->dev,"%s in fw=%p",__func__,fw);

	ret = rkpreisp_power_on(pdata,1);
	if (!ret) {
		mdelay(10); /*delay for dsp boot*/
		rkpreisp_request_sleep(pdata, PREISP_SLEEP_MODE_STANDBY);
	}

    if (fw) {
        release_firmware(fw);
    }
    if(pdata->is_resume_processing == true)
    {
		dev_info(pdata->dev,"resume done!\n");
#ifdef ZD552KL_PHOENIX
		mutex_unlock(&pdata->resume_lock);
#endif
		wake_unlock(&pdata->resume_wake_lock);
		pdata->is_resume_processing = false;
    }
    else
    {
		dev_info(pdata->dev,"not resume, not unlock resume_wake_lock\n");
    }
}

int parse_arg(const char *s, struct auto_arg *arg)
{
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        long v;
        v = simple_strtol(s, NULL, 16);
        arg->type = AUTO_ARG_TYPE_INT32;
        arg->m_int32 = v;
    } else if (isdigit(s[0])) {
        long v;
        v = simple_strtol(s, NULL, 10);
        arg->type = AUTO_ARG_TYPE_INT32;
        arg->m_int32 = v;
    } else {
        arg->type = AUTO_ARG_TYPE_STR;
        arg->m_str = s;
    }

    return 0;
}

int parse_auto_args(char *s, struct auto_args *args)
{
    int i = 0;
    char c = 0;
    int last_is_arg_flag = 0;
    const char *last_arg;

    args->argc = 0;

    i = -1;
    do {
        c = s[++i];
        if (c == ' ' || c == ',' || c == '\n' || c == '\r' || c == 0) {
            if (last_is_arg_flag) {
                args->argc++;
            }
            last_is_arg_flag = 0;
        } else {
            last_is_arg_flag = 1;
        }
    } while(c != 0 && c != '\n' && c != '\r');

    args->argv = (struct auto_arg*)kmalloc(
            args->argc * sizeof(struct auto_arg), GFP_KERNEL);

    i = -1;
    last_is_arg_flag = 0;
    last_arg = s;
    args->argc = 0;
    do {
        c = s[++i];
        if (c == ' ' || c == ',' || c == '\n' || c == '\r' || c == 0) {
            if (last_is_arg_flag) {
                parse_arg(last_arg, args->argv + args->argc++);
                s[i] = 0;
            }
            last_is_arg_flag = 0;
        } else {
            if (last_is_arg_flag == 0) {
                last_arg = s + i;
            }
            last_is_arg_flag = 1;
        }
    } while(c != 0 && c != '\n' && c != '\r');

    return c == 0 ? i : i+1;
}

void free_auto_args(struct auto_args *args)
{
    kfree(args->argv);
    args->argc = 0;
}

void int32_hexdump(const char *prefix, int32_t *data, int len)
{
    pr_err("%s\n", prefix);
    print_hex_dump(KERN_ERR, "offset ", DUMP_PREFIX_OFFSET,
            16, 4, data, len, false);
    pr_err("\n");
}

int do_cmd_write(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    int32_t addr;
    int32_t len = (args->argc - 2) * sizeof(int32_t);
    int32_t *data;
    int i;

    if (args->argc < 3 && args->argv[1].type != AUTO_ARG_TYPE_INT32) {
        dev_err(pdata->dev, "Mis or unknown args!");
        return -1;
    }

    len = MIN(len, APB_MAX_OP_BYTES);

    addr = args->argv[1].m_int32;
    data = (int32_t*)kmalloc(len, GFP_KERNEL);
    for (i = 0; i < len/4; i++) {
        if (args->argv[i+2].type != AUTO_ARG_TYPE_INT32) {
            dev_err(pdata->dev, "Unknown args!");
            kfree(data);
            return -1;
        }

        data[i] = args->argv[i+2].m_int32;
    }

    spi2apb_write(pdata->spi, addr, data, len);

    kfree(data);

    dev_info(pdata->dev, "write addr: 0x%x, len: %d bytes", addr, len);
    return 0;
}

int do_cmd_read(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    int32_t addr;
    int32_t len;
    int32_t *data;

    if (args->argc < 3 && args->argv[1].type != AUTO_ARG_TYPE_INT32) {
        dev_err(pdata->dev, "Mis or unknown args!");
        return -1;
    }

    addr = args->argv[1].m_int32;
    if (args->argc == 2) {
        len = 32;
    } else {
        if (args->argv[2].type != AUTO_ARG_TYPE_INT32) {
            dev_err(pdata->dev, "Unknown args!");
            return -1;
        }
        len = args->argv[2].m_int32 * sizeof(int32_t);
        len = MIN(len, APB_MAX_OP_BYTES);
    }

    data = (int32_t*)kmalloc(len, GFP_KERNEL);

    dev_info(pdata->dev, "\nread addr: %x, len: %d bytes", addr, len);
    spi2apb_read(pdata->spi, addr, data, len);
    int32_hexdump("read data:", data, len);
    kfree(data);

    return 0;
}

int do_cmd_set_spi_rate(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    if (args->argc < 2 && args->argv[1].type != AUTO_ARG_TYPE_INT32) {
        dev_err(pdata->dev, "Mis or unknown args!");
        return -1;
    }

    pdata->max_speed_hz = args->argv[1].m_int32;
    dev_info(pdata->dev, "set spi max speed to %d!", pdata->max_speed_hz);

    if (args->argc == 3 && args->argv[2].type == AUTO_ARG_TYPE_INT32) {
        pdata->min_speed_hz = args->argv[2].m_int32;
        dev_info(pdata->dev, "set spi min speed to %d!", pdata->min_speed_hz);
    }

    return 0;
}

int do_cmd_query(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    int32_t state;

    spi2apb_operation_query(pdata->spi, &state);
    dev_info(pdata->dev, "state %x", state);
    return 0;
}

int do_cmd_download_fw(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    int ret = 0;
    const char *fw_name = NULL;

    if (args->argc == 2 && args->argv[1].type == AUTO_ARG_TYPE_STR) {
        fw_name = args->argv[1].m_str;
    }

    ret = spi_download_fw(pdata->spi, fw_name,pdata->fw_speed_hz,pdata->max_speed_hz);
    if (ret)
        dev_err(pdata->dev, "download firmware failed!");
    else
        dev_info(pdata->dev, "download firmware success!");
    return 0;
}

int do_cmd_fast_write(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    int ret = 0;
    int32_t reg;

    if (args->argc != 2 && args->argv[1].type != AUTO_ARG_TYPE_INT32) {
        dev_err(pdata->dev, "Mis or unknown args!");
        return -1;
    }

    reg = args->argv[1].m_int32;

    ret = spi2apb_interrupt_request(pdata->spi, reg);
    dev_info(pdata->dev, "interrupt request reg1:%x ret:%x", reg, ret);

    return 0;
}

int do_cmd_fast_read(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    int32_t state;
    spi2apb_state_query(pdata->spi, &state);
    dev_info(pdata->dev, "dsp state %x", state);

    return 0;
}

int do_cmd_queue_init(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    int state = 0;
    state = dsp_msq_init(pdata->spi);
    dev_info(pdata->dev, "message queue init state: %d", state);

    return 0;
}

int do_cmd_send_msg(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    struct msg *m;
    int ret = 0;
    int msg_len;
    int i = 0;

    if (args->argc < 2) {
        dev_err(pdata->dev, "need more args");
        return -1;
    }

    msg_len = args->argc * sizeof(uint32_t);

    m = (struct msg *)kmalloc(msg_len, GFP_KERNEL);
    m->size = msg_len / 4;
    for (i = 1; i < m->size; i++) {
        if (args->argv[i].type != AUTO_ARG_TYPE_INT32) {
            dev_err(pdata->dev, "Unknown args!");
            kfree(m);
            return -1;
        }

        *((int32_t*)m + i) = args->argv[i].m_int32;
    }

    ret = preisp_send_msg_to_dsp(pdata, m);

    dev_info(pdata->dev, "send msg len: %d, ret: %x",
            m->size, ret);

    kfree(m);

    return 0;
}

int do_cmd_recv_msg(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    struct msg *m;
    char buf[256] = "";
    int ret = 0;

    ret = dsp_msq_recv_msg(pdata->spi, &m);
    if (ret || m == NULL)
        return 0;

    dev_info(pdata->dev, "\nrecv msg len: %d, ret: %x",
            m->size, ret);
    int32_hexdump("recv msg:", (int32_t*)m, m->size * 4);

    dev_info(pdata->dev, buf);

    dsp_msq_free_received_msg(m);

    return 0;
}

int do_cmd_power_on(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    int ret;
    ret = rkpreisp_power_on(pdata,1);
    dev_info(pdata->dev, "do cmd power on, count++");
    return ret;
}

int do_cmd_power_off(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    int ret;
    ret = rkpreisp_power_off(pdata);
    dev_info(pdata->dev, "do cmd power off, count--");
    return ret;
}

int do_cmd_set_dsp_log_level(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    int ret;

    if (args->argc != 2 && args->argv[1].type != AUTO_ARG_TYPE_INT32) {
        dev_err(pdata->dev, "Mis or unknown args!");
        return -1;
    }

    pdata->log_level = args->argv[1].m_int32;
    ret = rkpreisp_set_log_level(pdata, pdata->log_level);

    dev_info(pdata->dev, "set dsp log level %d, ret: %d",
            pdata->log_level, ret);

    return ret;
}

int do_cmd_request_bypass(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    return rkpreisp_request_sleep(pdata, PREISP_SLEEP_MODE_BYPASS);
}

int do_cmd_request_sleep(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    return rkpreisp_request_sleep(pdata, PREISP_SLEEP_MODE_STANDBY);
}

int do_cmd_version(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    dev_info(pdata->dev, "driver version: %s", RKPREISP_VERSION);
    return 0;
}

int do_cmd_wakeup(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    return rkpreisp_wakeup(pdata);
}
#ifdef ZD552KL_PHOENIX
int do_cmd_set_core_voltage(struct spi_rk_preisp_data *pdata,
        const struct auto_args *args)
{
    int ret=0;
	uint32_t val=0;
    if (args->argc != 2 && args->argv[1].type != AUTO_ARG_TYPE_INT32) {
        dev_err(pdata->dev, "Mis or unknown args!");
        return -1;
    }
	val=(uint32_t)args->argv[1].m_int32;
	if(val<=PREISP_MAX_CORE_VOLTAGE)
	{
		g_cur_core_voltage=val;
		dev_info(pdata->dev, "set dsp core voltage %d", val);
	}
	else
		dev_info(pdata->dev,"new value(%d) must not larger than %d",val,PREISP_MAX_CORE_VOLTAGE);

    return ret;
}
#endif

int do_cmd(struct spi_rk_preisp_data *pdata,
    const struct auto_args *args)
{
    const char * s;
    if (args->argv->type != AUTO_ARG_TYPE_STR)
        return 0;

    s = args->argv->m_str;
    //echo bypass > /dev/rk_preisp
    if (!strcmp(s, "bypass")) return do_cmd_request_bypass(pdata, args);
    //echo c > /dev/rk_preisp
    if (!strcmp(s, "c")) return do_cmd_recv_msg(pdata, args);
    //echo f [fw_name] > /dev/rk_preisp
    if (!strcmp(s, "f")) return do_cmd_download_fw(pdata, args);
    //echo fw reg1 > /dev/rk_preisp
    if (!strcmp(s, "fw")) return do_cmd_fast_write(pdata, args);
    //echo fr > /dev/rk_preisp
    if (!strcmp(s, "fr")) return do_cmd_fast_read(pdata, args);
    //echo i > /dev/rk_preisp
    if (!strcmp(s, "i")) return do_cmd_queue_init(pdata, args);
    //echo log level > /dev/rk_preisp
    if (!strcmp(s, "log")) return do_cmd_set_dsp_log_level(pdata, args);
    //echo on > /dev/rk_preisp
    if (!strcmp(s, "on")) return do_cmd_power_on(pdata, args);
    //echo off > /dev/rk_preisp
    if (!strcmp(s, "off")) return do_cmd_power_off(pdata, args);
    //echo q > /dev/rk_preisp
    if (!strcmp(s, "q")) return do_cmd_query(pdata, args);
    //echo r addr [length] > /dev/rk_preisp
    if (!strcmp(s, "r")) return do_cmd_read(pdata, args);
    //echo rate > /dev/rk_preisp
    if (!strcmp(s, "rate")) return do_cmd_set_spi_rate(pdata, args);
    //echo s type,... > /dev/rk_preisp
    if (!strcmp(s, "s")) return do_cmd_send_msg(pdata, args);
    //echo sleep > /dev/rk_preisp
    if (!strcmp(s, "sleep")) return do_cmd_request_sleep(pdata, args);
    //echo v > /dev/rk_preisp
    if (!strcmp(s, "v")) return do_cmd_version(pdata, args);
    //echo w addr value,... > /dev/rk_preisp
    if (!strcmp(s, "w")) return do_cmd_write(pdata, args);
    //echo wake > /dev/rk_preisp
    if (!strcmp(s, "wake")) return do_cmd_wakeup(pdata, args);
	//modify core_valtage
#ifdef ZD552KL_PHOENIX
	if (!strcmp(s, "core_voltage")) return do_cmd_set_core_voltage(pdata, args);
#endif
    return 0;
}

static int rkpreisp_open(struct inode *inode, struct file *file)
{
    struct spi_rk_preisp_data *pdata =
        container_of(file->private_data, struct spi_rk_preisp_data, misc);

    preisp_client *client = preisp_client_new();
#ifdef ZD552KL_PHOENIX
    bool do_resume = false;
#endif
    client->private_data = pdata;
    file->private_data = client;
	
#ifdef ZD552KL_PHOENIX
    mutex_lock(&pdata->resume_lock);
    //ASUS_BSP +++ Zhengwei "enter standby sleep if open preisp before resume"
    if(atomic_read(&pdata->power_on_cnt) == 0 && pdata->is_suspended)
    {
        do_resume = true;
    }
#endif
    rkpreisp_power_on(pdata,1);
#ifdef ZD552KL_PHOENIX
    if(do_resume)
    {
        mdelay(10); /*delay for dsp boot*/
        dev_info(pdata->dev, "open preisp before resume, sleep first, then wakeup\n");
        rkpreisp_request_sleep(pdata, PREISP_SLEEP_MODE_STANDBY);
        rkpreisp_power_on(pdata,1);
    }
    //ASUS_BSP --- Zhengwei "enter standby sleep if open preisp before resume"
    mutex_unlock(&pdata->resume_lock);
#endif
    return 0;
}

static int rkpreisp_release(struct inode *inode, struct file *file)
{
    preisp_client *client = file->private_data;
    struct spi_rk_preisp_data *pdata = client->private_data;

    preisp_client_disconnect(pdata, client);
    preisp_client_release(client);
    rkpreisp_power_off(pdata);
    return 0;
}
#ifdef ZD552KL_PHOENIX
void set_fw_revision(const char *version_string)
{
	memset(g_preisp_data->fw_version,0,sizeof(g_preisp_data->fw_version));
	strcpy(g_preisp_data->fw_version,version_string);
	dev_info(g_preisp_data->dev,"fw version now is %s\n",g_preisp_data->fw_version);
}
static ssize_t rkpreisp_read(struct file *file,char __user *user_buf, size_t count, loff_t *ppos)
{
	int n;
	char kbuf[64];

	if(*ppos == 0)
	{
		if(atomic_read(&g_preisp_data->power_on_cnt)>0)
			n = snprintf(kbuf, sizeof(kbuf),"%s\n%c",g_preisp_data->fw_version,'\0');
		else
			n = snprintf(kbuf, sizeof(kbuf),"%s\n%c","Power Off",'\0');
		if(copy_to_user(user_buf,kbuf,n))
		{
			dev_err(g_preisp_data->dev,"%s(): copy_to_user fail!\n",__func__);
			return -EFAULT;
		}
		*ppos += n;
		return n;
	}
	return 0;
}
#endif
static ssize_t rkpreisp_write(struct file *file,
        const char __user *user_buf, size_t count, loff_t *ppos)
{
    char *buf;
    struct auto_args args;
    int i;
    preisp_client *client = file->private_data;
    struct spi_rk_preisp_data *pdata = client->private_data;

    buf = (char *)kmalloc(count + 1, GFP_KERNEL);
    if(copy_from_user(buf, user_buf, count))
        return -EFAULT;
    buf[count] = 0;

    i = 0;
    while (buf[i] != 0) {
        i += parse_auto_args(buf + i, &args);
        if (args.argc == 0)
            continue;

        do_cmd(pdata, &args);
        free_auto_args(&args);
    }

    kfree(buf);

    return count;
}

static void print_dsp_log(struct spi_rk_preisp_data *pdata, msg_dsp_log_t *log)
{
    char *str = (char *)(log);
    str[log->size * sizeof(int32_t) - 1] = 0;
    str += sizeof(msg_dsp_log_t);

    dev_info(pdata->dev, "DSP%d: %s", log->core_id, str);
}

static void dispatch_received_msg(struct spi_rk_preisp_data *pdata,
        struct msg *msg)
{
    preisp_client *client;

#if DEBUG_DUMP_ALL_SEND_RECV_MSG == 1
    int32_hexdump("recv msg:", (int32_t*)msg, msg->size*4);
#endif

    if (msg->type == id_msg_dsp_log_t) {
        print_dsp_log(pdata, (msg_dsp_log_t*)msg);
    } else if (msg->type == id_msg_do_i2c_t) {
        ap_i2c_do_xfer(pdata->spi, (msg_do_i2c_t*)msg);
    } else {
        mutex_lock(&pdata->clients.mutex);
        list_for_each_entry(client, &pdata->clients.list, list) {
            if (client->id == msg->camera_id) {
                msq_send_msg(&client->q, msg);
                wake_up_interruptible(&client->wait);
            }
        }
        mutex_unlock(&pdata->clients.mutex);
    }
}

static long rkpreisp_ioctl(struct file *file,
        unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    void __user *ubuf = (void __user *)arg;
    preisp_client *client = file->private_data;
    struct spi_rk_preisp_data *pdata = client->private_data;

    switch (cmd) {
    case PREISP_POWER_ON:
        ret = rkpreisp_power_on(pdata,1);
        break;
    case PREISP_POWER_OFF:
        ret = rkpreisp_power_off(pdata);
        break;
    case PREISP_REQUEST_SLEEP: {
        int sleep_mode = arg;
	pdata->do_force_sleep=true;
        ret = rkpreisp_request_sleep(pdata, sleep_mode);
        break;
    }
    case PREISP_WAKEUP:
#ifdef ZD552KL_PHOENIX
	pdata->do_force_wakeup=true;
#else
	pdata->do_force_sleep=true;
#endif
        ret = rkpreisp_wakeup(pdata);
        break;
    case PREISP_DOWNLOAD_FW: {
        char fw_name[PREISP_FW_NAME_LEN];
        if (strncpy_from_user(fw_name, ubuf, PREISP_FW_NAME_LEN) <= 0) {
            ret = -EINVAL;
            break;
        }
        dev_info(pdata->dev, "download fw:%s", fw_name);
        ret = spi_download_fw(pdata->spi, fw_name,pdata->fw_speed_hz,pdata->max_speed_hz);
        break;
    }
    case PREISP_APB_WRITE: {
        struct preisp_apb_pkt pkt;
        int32_t *data;

        if (copy_from_user(&pkt, ubuf, sizeof(pkt))) {
            ret = -EINVAL;
            break;
        }
        pkt.data_len = MIN(pkt.data_len, APB_MAX_OP_BYTES);
        data = kmalloc(pkt.data_len, GFP_KERNEL);
        if (copy_from_user(data, (void __user*)pkt.data, pkt.data_len)) {
            kfree(data);
            ret = -EINVAL;
            break;
        }
        ret = spi2apb_safe_write(pdata->spi, pkt.addr, data, pkt.data_len);
        kfree(data);
        break;
    }
    case PREISP_APB_READ: {
        struct preisp_apb_pkt pkt;
        int32_t *data;

        if (copy_from_user(&pkt, ubuf, sizeof(pkt))) {
            ret = -EINVAL;
            break;
        }
        pkt.data_len = MIN(pkt.data_len, APB_MAX_OP_BYTES);
        data = kmalloc(pkt.data_len, GFP_KERNEL);
        ret = spi2apb_safe_read(pdata->spi, pkt.addr, data, pkt.data_len);
        if (ret) {
            kfree(data);
            break;
        }
        ret = copy_to_user((void __user*)pkt.data, data, pkt.data_len);

        kfree(data);
        break;
    }
    case PREISP_ST_QUERY: {
        int32_t state;
        ret = spi2apb_state_query(pdata->spi, &state);
        if (ret)
            break;

        ret = put_user(state, (int32_t __user *)ubuf);
        break;
    }
    case PREISP_IRQ_REQUEST: {
        int int_num = arg;
        ret = spi2apb_interrupt_request(pdata->spi, int_num);
        break;
    }
    case PREISP_SEND_MSG: {
        struct msg *msg;
        uint32_t len;

        if (get_user(len, (uint32_t __user*)ubuf)) {
            ret = -EINVAL;
            break;
        }
        len = len * sizeof(int32_t);
        msg = kmalloc(len, GFP_KERNEL);
        if (copy_from_user(msg, ubuf, len)) {
            kfree(msg);
            ret = -EINVAL;
            break;
        }
#if DEBUG_DUMP_ALL_SEND_RECV_MSG == 1
        int32_hexdump("send msg:", (int32_t*)msg, len);
#endif

#if DEBUG_MSG_LOOP_TEST == 0
        ret = preisp_send_msg_to_dsp(pdata, msg);
#else
        dispatch_received_msg(pdata, msg);
#endif
        kfree(msg);
        break;
    }
    case PREISP_QUERY_MSG: {
        struct msg *msg;
        ret = msq_recv_msg(&client->q, &msg);
        if (ret)
            break;

        ret = put_user(msg->size, (uint32_t __user*)ubuf);
        break;
    }
    case PREISP_RECV_MSG: {
        struct msg *msg;
        ret = msq_recv_msg(&client->q, &msg);
        if (ret)
            break;
        ret = copy_to_user(ubuf, msg, msg->size * sizeof(uint32_t));
        msq_free_received_msg(&client->q, msg);
        break;
    }
    case PREISP_CLIENT_CONNECT:
    {
        int id = arg;
        client->id = id;
        ret = preisp_client_connect(pdata, client);
        break;
    }
    case PREISP_CLIENT_DISCONNECT:
    {
        preisp_client_disconnect(pdata, client);
        client->id = INVALID_ID;
        break;
    }
    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

static unsigned int rkpreisp_poll(struct file *file, poll_table *wait)
{
    preisp_client *client = file->private_data;
    unsigned int mask = 0;

    poll_wait(file, &client->wait, wait);

    if (!msq_is_empty(&client->q))
        mask |= POLLIN;

    return mask;
}

static const struct file_operations rkpreisp_fops = {
    .owner = THIS_MODULE,
    .open = rkpreisp_open,
    .release = rkpreisp_release,
    .write = rkpreisp_write,
#ifdef ZD552KL_PHOENIX
    .read = rkpreisp_read,
#endif
    .poll = rkpreisp_poll,
    .unlocked_ioctl = rkpreisp_ioctl,
    .compat_ioctl = rkpreisp_ioctl,
};

static irqreturn_t rkpreisp_threaded_isr(int irq, void *dev_id)
{
    struct spi_rk_preisp_data *pdata = dev_id;
    struct msg *msg;

    BUG_ON(irq != pdata->irq);


    while (!dsp_msq_recv_msg(pdata->spi, &msg) && msg != NULL) {
        dispatch_received_msg(pdata, msg);
        dsp_msq_free_received_msg(msg);
    }

    return IRQ_HANDLED;
}

static irqreturn_t rkpreisp_sleep_isr(int irq, void *dev_id)
{
    struct spi_rk_preisp_data *pdata = dev_id;

    BUG_ON(irq != pdata->sleepst_irq);
    if (pdata->powerdown_gpio > 0) {
        gpio_set_value(pdata->powerdown_gpio, !pdata->powerdown_active);
    }
#ifdef ZD552KL_PHOENIX
	if(pdata->is_standby_sleep_mode == 1)
		queue_work(sleep_wq,&(sleep_work.work));
#endif

    pdata->sleep_state_flag = 1;

    return IRQ_HANDLED;
}

static int rkpreisp_parse_dt_property(struct device *dev,
                  struct spi_rk_preisp_data *pdata)
{
    int ret = 0;
    struct device_node *node = dev->of_node;
    enum of_gpio_flags flags;;

    if (!node)
        return -ENODEV;

    of_property_read_u32(node, "spi-max-frequency",
            &pdata->max_speed_hz);

    ret = of_property_read_u32(node, "spi-min-frequency",
            &pdata->min_speed_hz);
    if (ret) {
        dev_warn(dev, "can not get spi-min-frequency!");
        pdata->min_speed_hz = pdata->max_speed_hz / 2;
    }

    ret = of_property_read_u32(node, "spi-fw-frequency",
            &pdata->fw_speed_hz);
    if(ret)
    {
		dev_warn(dev, "can not get spi-fw-frequency, set it equal to max_speed_hz %d!",pdata->max_speed_hz);
		pdata->fw_speed_hz = pdata->max_speed_hz;
    }

    pdata->mclk = devm_clk_get(dev, "mclk");
    if (IS_ERR(pdata->mclk)) {
        dev_warn(dev, "can not get mclk, error %ld\n", PTR_ERR(pdata->mclk));
        pdata->mclk = NULL;
    }

    ret = of_get_named_gpio_flags(node, "reset-gpio", 0, &flags);
    if (ret <= 0) {
        dev_warn(dev, "can not find property reset-gpio, error %d\n", ret);
    }

    pdata->reset_gpio = ret;
    pdata->reset_active = 1;
    if (flags == OF_GPIO_ACTIVE_LOW) {
        pdata->reset_active = 0;
    }

    if (pdata->reset_gpio > 0) {
        ret = devm_gpio_request(dev, pdata->reset_gpio, "preisp-reset");
        if (ret) {
            dev_err(dev, "gpio %d request error %d\n", pdata->reset_gpio, ret);
            return ret;
        }

        ret = gpio_direction_output(pdata->reset_gpio, !pdata->reset_active);
        if (ret) {
            dev_err(dev, "gpio %d direction output error %d\n",
                    pdata->reset_gpio, ret);
            return ret;
        }
    }

	
	//asus bsp ralf:add vio gpio info>>
	ret = of_get_named_gpio_flags(node, "vio-en-gpio", 0, &flags);
	if (ret <= 0) {
        dev_warn(dev, "can not find property vio-en-gpio, error %d\n", ret);
    }else
    	pdata->vio_en_gpio= ret;
    
	ret = of_get_named_gpio_flags(node, "snapshot-gpio", 0, &flags);
	if (ret <= 0) {
        dev_warn(dev, "can not find property snapshot_gpio, error %d\n", ret);
    }else {
		pdata->snapshot_gpio= ret;
		pdata->snapshot_active = 1;
	    if (flags == OF_GPIO_ACTIVE_LOW) {
	        pdata->snapshot_active = 0;
	    }
		
		ret = devm_gpio_request(dev, pdata->snapshot_gpio, "preisp-snapshot");
	    if (ret) {
	        dev_err(dev, "gpio %d request error %d\n", pdata->snapshot_gpio, ret);
	    }else {
	    	ret = gpio_direction_input(pdata->snapshot_gpio);
			if (ret) {
		        dev_err(dev, "gpio %d direction input error %d\n",pdata->snapshot_gpio, ret);
		    }
	    }
    }
	//asus bsp ralf:add vio gpio info<<
	
    ret = of_get_named_gpio_flags(node, "irq-gpio", 0, NULL);
    if (ret <= 0) {
        dev_warn(dev, "can not find property irq-gpio, error %d\n", ret);
        return ret;
    }

    pdata->irq_gpio = ret;

    ret = devm_gpio_request(dev, pdata->irq_gpio, "preisp-irq");
    if (ret) {
        dev_err(dev, "gpio %d request error %d\n", pdata->irq_gpio, ret);
        return ret;
    }

    ret = gpio_direction_input(pdata->irq_gpio);
    if (ret) {
        dev_err(dev, "gpio %d direction input error %d\n",
                pdata->irq_gpio, ret);
        return ret;
    }

    ret = gpio_to_irq(pdata->irq_gpio);
    if (ret < 0) {
        dev_err(dev, "Unable to get irq number for GPIO %d, error %d\n",
            pdata->irq_gpio, ret);
        return ret;
    }
    pdata->irq = ret;
    ret = request_threaded_irq(pdata->irq, NULL, rkpreisp_threaded_isr,
            IRQF_TRIGGER_RISING | IRQF_ONESHOT, "preisp-irq", pdata); 
    if (ret) {
        dev_err(dev, "cannot request thread irq: %d\n", ret);
        return ret;
    }

    disable_irq(pdata->irq);

    ret = of_get_named_gpio_flags(node, "powerdown-gpio", 0, &flags);
    if (ret <= 0) {
        dev_warn(dev, "can not find property powerdown-gpio, error %d\n", ret);
    }

    pdata->powerdown_gpio = ret;
    pdata->powerdown_active = 1;
    if (flags == OF_GPIO_ACTIVE_LOW) {
        pdata->powerdown_active = 0;
    }

    if (pdata->powerdown_gpio > 0) {
        ret = devm_gpio_request(dev, pdata->powerdown_gpio, "preisp-powerdown");
        if (ret) {
            dev_err(dev, "gpio %d request error %d\n", pdata->powerdown_gpio, ret);
            return ret;
        }

        ret = gpio_direction_output(pdata->powerdown_gpio, !pdata->powerdown_active);
        if (ret) {
            dev_err(dev, "gpio %d direction output error %d\n",
                    pdata->powerdown_gpio, ret);
            return ret;
        }
    }

    pdata->sleepst_gpio = -1;
    pdata->sleepst_irq = -1;
    pdata->wakeup_gpio = -1;

    ret = of_get_named_gpio_flags(node, "sleepst-gpio", 0, NULL);
    if (ret <= 0) {
        dev_warn(dev, "can not find property sleepst-gpio, error %d\n", ret);
        return 0;
    }

    pdata->sleepst_gpio = ret;

    ret = devm_gpio_request(dev, pdata->sleepst_gpio, "preisp-sleep-irq");
    if (ret) {
        dev_err(dev, "gpio %d request error %d\n", pdata->sleepst_gpio, ret);
        return 0;
    }

    ret = gpio_direction_input(pdata->sleepst_gpio);
    if (ret) {
        dev_err(dev, "gpio %d direction input error %d\n",
                pdata->sleepst_gpio, ret);
        return ret;
    }

    ret = gpio_to_irq(pdata->sleepst_gpio);
    if (ret < 0) {
        dev_err(dev, "Unable to get irq number for GPIO %d, error %d\n",
            pdata->sleepst_gpio, ret);
        return ret;
    }
    pdata->sleepst_irq = ret;
    ret = request_any_context_irq(pdata->sleepst_irq, rkpreisp_sleep_isr,
            IRQF_TRIGGER_RISING, "preisp-sleep-irq", pdata);
    disable_irq(pdata->sleepst_irq);

    ret = of_get_named_gpio_flags(node, "wakeup-gpio", 0, &flags);
    if (ret <= 0) {
        dev_warn(dev, "can not find property wakeup-gpio, error %d\n", ret);
    }

    pdata->wakeup_gpio = ret;
    pdata->wakeup_active = 1;
    if (flags == OF_GPIO_ACTIVE_LOW) {
        pdata->wakeup_active = 0;
    }

    if (pdata->wakeup_gpio > 0) {
        ret = devm_gpio_request(dev, pdata->wakeup_gpio, "preisp-wakeup");
        if (ret) {
            dev_err(dev, "gpio %d request error %d\n", pdata->wakeup_gpio, ret);
            return ret;
        }

        ret = gpio_direction_output(pdata->wakeup_gpio, !pdata->wakeup_active);
        if (ret) {
            dev_err(dev, "gpio %d direction output error %d\n",
                    pdata->wakeup_gpio, ret);
            return ret;
        }
    }
	//dev_info(dev,"reading firmware-nowait-mode");
    ret = of_property_read_u32(node, "firmware-nowait-mode", &pdata->fw_nowait_mode);
    if (ret) {
        dev_warn(dev, "can not get firmware-nowait-mode!"); 
        pdata->fw_nowait_mode = 0;
    }
	//pdata->fw_nowait_mode=1;
    return ret;
}

static int spi_rk_preisp_probe(struct spi_device *spi)
{
    struct spi_rk_preisp_data *data;
    int err;

    dev_info(&spi->dev, "rk preisp spi probe start, verison:%s",
            RKPREISP_VERSION);
    data = devm_kzalloc(&spi->dev, sizeof(*data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }
    atomic_set(&data->power_on_cnt, 0);
    atomic_set(&data->wake_sleep_cnt, 0);
    preisp_client_list_init(&data->clients);
    mutex_init(&data->send_msg_lock);
    mutex_init(&data->power_lock);
    mutex_init(&data->wake_sleep_lock);
#ifdef ZD552KL_PHOENIX
    mutex_init(&data->resume_lock);
#endif
    wake_lock_init(&data->resume_wake_lock, WAKE_LOCK_SUSPEND, "Preisp_Wake_Lock");
    data->spi = spi;
    data->dev = &spi->dev;

#ifdef ZD552KL_PHOENIX
    g_preisp_data = data;
    sleep_wq = create_workqueue("sleep wq");
    sleep_work.pdata = data;
    INIT_WORK(&(sleep_work.work), sleep_operation);
    set_fw_revision("NotLoad");
#endif

    rkpreisp_parse_dt_property(data->dev, data);

    spi_set_drvdata(spi, data);
    data->log_level = LOG_INFO;

    data->misc.minor = MISC_DYNAMIC_MINOR;
    data->misc.name = "rk_preisp";
    data->misc.fops = &rkpreisp_fops;

#ifdef ZD552KL_PHOENIX
    data->do_force_wakeup=false;
#else
    g_preisp_data = data;
#endif
    data->do_force_sleep=false;

    err = misc_register(&data->misc);
    if (err < 0) {
        dev_err(data->dev, "Error: misc_register returned %d", err);
    }

    ///////////////////////////////
   // printk("%s:%d enable pinctrl \n",__func__,__LINE__);
    err=msm_camera_pinctrl_init(&data->pinctrl_info,data->dev);
    if(err<0)
    {
       // printk("%s:%d enable pinctrl FAIL!!!!!!!!!!.\n",__func__,__LINE__);
        data->pinctrl_status = 0;
    }else
    {
        data->pinctrl_status = 1;
    }
    err=msm_camera_get_clk_info_internal(data->dev,&data->clk_info,&data->clk_ptr,&data->num_clk);
    if(err<0)
    {
        printk("%s:%d get clk info FAIL!!!!!!!!!!\n",__func__,__LINE__);
    }
    if(!data->clk_ptr)
    {
        printk("%s:%d mclk is NULL !!!!!!!!!!!! \n",__func__,__LINE__);
    }
#ifdef ZD552KL_PHOENIX
	//ASUS_BSP Zhengwei +++ "late resume preisp"
	data->fb_notif.notifier_call = fb_notifier_callback;
	err = fb_register_client(&data->fb_notif);
	if (err) {
		dev_err(data->dev, "Unable to register fb_notifier: %d\n",
			err);
	}
	//ASUS_BSP Zhengwei --- "late resume preisp"
#endif

	if (!data->fw_nowait_mode) {
        return 0;
    }
    err = request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
            RKL_DEFAULT_FW_NAME, data->dev, GFP_KERNEL, data, fw_nowait_power_on);
    if (err) {
        dev_err(data->dev, "request firmware nowait failed!");
    }

    return 0;
}

static int spi_rk_preisp_remove(struct spi_device *spi)
{
    struct spi_rk_preisp_data *data = spi_get_drvdata(spi);
    spi_set_drvdata(spi, NULL);
#ifdef ZD552KL_PHOENIX
    fb_unregister_client(&data->fb_notif);//ASUS_BSP Zhengwei "late resume preisp"
#endif
    misc_deregister(&data->misc);
    return 0;
}

static int spi_rk_preisp_suspend(struct spi_device *spi, pm_message_t mesg)
{
    struct spi_rk_preisp_data *pdata = spi_get_drvdata(spi);

    if (!pdata->fw_nowait_mode) {
        return 0;
    }
#ifdef ZD552KL_PHOENIX
    pdata->is_suspended = true;
#endif
    rkpreisp_power_off(pdata);
    return 0;
}

static int spi_rk_preisp_resume(struct spi_device *spi)
{
    struct spi_rk_preisp_data *pdata = spi_get_drvdata(spi);
    int ret = 0;

    if (pdata->powerdown_gpio > 0) {
        gpio_direction_output(pdata->powerdown_gpio, !pdata->powerdown_active);
    }
    if (pdata->wakeup_gpio > 0) {
        gpio_direction_output(pdata->wakeup_gpio, !pdata->wakeup_active);
    }
    if (pdata->reset_gpio > 0) {
        gpio_direction_output(pdata->reset_gpio, !pdata->reset_active);
    }
    if (!pdata->fw_nowait_mode)
        return 0;

    wake_lock(&pdata->resume_wake_lock);
#ifdef ZD552KL_PHOENIX
    mutex_lock(&pdata->resume_lock);
#endif
    dev_info(pdata->dev,"resume, going to do power up\n");
    pdata->is_resume_processing = true;
    ret = request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
            RKL_DEFAULT_FW_NAME, pdata->dev, GFP_KERNEL, pdata, fw_nowait_power_on);
    if (ret) {
        dev_err(pdata->dev, "request firmware nowait failed!");
        wake_unlock(&pdata->resume_wake_lock);
#ifdef ZD552KL_PHOENIX
        mutex_unlock(&pdata->resume_lock);
#endif
    }

    return ret;
}

#ifdef ZD552KL_PHOENIX
//ASUS_BSP +++ Zhengwei "late resume preisp"
static int fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct fb_event *evdata = (struct fb_event *)data;
	int *blank;
	struct spi_rk_preisp_data *pdata = container_of(self, struct spi_rk_preisp_data, fb_notif);

	blank = evdata->data;
	dev_dbg(pdata->dev, "fb event is %lu, evdata data %d",event,*blank);
	if (evdata && evdata->data && pdata==g_preisp_data)
	{
		if(event == FB_EVENT_SUSPEND)
		{
			//spi_rk_preisp_suspend(pdata->spi,PMSG_SUSPEND);
		}
		else if(event == FB_EARLY_EVENT_BLANK)
		{
			if(*blank == FB_BLANK_UNBLANK)
			{
				if(atomic_read(&pdata->power_on_cnt) == 0)
				{
					spi_rk_preisp_resume(pdata->spi);
				}
				else
				{
					dev_info(pdata->dev, "preisp not power off, not resume it\n");
				}
			}
		}
	}
	else
	{
		printk("preisp, error! pdata %p, g_preisp_data %p\n", pdata, g_preisp_data);
	}

	return 0;
}
//ASUS_BSP --- Zhengwei "late resume preisp"
#endif

/*
dts:
    spi_rk_preisp@xx {
        compatible =  "rockchip,spi_rk_preisp";
        reg = <x>;
        spi-max-frequency = <48000000>;
        //spi-cpol;
        //spi-cpha;
        firmware-nowait-mode = <0>;
        reset-gpio = <&gpio6 GPIO_A0 GPIO_ACTIVE_HIGH>;
        irq-gpio = <&gpio6 GPIO_A2 GPIO_ACTIVE_HIGH>;
        clocks = <&xxx>;
        clock-names = "mclk";

        //TODO:
        //regulator config...
    };
*/

static const struct of_device_id spi_rk_preisp_dt_match[] = {
    { .compatible = "rockchip,spi_rk_preisp", },
    {},
};
MODULE_DEVICE_TABLE(of, spi_rk_preisp_dt_match);

static struct spi_driver spi_rk_preisp_driver = {
    .driver = {
        .name   = "spi_rk_preisp",
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(spi_rk_preisp_dt_match),
    },
    .probe      = spi_rk_preisp_probe,
    .remove     = spi_rk_preisp_remove,
    .suspend    = spi_rk_preisp_suspend,
#ifdef ZD552KL_PHOENIX
    //.resume     = spi_rk_preisp_resume, //ASUS_BSP Zhengwei "late resume preisp"
#else
    .resume     = spi_rk_preisp_resume,
#endif
};
module_spi_driver(spi_rk_preisp_driver);

MODULE_AUTHOR("Tusson <dusong@rock-chips.com>");
MODULE_DESCRIPTION("Rockchip spi interface for PreIsp");
MODULE_LICENSE("GPL");
