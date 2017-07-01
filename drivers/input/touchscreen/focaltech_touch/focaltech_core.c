/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2016, FocalTech Systems, Ltd., all rights reserved.
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
/*****************************************************************************
*
* File Name: focaltech_core.c
*
* Author: Focaltech Driver Team
*
* Created: 2016-08-08
*
* Abstract:
*
* Reference:
*
*****************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include "focaltech_core.h"

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#define FTS_SUSPEND_LEVEL 1     /* Early-suspend level */
#endif

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define FTS_DRIVER_NAME                     "fts_tstouch"
#define INTERVAL_READ_REG                   20  //interval time per read reg unit:ms
#define TIMEOUT_READ_REG                    300 //timeout of read reg unit:ms
#if FTS_POWER_SOURCE_CUST_EN
#define FTS_VTG_MIN_UV                      2600000
#define FTS_VTG_MAX_UV                      3300000
#define FTS_I2C_VTG_MIN_UV                  1800000
#define FTS_I2C_VTG_MAX_UV                  1800000
#endif
#define FTS_READ_TOUCH_BUFFER_DIVIDED       0
/*****************************************************************************
* Global variable or extern global variabls/functions
******************************************************************************/
struct i2c_client *fts_i2c_client;
struct fts_ts_data *fts_wq_data;
struct input_dev *fts_input_dev;
extern int asus_lcd_id;
//<ASUS-BSP Robert_He 20170310> add switch touch firmware node ++++++
u8  asus_firmware_id_touch = 0;
u8  asus_vendor_id_touch = 0;
//<ASUS-BSP Robert_He 20170310> add switch touch firmware node ------
//<ASUS-BSP Robert_He 20170310> add touch test node ++++++
bool FOCAL_TOUCH_DISABLE = false;
//<ASUS-BSP Robert_He 20170310> add touch test node ------



#if FTS_DEBUG_EN
int g_show_log = 1;
#else
int g_show_log = 0;
#endif

#if (FTS_DEBUG_EN && (FTS_DEBUG_LEVEL == 2))
char g_sz_debug[1024] = { 0 };
#endif

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
static void fts_release_all_finger(void);
static int fts_ts_suspend(struct device *dev);
static int fts_ts_resume(struct device *dev);
//<ASUS-BSP Robert_He 20170310> add touch test node ++++++
static ssize_t focal_touch_function_show(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t focal_touch_function_store(struct device *dev,struct device_attribute *attr,const char *buf,size_t count);
static ssize_t focal_touch_status_show(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t focal_touch_error_store(struct device *dev,struct device_attribute *attr,const char *buf,size_t count);
static ssize_t focal_touch_vendor_show(struct device *dev,struct device_attribute *attr,char *buf);


static struct device_attribute attrs[] = {
	__ATTR(touch_function,(S_IRUGO | 0220),
			focal_touch_function_show,
			focal_touch_function_store),
	__ATTR(touch_status,S_IRUGO,
			focal_touch_status_show,
			focal_touch_error_store),
	__ATTR(TP_Vender,S_IRUGO,
			focal_touch_vendor_show,
			focal_touch_error_store),
};


static ssize_t focal_touch_function_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if(FOCAL_TOUCH_DISABLE)
		return snprintf(buf,PAGE_SIZE, "%d\n",0);
	else
		return snprintf(buf,PAGE_SIZE, "%d\n",1);
}

static ssize_t focal_touch_function_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	if (input == 1)
		FOCAL_TOUCH_DISABLE= false;
	else if (input == 0)
		FOCAL_TOUCH_DISABLE = true;
	else
		return -EINVAL;

	return count;
}


static ssize_t focal_touch_status_show(struct device *dev,struct device_attribute *attr,char *buf)
{
	int retval;
	u8 status = 0;
	retval = fts_i2c_read_reg(fts_wq_data->client,FTS_REG_CHIP_ID,&status);
	if (retval < 0) {
		printk("[TOUCH]:%s: Failed to read device status, error = %d\n",
				__func__, retval);
		return snprintf(buf, PAGE_SIZE, "%d\n",0);	
	}
	
	if(status == 0)
	{
		return snprintf(buf, PAGE_SIZE, "%d\n",0);
	}
	else
	{
		printk("[TOUCH]: %s : Touch IC Status is %d\n",__func__,status);
		return snprintf(buf, PAGE_SIZE, "%d\n",1);	
	}
}

static ssize_t focal_touch_error_store(struct device *dev,struct device_attribute *attr,const char *buf,size_t count)
{
	dev_warn(dev, "%s Attempted to write to read-only attribute %s\n",
			__func__, attr->attr.name);
	return -EPERM;
}


static ssize_t focal_touch_vendor_show(struct device *dev,struct device_attribute *attr,char *buf)
{
	int retval;
	u8 status = 0;
	retval = fts_i2c_read_reg(fts_wq_data->client,FTS_REG_VENDOR_ID,&status);
	if (retval < 0) {
		printk("[TOUCH]:%s: Failed to read device status, error = %d\n",
				__func__, retval);
		return snprintf(buf, PAGE_SIZE, "%d\n",0);	
	}
	
	if(status == 0)
	{
		return snprintf(buf, PAGE_SIZE, "%d\n",0);
	}
	else
	{
		printk("[TOUCH]: %s : Touch IC Status is %d\n",__func__,status);
		return snprintf(buf, PAGE_SIZE, "%x\n",status);	
	}
}
//<ASUS-BSP Robert_He 20170310> add touch test node ------



/*****************************************************************************
*  Name: fts_wait_tp_to_valid
*  Brief:   Read chip id until TP FW become valid, need call when reset/power on/resume...
*           1. Read Chip ID per INTERVAL_READ_REG(20ms)
*           2. Timeout: TIMEOUT_READ_REG(300ms)
*  Input:
*  Output:
*  Return: 0 - Get correct Device ID
*****************************************************************************/
int fts_wait_tp_to_valid(struct i2c_client *client)
{
    int ret = 0;
    int cnt = 0;
    u8 reg_value = 0;
	//<ASUS-BSP Robert_He 20170328 use touch chip ic id +++++

    do {
        ret = fts_i2c_read_reg(client, FTS_REG_CHIP_ID, &reg_value);
        if ((ret < 0) || (reg_value != FTS_582C_CHIP_ID1)) {
            FTS_INFO("TP Not Ready, ReadData = 0x%x", reg_value);
        }
        else if (reg_value == FTS_582C_CHIP_ID1) {
            FTS_INFO("TP Ready, Device ID = 0x%x", reg_value);
            return 0;
        }
        cnt++;
        msleep(INTERVAL_READ_REG);
    }
    while ((cnt * INTERVAL_READ_REG) < TIMEOUT_READ_REG);
	//<ASUS-BSP Robert_He 20170328 use touch chip ic id -----

    /* error: not get correct reg data */
    return -1;
}

/*****************************************************************************
*  Name: fts_recover_state
*  Brief: Need execute this function when reset
*  Input:
*  Output:
*  Return:
*****************************************************************************/
void fts_tp_state_recovery(struct i2c_client *client)
{
    /* wait tp stable */
    fts_wait_tp_to_valid(client);
    /* recover TP charger state 0x8B */
    /* recover TP glove state 0xC0 */
    /* recover TP cover state 0xC1 */
    fts_ex_mode_recovery(client);

#if FTS_PSENSOR_EN
    fts_proximity_recovery(client);
#endif

    /* recover TP gesture state 0xD0 */
#if FTS_GESTURE_EN
    fts_gesture_recovery(client);
#endif

    fts_release_all_finger();
}

/*****************************************************************************
*  Name: fts_reset_proc
*  Brief: Execute reset operation
*  Input: hdelayms - delay time unit:ms
*  Output:
*  Return:
*****************************************************************************/
int fts_reset_proc(int hdelayms)
{
    gpio_direction_output(fts_wq_data->pdata->reset_gpio, 0);
    msleep(20);
    gpio_direction_output(fts_wq_data->pdata->reset_gpio, 1);
    msleep(hdelayms);

    return 0;
}

/*****************************************************************************
* Power Control
*****************************************************************************/
#if FTS_POWER_SOURCE_CUST_EN
static int fts_power_source_init(struct fts_ts_data *data)
{
    int rc;

    FTS_FUNC_ENTER();

    data->vdd = regulator_get(&data->client->dev, "vdd");
    if (IS_ERR(data->vdd)) {
        rc = PTR_ERR(data->vdd);
        FTS_ERROR("Regulator get failed vdd rc=%d", rc);
    }

    if (regulator_count_voltages(data->vdd) > 0) {
        rc = regulator_set_voltage(data->vdd, FTS_VTG_MIN_UV, FTS_VTG_MAX_UV);
        if (rc) {
            FTS_ERROR("Regulator set_vtg failed vdd rc=%d", rc);
            goto reg_vdd_put;
        }
    }

    data->vcc_i2c = regulator_get(&data->client->dev, "vcc_i2c");
    if (IS_ERR(data->vcc_i2c)) {
        rc = PTR_ERR(data->vcc_i2c);
        FTS_ERROR("Regulator get failed vcc_i2c rc=%d", rc);
        goto reg_vdd_set_vtg;
    }

    if (regulator_count_voltages(data->vcc_i2c) > 0) {
        rc = regulator_set_voltage(data->vcc_i2c, FTS_I2C_VTG_MIN_UV,
                                   FTS_I2C_VTG_MAX_UV);
        if (rc) {
            FTS_ERROR("Regulator set_vtg failed vcc_i2c rc=%d", rc);
            goto reg_vcc_i2c_put;
        }
    }

    FTS_FUNC_EXIT();
    return 0;

reg_vcc_i2c_put:
    regulator_put(data->vcc_i2c);
reg_vdd_set_vtg:
    if (regulator_count_voltages(data->vdd) > 0)
        regulator_set_voltage(data->vdd, 0, FTS_VTG_MAX_UV);
reg_vdd_put:
    regulator_put(data->vdd);
    FTS_FUNC_EXIT();
    return rc;
}

static int fts_power_source_ctrl(struct fts_ts_data *data, int enable)
{
    int rc;

    FTS_FUNC_ENTER();
    if (enable) {
        rc = regulator_enable(data->vdd);
        if (rc) {
            FTS_ERROR("Regulator vdd enable failed rc=%d", rc);
        }

        rc = regulator_enable(data->vcc_i2c);
        if (rc) {
            FTS_ERROR("Regulator vcc_i2c enable failed rc=%d", rc);
        }
    }
    else {
        rc = regulator_disable(data->vdd);
        if (rc) {
            FTS_ERROR("Regulator vdd disable failed rc=%d", rc);
        }
        rc = regulator_disable(data->vcc_i2c);
        if (rc) {
            FTS_ERROR("Regulator vcc_i2c disable failed rc=%d", rc);
        }
    }
    FTS_FUNC_EXIT();
    return 0;
}

#endif

/*****************************************************************************
*  Reprot related
*****************************************************************************/
/*****************************************************************************
*  Name: fts_release_all_finger
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void fts_release_all_finger(void)
{
#if FTS_MT_PROTOCOL_B_EN
    unsigned int finger_count = 0;

    for (finger_count = 0; finger_count < FTS_MAX_POINTS; finger_count++) {
        input_mt_slot(fts_input_dev, finger_count);
        input_mt_report_slot_state(fts_input_dev, MT_TOOL_FINGER, false);
    }
#else
    input_mt_sync(fts_input_dev);
#endif
    input_report_key(fts_input_dev, BTN_TOUCH, 0);
    input_sync(fts_input_dev);
}

/*****************************************************************************
*  Name: fts_read_touchdata
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_read_touchdata(struct fts_ts_data *data)
{
    u8 buf[POINT_READ_BUF] = { 0 };
    u8 pointid = FTS_MAX_ID;
    int ret = -1;
    int i;
    struct ts_event *event = &(data->event);

#if FTS_GESTURE_EN
    {
        u8 state;
        if (data->suspended) {
            fts_i2c_read_reg(data->client, FTS_REG_GESTURE_EN, &state);
            if (state == 1) {
                fts_gesture_readdata(data->client);
                return 1;
            }
        }
    }
#endif

#if FTS_PSENSOR_EN
    if ((fts_sensor_read_data(data) != 0) && (data->suspended == 1)) {
        return 1;
    }
#endif
#if FTS_READ_TOUCH_BUFFER_DIVIDED
    memset(buf, 0xFF, POINT_READ_BUF);
    memset(event, 0, sizeof(struct ts_event));

    buf[0] = 0x00;
    ret = fts_i2c_read(data->client, buf, 1, buf, 3);
    if (ret < 0) {
        FTS_ERROR("%s read touchdata failed.", __func__);
        return ret;
    }
    event->touch_point = 0;
    event->point_num = buf[FTS_TOUCH_POINT_NUM] & 0x0F;
    if (event->point_num > FTS_MAX_POINTS)
        event->point_num = FTS_MAX_POINTS;
    buf[3] = 0x03;
    if (event->point_num > 0) {
        fts_i2c_read(data->client, buf + 3, 1, buf + 3,
                     event->point_num * FTS_ONE_TCH_LEN);
    }
#else
    ret = fts_i2c_read(data->client, buf, 1, buf, POINT_READ_BUF);
    if (ret < 0) {
        FTS_ERROR("[B]Read touchdata failed, ret: %d", ret);
        return ret;
    }
    memset(event, 0, sizeof(struct ts_event));
    event->point_num = buf[FTS_TOUCH_POINT_NUM] & 0x0F;
    if (event->point_num > FTS_MAX_POINTS)
        event->point_num = FTS_MAX_POINTS;
    event->touch_point = 0;
#endif

#if (FTS_DEBUG_EN && (FTS_DEBUG_LEVEL == 2))
    {
        int len = event->point_num * FTS_ONE_TCH_LEN;
        int count = 0;
        memset(g_sz_debug, 0, 1024);
        if (len > (POINT_READ_BUF - 3)) {
            len = POINT_READ_BUF - 3;
        }
        else if (len == 0) {
            len += FTS_ONE_TCH_LEN;
        }
        count += sprintf(g_sz_debug, "%02X,%02X,%02X", buf[0], buf[1], buf[2]);
        for (i = 0; i < len; i++) {
            count += sprintf(g_sz_debug + count, ",%02X", buf[i + 3]);
        }
        FTS_DEBUG("buffer: %s", g_sz_debug);
    }
#endif

    for (i = 0; i < FTS_MAX_POINTS; i++) {
        pointid = (buf[FTS_TOUCH_ID_POS + FTS_ONE_TCH_LEN * i]) >> 4;
        if (pointid >= FTS_MAX_ID)
            break;
        else
            event->touch_point++;

        event->au16_x[i] =
            (s16) (buf[FTS_TOUCH_X_H_POS + FTS_ONE_TCH_LEN * i] & 0x0F) <<
            8 | (s16) buf[FTS_TOUCH_X_L_POS + FTS_ONE_TCH_LEN * i];
        event->au16_y[i] =
            (s16) (buf[FTS_TOUCH_Y_H_POS + FTS_ONE_TCH_LEN * i] & 0x0F) <<
            8 | (s16) buf[FTS_TOUCH_Y_L_POS + FTS_ONE_TCH_LEN * i];
        event->au8_touch_event[i] =
            buf[FTS_TOUCH_EVENT_POS + FTS_ONE_TCH_LEN * i] >> 6;
        event->au8_finger_id[i] =
            (buf[FTS_TOUCH_ID_POS + FTS_ONE_TCH_LEN * i]) >> 4;
        event->area[i] = (buf[FTS_TOUCH_AREA_POS + FTS_ONE_TCH_LEN * i]) >> 4;
        event->pressure[i] = (s16) buf[FTS_TOUCH_PRE_POS + FTS_ONE_TCH_LEN * i];

        if (0 == event->area[i])
            event->area[i] = 0x09;

        if (0 == event->pressure[i])
            event->pressure[i] = 0x3f;

        if ((event->au8_touch_event[i] == 0 || event->au8_touch_event[i] == 2)
            && (event->point_num == 0))
            break;
    }
    return 0;
}

/*****************************************************************************
*  Name: fts_report_value
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void fts_report_value(struct fts_ts_data *data)
{
    struct ts_event *event = &data->event;
    int i;
    int uppoint = 0;
    int touchs = 0;
    int pressure;

    //FTS_DEBUG("point number: %d, touch point: %d", event->point_num,
              //event->touch_point);
//<ASUS-BSP Robert_He 20170310> add touch test node ++++++
    if (FOCAL_TOUCH_DISABLE)
		return;
//<ASUS-BSP Robert_He 20170310> add touch test node ------

    if (data->pdata->have_key) {
        if ((1 == event->touch_point || 1 == event->point_num) &&
            (event->au16_y[0] == data->pdata->key_y_coord)) {

            if (event->point_num == 0) {
                FTS_DEBUG("Keys All Up!");
                for (i = 0; i < data->pdata->key_number; i++) {
                    input_report_key(data->input_dev, data->pdata->keys[i], 0);
                }
            }
            else {
                for (i = 0; i < data->pdata->key_number; i++) {
                    if (event->au16_x[0] >
                        (data->pdata->key_x_coords[i] - FTS_KEY_WIDTH)
                        && event->au16_x[0] <
                        (data->pdata->key_x_coords[i] + FTS_KEY_WIDTH)) {

                        if (event->au8_touch_event[i] == 0 ||
                            event->au8_touch_event[i] == 2) {
                            input_report_key(data->input_dev,
                                             data->pdata->keys[i], 1);
                            FTS_DEBUG("Key%d(%d, %d) DOWN!", i,
                                      event->au16_x[0], event->au16_y[0]);
                        }
                        else {
                            input_report_key(data->input_dev,
                                             data->pdata->keys[i], 0);
                            FTS_DEBUG("Key%d(%d, %d) Up!", i, event->au16_x[0],
                                      event->au16_y[0]);
                        }
                        break;
                    }
                }
            }
            input_sync(data->input_dev);
            return;
        }
    }
#if FTS_MT_PROTOCOL_B_EN
    for (i = 0; i < event->touch_point; i++) {
        input_mt_slot(data->input_dev, event->au8_finger_id[i]);

        if (event->au8_touch_event[i] == FTS_TOUCH_DOWN
            || event->au8_touch_event[i] == FTS_TOUCH_CONTACT) {
            input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, true);

            if (FTS_REPORT_PRESSURE_EN) {
                if (FTS_FORCE_TOUCH_EN) {
                    if (event->pressure[i] > 0) {
                        pressure = event->pressure[i];
                    }
                    else {
                        FTS_ERROR("[B]Illegal pressure: %d",
                                  event->pressure[i]);
                        pressure = 1;
                    }
                }
                else {
                    pressure = 0x3f;
                }

                input_report_abs(data->input_dev, ABS_MT_PRESSURE, pressure);
            }

            if (event->area[i] > 0) {
                input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->area[i]);  //0x05
            }
            else {
                FTS_ERROR("[B]Illegal touch-major: %d", event->area[i]);
                input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 1);   //0x05
            }
            //<ASUS-BSP Robert_He 20170405> change oriention for fw has changed ++++++

            input_report_abs(data->input_dev, ABS_MT_POSITION_X,
                             event->au16_x[i]);           //change for touch oriention
            input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
                             event->au16_y[i]);           //change for touch oriention
            //<ASUS-BSP Robert_He 20170405> change oriention for fw has changed ------
            touchs |= BIT(event->au8_finger_id[i]);
            data->touchs |= BIT(event->au8_finger_id[i]);
/*
            if (FTS_REPORT_PRESSURE_EN) {
                FTS_DEBUG("[B]P%d(%d, %d)[p:%d,tm:%d] DOWN!",
                          event->au8_finger_id[i], event->au16_x[i],
                          event->au16_y[i], pressure, event->area[i]);
            }
            else {
                FTS_DEBUG("[B]P%d(%d, %d)[tm:%d] DOWN!",
                          event->au8_finger_id[i], event->au16_x[i],
                          event->au16_y[i], event->area[i]);
            }
*/
        }
        else {
            uppoint++;
            input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
            data->touchs &= ~BIT(event->au8_finger_id[i]);
            //FTS_DEBUG("[B]P%d UP!", event->au8_finger_id[i]);
        }
    }

    if (unlikely(data->touchs ^ touchs)) {
        for (i = 0; i < data->pdata->max_touch_number; i++) {
            if (BIT(i) & (data->touchs ^ touchs)) {
                //FTS_DEBUG("[B]P%d UP!", i);
                input_mt_slot(data->input_dev, i);
                input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER,
                                           false);
            }
        }
    }
#else
    for (i = 0; i < event->touch_point; i++) {

        if (event->au8_touch_event[i] == FTS_TOUCH_DOWN
            || event->au8_touch_event[i] == FTS_TOUCH_CONTACT) {
            input_report_key(data->input_dev, BTN_TOUCH, 1);
            input_report_abs(data->input_dev, ABS_MT_TRACKING_ID,
                             event->au8_finger_id[i]);
            if (FTS_REPORT_PRESSURE_EN) {
                if (FTS_FORCE_TOUCH_EN) {
                    if (event->pressure[i] > 0) {
                        pressure = event->pressure[i];
                    }
                    else {
                        FTS_ERROR("[A]Illegal pressure: %d",
                                  event->pressure[i]);
                        pressure = 1;
                    }
                }
                else {
                    pressure = 0x3f;
                }

                input_report_abs(data->input_dev, ABS_MT_PRESSURE, pressure);
            }

            if (event->area[i] > 0) {
                input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->area[i]);  //0x05
            }
            else {
                FTS_ERROR("[A]Illegal touch-major: %d", event->area[i]);
                input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 1);   //0x05
            }
            input_report_abs(data->input_dev, ABS_MT_POSITION_X,
                             event->au16_x[i]);
            input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
                             event->au16_y[i]);

            input_mt_sync(data->input_dev);

            if (FTS_REPORT_PRESSURE_EN) {
                FTS_DEBUG("[A]P%d(%d, %d)[p:%d,tm:%d] DOWN!",
                          event->au8_finger_id[i], event->au16_x[i],
                          event->au16_y[i], pressure, event->area[i]);
            }
            else {
                FTS_DEBUG("[A]P%d(%d, %d)[tm:%d] DOWN!",
                          event->au8_finger_id[i], event->au16_x[i],
                          event->au16_y[i], event->area[i]);
            }
        }
        else {
            uppoint++;
        }
    }
#endif

    data->touchs = touchs;
    if (event->touch_point == uppoint) {
        //FTS_DEBUG("Points All Up!");
        input_report_key(data->input_dev, BTN_TOUCH, 0);
    }
    else {
        input_report_key(data->input_dev, BTN_TOUCH, event->touch_point > 0);
    }

    input_sync(data->input_dev);
}

/*****************************************************************************
*  Name: fts_touch_irq_work
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void fts_touch_irq_work(struct work_struct *work)
{
    int ret = -1;

#if FTS_ESDCHECK_EN
    fts_esdcheck_set_intr(1);
#endif
    ret = fts_read_touchdata(fts_wq_data);
    if (ret == 0) {
        fts_report_value(fts_wq_data);
    }

    enable_irq(fts_wq_data->client->irq);
#if FTS_ESDCHECK_EN
    fts_esdcheck_set_intr(0);
#endif
}

/*****************************************************************************
*  Name: fts_ts_interrupt
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static irqreturn_t fts_ts_interrupt(int irq, void *dev_id)
{
    struct fts_ts_data *fts_ts = dev_id;

    if (!fts_ts) {
        FTS_ERROR("[INTR]: Invalid fts_ts");
        return IRQ_HANDLED;
    }
    disable_irq_nosync(fts_ts->client->irq);

    queue_work(fts_ts->ts_workqueue, &fts_ts->touch_event_work);

    return IRQ_HANDLED;
}

/*****************************************************************************
*  Name: fts_gpio_configure
*  Brief: Configure IRQ&RESET GPIO
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_gpio_configure(struct fts_ts_data *data)
{
    int err = 0;

    FTS_FUNC_ENTER();
    /* request irq gpio */
    if (gpio_is_valid(data->pdata->irq_gpio)) {
        err = gpio_request(data->pdata->irq_gpio, "fts_irq_gpio");
        if (err) {
            FTS_ERROR("[GPIO]irq gpio request failed");
            goto err_irq_gpio_req;
        }

        err = gpio_direction_input(data->pdata->irq_gpio);
        if (err) {
            FTS_ERROR("[GPIO]set_direction for irq gpio failed");
            goto err_irq_gpio_dir;
        }
    }
    /* request reset gpio */
    if (gpio_is_valid(data->pdata->reset_gpio)) {
        err = gpio_request(data->pdata->reset_gpio, "fts_reset_gpio");
        if (err) {
            FTS_ERROR("[GPIO]reset gpio request failed");
            goto err_irq_gpio_dir;
        }

        err = gpio_direction_output(data->pdata->reset_gpio, 1);
        if (err) {
            FTS_ERROR("[GPIO]set_direction for reset gpio failed");
            goto err_reset_gpio_dir;
        }
    }

    FTS_FUNC_EXIT();
    return 0;

err_reset_gpio_dir:
    if (gpio_is_valid(data->pdata->reset_gpio))
        gpio_free(data->pdata->reset_gpio);
err_irq_gpio_dir:
    if (gpio_is_valid(data->pdata->irq_gpio))
        gpio_free(data->pdata->irq_gpio);
err_irq_gpio_req:
    FTS_FUNC_EXIT();
    return err;
}

/*****************************************************************************
*  Name: fts_get_dt_coords
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_get_dt_coords(struct device *dev, char *name,
                             struct fts_ts_platform_data *pdata)
{
    u32 coords[FTS_COORDS_ARR_SIZE];
    struct property *prop;
    struct device_node *np = dev->of_node;
    int coords_size, rc;

    prop = of_find_property(np, name, NULL);
    if (!prop)
        return -EINVAL;
    if (!prop->value)
        return -ENODATA;

    coords_size = prop->length / sizeof(u32);
    if (coords_size != FTS_COORDS_ARR_SIZE) {
        FTS_ERROR("invalid %s", name);
        return -EINVAL;
    }

    rc = of_property_read_u32_array(np, name, coords, coords_size);
    if (rc && (rc != -EINVAL)) {
        FTS_ERROR("Unable to read %s", name);
        return rc;
    }

    if (!strcmp(name, "focaltech,display-coords")) {
        pdata->x_min = coords[0];
        pdata->y_min = coords[1];
        pdata->x_max = coords[2];
        pdata->y_max = coords[3];
    }
    else {
        FTS_ERROR("unsupported property %s", name);
        return -EINVAL;
    }

    return 0;
}

/*****************************************************************************
*  Name: fts_parse_dt
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_parse_dt(struct device *dev, struct fts_ts_platform_data *pdata)
{
    int rc;
    struct device_node *np = dev->of_node;
    u32 temp_val;

    FTS_FUNC_ENTER();

    rc = fts_get_dt_coords(dev, "focaltech,display-coords", pdata);
    if (rc)
        FTS_ERROR("Unable to get display-coords");

    /* key */
    pdata->have_key = of_property_read_bool(np, "focaltech,have-key");
    if (pdata->have_key) {
        rc = of_property_read_u32(np, "focaltech,key-number",
                                  &pdata->key_number);
        if (rc) {
            FTS_ERROR("Key number undefined!");
        }
        rc = of_property_read_u32_array(np, "focaltech,keys",
                                        pdata->keys, pdata->key_number);
        if (rc) {
            FTS_ERROR("Keys undefined!");
        }
        rc = of_property_read_u32(np, "focaltech,key-y-coord",
                                  &pdata->key_y_coord);
        if (rc) {
            FTS_ERROR("Key Y Coord undefined!");
        }
        rc = of_property_read_u32_array(np, "focaltech,key-x-coords",
                                        pdata->key_x_coords, pdata->key_number);
        if (rc) {
            FTS_ERROR("Key X Coords undefined!");
        }
        FTS_DEBUG("%d: (%d, %d, %d), [%d, %d, %d][%d]",
                  pdata->key_number, pdata->keys[0], pdata->keys[1],
                  pdata->keys[2], pdata->key_x_coords[0],
                  pdata->key_x_coords[1], pdata->key_x_coords[2],
                  pdata->key_y_coord);
    }

    /* reset, irq gpio info */
    pdata->reset_gpio =
        of_get_named_gpio_flags(np, "focaltech,reset-gpio", 0,
                                &pdata->reset_gpio_flags);
    if (pdata->reset_gpio < 0) {
        FTS_ERROR("Unable to get reset_gpio");
    }

    pdata->irq_gpio =
        of_get_named_gpio_flags(np, "focaltech,irq-gpio", 0,
                                &pdata->irq_gpio_flags);
    if (pdata->irq_gpio < 0) {
        FTS_ERROR("Unable to get irq_gpio");
    }

    rc = of_property_read_u32(np, "focaltech,max-touch-number", &temp_val);
    if (!rc)
        pdata->max_touch_number = temp_val;
    else
        FTS_ERROR("Unable to get max-touch-number");

    FTS_FUNC_EXIT();
    return 0;
}

#if defined(CONFIG_FB)
/*****************************************************************************
*  Name: fb_notifier_callback
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fb_notifier_callback(struct notifier_block *self,
                                unsigned long event, void *data)
{
    struct fb_event *evdata = data;
    int *blank;
    struct fts_ts_data *fts_data =
        container_of(self, struct fts_ts_data, fb_notif);

    if (evdata && evdata->data && event == FB_EVENT_BLANK &&
        fts_data && fts_data->client) {
        blank = evdata->data;
        if (*blank == FB_BLANK_UNBLANK)
            fts_ts_resume(&fts_data->client->dev);
        else if (*blank == FB_BLANK_POWERDOWN)
            fts_ts_suspend(&fts_data->client->dev);
    }

    return 0;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
/*****************************************************************************
*  Name: fts_ts_early_suspend
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void fts_ts_early_suspend(struct early_suspend *handler)
{
    struct fts_ts_data *data = container_of(handler,
                                            struct fts_ts_data,
                                            early_suspend);

    fts_ts_suspend(&data->client->dev);
}

/*****************************************************************************
*  Name: fts_ts_late_resume
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void fts_ts_late_resume(struct early_suspend *handler)
{
    struct fts_ts_data *data = container_of(handler,
                                            struct fts_ts_data,
                                            early_suspend);

    fts_ts_resume(&data->client->dev);
}
#endif
//<ASUS-BSP Robert_He 20170328> add switch touch firmware node ++++++
extern u8 fts_cap_show_fw_version(void);
//<ASUS-BSP Robert_He 20170328> add switch touch firmware node ------

//<ASUS-BSP Robert_He 20170328> add switch touch firmware node ++++++
static ssize_t touch_switch_name(struct switch_dev *sdev,char *buf)
{

    u8 reg_addr;
	u8 fw_ver;
	u8 vendor_id;
	u8 cap_ver;
    int err;

    reg_addr = FTS_REG_FW_VER;
    err = fts_i2c_read(fts_i2c_client, &reg_addr, 1, &fw_ver, 1);
    if (err < 0)
        FTS_ERROR("fw version read failed");

	reg_addr = FTS_REG_VENDOR_ID;
	err = fts_i2c_read(fts_i2c_client, &reg_addr, 1, &vendor_id, 1);
	if (err < 0)
			FTS_ERROR("vendor id read failed");

	cap_ver = fts_cap_show_fw_version();
	return snprintf(buf,PAGE_SIZE,"%02x_%02x_%02x\n",
             vendor_id,fw_ver,cap_ver);
}
//<ASUS-BSP Robert_He 20170328> add switch touch firmware node ++++++


/*****************************************************************************
*  Name: fts_ts_probe
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_ts_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
    struct fts_ts_platform_data *pdata;
    struct fts_ts_data *data;
    struct input_dev *input_dev;
    int err, len;
//<ASUS-BSP Robert_He 20170310> add touch test node ++++++
	int attr_count=0;
//<ASUS-BSP Robert_He 20170310> add touch test node ------


    FTS_FUNC_ENTER();
	printk("[Focal]here is touch probe");
	//return 0;
    /* 1. Get Platform data */
    if (client->dev.of_node) {
        pdata = devm_kzalloc(&client->dev,
                             sizeof(struct fts_ts_platform_data), GFP_KERNEL);
        if (!pdata) {
            FTS_ERROR("[MEMORY]Failed to allocate memory");
            FTS_FUNC_EXIT();
            return -ENOMEM;
        }
        err = fts_parse_dt(&client->dev, pdata);
        if (err) {
            FTS_ERROR("[DTS]DT parsing failed");
        }
    }
    else {
        pdata = client->dev.platform_data;
    }

    if (!pdata) {
        FTS_ERROR("Invalid pdata");
        FTS_FUNC_EXIT();
        return -EINVAL;
    }

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        FTS_ERROR("I2C not supported");
        FTS_FUNC_EXIT();
        return -ENODEV;
    }

    data = devm_kzalloc(&client->dev, sizeof(struct fts_ts_data), GFP_KERNEL);
    if (!data) {
        FTS_ERROR("[MEMORY]Failed to allocate memory");
        FTS_FUNC_EXIT();
        return -ENOMEM;
    }

    input_dev = input_allocate_device();
    if (!input_dev) {
        FTS_ERROR("[INPUT]Failed to allocate input device");
        FTS_FUNC_EXIT();
        return -ENOMEM;
    }

    data->input_dev = input_dev;
    data->client = client;
    data->pdata = pdata;

    fts_wq_data = data;
    fts_i2c_client = client;
    fts_input_dev = input_dev;
    /* Init and register Input device */
    input_dev->name = FTS_DRIVER_NAME;
    input_dev->id.bustype = BUS_I2C;
    input_dev->dev.parent = &client->dev;

    input_set_drvdata(input_dev, data);
    i2c_set_clientdata(client, data);

    __set_bit(EV_KEY, input_dev->evbit);
    if (data->pdata->have_key) {
        FTS_DEBUG("set key capabilities");
        for (len = 0; len < data->pdata->key_number; len++) {
            input_set_capability(input_dev, EV_KEY, data->pdata->keys[len]);
        }
    }
    __set_bit(EV_ABS, input_dev->evbit);
    __set_bit(BTN_TOUCH, input_dev->keybit);
    __set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

#if FTS_MT_PROTOCOL_B_EN
    input_mt_init_slots(input_dev, pdata->max_touch_number, INPUT_MT_DIRECT);
#else
    input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 0x0f, 0, 0);
#endif
    input_set_abs_params(input_dev, ABS_MT_POSITION_X, pdata->x_min,
                         pdata->x_max, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_POSITION_Y, pdata->y_min,
                         pdata->y_max, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 0xFF, 0, 0);
	 if (FTS_REPORT_PRESSURE_EN) {
    input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 0xFF, 0, 0);
	 	}

    err = input_register_device(input_dev);
    if (err) {
        FTS_ERROR("Input device registration failed");
        goto free_inputdev;
    }

#if FTS_POWER_SOURCE_CUST_EN
    fts_power_source_init(data);
    fts_power_source_ctrl(data, 1);
#endif

    err = fts_gpio_configure(data);
    if (err < 0) {
        FTS_ERROR("[GPIO]Failed to configure the gpios");
        goto free_gpio;
    }

    fts_reset_proc(200);
    fts_wait_tp_to_valid(client);

    INIT_WORK(&data->touch_event_work, fts_touch_irq_work);
    data->ts_workqueue = create_workqueue(FTS_WORKQUEUE_NAME);
    if (!data->ts_workqueue) {
        err = -ESRCH;
        FTS_ERROR("Create touch workqueue failed");
        goto exit_create_singlethread;
    }

    fts_ctpm_get_upgrade_array();

    err = request_threaded_irq(client->irq, NULL, fts_ts_interrupt,
                               pdata->
                               irq_gpio_flags | IRQF_ONESHOT |
                               IRQF_TRIGGER_FALLING, client->dev.driver->name,
                               data);
    if (err) {
        FTS_ERROR("Request irq failed!");
        goto free_gpio;
    }
//<ASUS-BSP Robert_He 20170310> add touch test node ++++++
	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		err = sysfs_create_file(&data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
		
		if (err < 0) {
			printk("[Touch]sysfile %d test %d\n",attr_count,err);
			goto err_sysfs;
		}
	}
//<ASUS-BSP Robert_He 20170310> add touch test node -----

    disable_irq(client->irq);

#if FTS_PSENSOR_EN
    if (fts_sensor_init(data) != 0) {
        FTS_ERROR("fts_sensor_init failed!");
        FTS_FUNC_EXIT();
        return 0;
    }
#endif

#if FTS_APK_NODE_EN
    fts_create_apk_debug_channel(client);
#endif

#if FTS_SYSFS_NODE_EN
    fts_create_sysfs(client);
#endif

    fts_ex_mode_init(client);

#if FTS_GESTURE_EN
    fts_gesture_init(input_dev, client);
#endif

#if FTS_ESDCHECK_EN
    fts_esdcheck_init();
#endif

#if FTS_AUTO_UPGRADE_EN
    fts_ctpm_upgrade_init();
#endif

    fts_update_fw_ver(data);
    fts_update_fw_vendor_id(data);

    FTS_INFO("Firmware version = 0x%02x.%d.%d, fw_vendor_id=0x%02x",
             data->fw_ver[0], data->fw_ver[1], data->fw_ver[2],
             data->fw_vendor_id);

//<ASUS-BSP Robert_He 20170310> add switch touch firmware node ++++++
	asus_vendor_id_touch = data->fw_vendor_id;
	asus_firmware_id_touch = data->fw_ver[0];
	data->touch_sdev.name="touch";
	data->touch_sdev.print_name = touch_switch_name;
	if (switch_dev_register(&data->touch_sdev)<0)
	{
		printk("[Touch]:switch_dev_register failed!\n");
	}
//<ASUS-BSP Robert_He 20170310> add switch touch firmware node ------


#if FTS_TEST_EN
    fts_test_init(client);
#endif

#if defined(CONFIG_FB)
    data->fb_notif.notifier_call = fb_notifier_callback;
    err = fb_register_client(&data->fb_notif);
    if (err)
        FTS_ERROR("[FB]Unable to register fb_notifier: %d", err);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    data->early_suspend.level =
        EARLY_SUSPEND_LEVEL_BLANK_SCREEN + FTS_SUSPEND_LEVEL;
    data->early_suspend.suspend = fts_ts_early_suspend;
    data->early_suspend.resume = fts_ts_late_resume;
    register_early_suspend(&data->early_suspend);
#endif

    enable_irq(client->irq);

    FTS_FUNC_EXIT();
    return 0;
//<ASUS-BSP Robert_He 20170310> add switch touch firmware node ++++++	
err_sysfs:
		for (attr_count--; attr_count >= 0; attr_count--) {
			sysfs_remove_file(&data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
		}
//<ASUS-BSP Robert_He 20170310> add switch touch firmware node ------

exit_create_singlethread:
    FTS_ERROR("==singlethread error =");
    i2c_set_clientdata(client, NULL);

free_gpio:
    if (gpio_is_valid(pdata->reset_gpio))
        gpio_free(pdata->reset_gpio);
    if (gpio_is_valid(pdata->irq_gpio))
        gpio_free(pdata->irq_gpio);
free_inputdev:
    input_free_device(input_dev);
    FTS_FUNC_EXIT();
    return err;
}

/*****************************************************************************
*  Name: fts_ts_remove
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_ts_remove(struct i2c_client *client)
{
    struct fts_ts_data *data = i2c_get_clientdata(client);

    FTS_FUNC_ENTER();
    cancel_work_sync(&data->touch_event_work);
    destroy_workqueue(data->ts_workqueue);

#if FTS_PSENSOR_EN
    fts_sensor_remove(data);
#endif

#if FTS_APK_NODE_EN
    fts_release_apk_debug_channel();
#endif

#if FTS_SYSFS_NODE_EN
    fts_remove_sysfs(fts_i2c_client);
#endif

    fts_ex_mode_exit(client);

#if FTS_AUTO_UPGRADE_EN
    cancel_work_sync(&fw_update_work);
#endif

#if defined(CONFIG_FB)
    if (fb_unregister_client(&data->fb_notif))
        FTS_ERROR("Error occurred while unregistering fb_notifier.");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    unregister_early_suspend(&data->early_suspend);
#endif
    free_irq(client->irq, data);

    if (gpio_is_valid(data->pdata->reset_gpio))
        gpio_free(data->pdata->reset_gpio);

    if (gpio_is_valid(data->pdata->irq_gpio))
        gpio_free(data->pdata->irq_gpio);

    input_unregister_device(data->input_dev);

#if FTS_TEST_EN
    fts_test_exit(client);
#endif

#if FTS_ESDCHECK_EN
    fts_esdcheck_exit();
#endif

    FTS_FUNC_EXIT();
    return 0;
}

/*****************************************************************************
*  Name: fts_ts_suspend
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_ts_suspend(struct device *dev)
{
    struct fts_ts_data *data = dev_get_drvdata(dev);
    int retval = 0;

    FTS_FUNC_ENTER();
    if (data->suspended) {
        FTS_INFO("Already in suspend state");
        FTS_FUNC_EXIT();
        return -1;
    }

    fts_release_all_finger();

#if FTS_GESTURE_EN
    retval = fts_gesture_suspend(data->client);
    if (retval == 0) {
        /* Enter into gesture mode(suspend) */
        retval = enable_irq_wake(fts_wq_data->client->irq);
        if (retval)
            FTS_ERROR("%s: set_irq_wake failed", __func__);
        data->suspended = true;
        FTS_FUNC_EXIT();
        return 0;
    }
#endif

#if FTS_PSENSOR_EN
    if (fts_sensor_suspend(data) != 0) {
        enable_irq_wake(data->client->irq);
        data->suspended = true;
        return 0;
    }
#endif

#if FTS_ESDCHECK_EN
    fts_esdcheck_suspend();
#endif

    disable_irq_nosync(data->client->irq);
    /* TP enter sleep mode */
    retval =
        fts_i2c_write_reg(data->client, FTS_REG_POWER_MODE,
                          FTS_REG_POWER_MODE_SLEEP_VALUE);
    if (retval < 0) {
        FTS_ERROR("Set TP to sleep mode fail, ret=%d!", retval);
    }
    data->suspended = true;

    FTS_FUNC_EXIT();

    return 0;
}

/*****************************************************************************
*  Name: fts_ts_resume
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_ts_resume(struct device *dev)
{
    struct fts_ts_data *data = dev_get_drvdata(dev);
    int err;
    char val;

    FTS_FUNC_ENTER();
    if (!data->suspended) {
        FTS_DEBUG("Already in awake state");
        FTS_FUNC_EXIT();
        return -1;
    }
//<BSP_Robert_he><20170421>realease all finger to avoid ghost touch +++	
	fts_release_all_finger();
//<BSP_Robert_he><20170421>realease all finger to avoid ghost touch ---

#if FTS_GESTURE_EN
    if (fts_gesture_resume(data->client) == 0) {
        err = disable_irq_wake(data->client->irq);
        if (err)
            FTS_ERROR("%s: disable_irq_wake failed", __func__);
        data->suspended = false;
        FTS_FUNC_EXIT();
        return 0;
    }
#endif

#if (!FTS_CHIP_IDC)
    fts_reset_proc(200);
#endif

    fts_tp_state_recovery(data->client);

#if FTS_PSENSOR_EN
    if (fts_sensor_resume(data) != 0) {
        disable_irq_wake(data->client->irq);
        data->suspended = false;
        FTS_FUNC_EXIT();
        return 0;
    }
#endif

    err = fts_i2c_read_reg(fts_i2c_client, 0xA6, &val);
    FTS_DEBUG("read 0xA6: %02X, ret: %d", val, err);

    data->suspended = false;

    enable_irq(data->client->irq);

#if FTS_ESDCHECK_EN
    fts_esdcheck_resume();
#endif
    FTS_FUNC_EXIT();
    return 0;
}
//<BSP_Robert_he><20170310>add shutdown function +++
/****************************************************************************
name:shutdown
*****************************************************************************/
static void fts_ts_shutdown(struct i2c_client *client)
{
	struct fts_ts_data *data = i2c_get_clientdata(client);
	FTS_FUNC_ENTER();
	gpio_direction_output(data->pdata->reset_gpio, 0);
	msleep(1);
	fts_power_source_ctrl(data, 0);
	FTS_FUNC_ENTER();
}
//<BSP_Robert_he><20170310>add shutdown function ---


/*****************************************************************************
* I2C Driver
*****************************************************************************/
static const struct i2c_device_id fts_ts_id[] = {
    {FTS_DRIVER_NAME, 0},
    {},
};

MODULE_DEVICE_TABLE(i2c, fts_ts_id);

static struct of_device_id fts_match_table[] = {
    {.compatible = "focaltech,ftstouch",},
    {},
};

static struct i2c_driver fts_ts_driver = {
    .probe = fts_ts_probe,
    .remove = fts_ts_remove,
    //<BSP_Robert_he><20170310>add shutdown function +++
    .shutdown = fts_ts_shutdown,
    //<BSP_Robert_he><20170310>add shutdown function ---
    .driver = {
               .name = FTS_DRIVER_NAME,
               .owner = THIS_MODULE,
               .of_match_table = fts_match_table,
               },
    .id_table = fts_ts_id,
};

/*****************************************************************************
*  Name: fts_ts_init
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int __init fts_ts_init(void)
{
    int ret = 0;
    FTS_FUNC_ENTER();
	if(asus_lcd_id == 1)
		return 0;
    ret = i2c_add_driver(&fts_ts_driver);
    if (ret != 0) {
        FTS_ERROR("Focaltech touch screen driver init failed!");
    }
    FTS_FUNC_EXIT();
    return ret;
}

/*****************************************************************************
*  Name: fts_ts_exit
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void __exit fts_ts_exit(void)
{
    i2c_del_driver(&fts_ts_driver);
}

module_init(fts_ts_init);
module_exit(fts_ts_exit);

MODULE_AUTHOR("FocalTech Driver Team");
MODULE_DESCRIPTION("FocalTech Touchscreen Driver");
MODULE_LICENSE("GPL v2");
