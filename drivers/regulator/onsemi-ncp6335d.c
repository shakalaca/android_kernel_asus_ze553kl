/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/log2.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/debugfs.h>
#include <linux/regulator/onsemi-ncp6335d.h>
#include <linux/string.h>

/* registers */
#define REG_NCP6335D_PID		0x03
#define REG_NCP6335D_PROGVSEL1		0x10
#define REG_NCP6335D_PROGVSEL0		0x11
#define REG_NCP6335D_PGOOD		0x12
#define REG_NCP6335D_TIMING		0x13
#define REG_NCP6335D_COMMAND		0x14
#define REG_NCP6335D_LIMIT      0x16

/* constraints */
#define NCP6335D_MIN_VOLTAGE_UV		600000
#define NCP6335D_STEP_VOLTAGE_UV	6250
#define NCP6335D_VOLTAGE_STEPS		128
#define NCP6335D_MIN_SLEW_NS		365
#define NCP6335D_MAX_SLEW_NS		2920

/* bits */
#define NCP6335D_ENABLE			BIT(7)
#define NCP6335D_DVS_PWM_MODE		BIT(5)
#define NCP6335D_PWM_MODE1		BIT(6)
#define NCP6335D_PWM_MODE0		BIT(7)
#define NCP6335D_PGOOD_DISCHG		BIT(4)
#define NCP6335D_SLEEP_MODE		BIT(4)

#define NCP6335D_VOUT_SEL_MASK		0x7F
#define NCP6335D_SLEW_MASK		0x18
#define NCP6335D_SLEW_SHIFT		0x3

struct ncp6335d_info {
	struct regulator_dev *regulator;
	struct regulator_init_data *init_data;
	struct regmap *regmap;
	struct device *dev;
	unsigned int vsel_reg;
	unsigned int vsel_backup_reg;
	unsigned int mode_bit;
	int curr_voltage;
	int slew_rate;

	unsigned int step_size;
	unsigned int min_voltage;
	unsigned int min_slew_ns;
	unsigned int max_slew_ns;
	unsigned int peek_poke_address;

	//ASUS_BSP +++ Zhengwei_Cai "porting buck convertor for RK"
	unsigned int vsel_gpio;
	unsigned int en_gpio;
	struct regulator *vreg;//for i2c
	int    client_count;
	int	   is_voltage_configured;
	//ASUS_BSP --- Zhengwei_Cai "porting buck convertor for RK"
	struct dentry *debug_root;
};
static struct ncp6335d_info *the_ncp6335d_info = NULL;
static int delay_array[] = {10, 20, 30, 40, 50};

static int ncp6335x_read(struct ncp6335d_info *dd, unsigned int reg,
						unsigned int *val)
{
	int i = 0, rc = 0;

	rc = regmap_read(dd->regmap, reg, val);
	for (i = 0; rc && i < ARRAY_SIZE(delay_array); i++) {
		pr_debug("Failed reading reg=%u - retry(%d)\n", reg, i);
		msleep(delay_array[i]);
		rc = regmap_read(dd->regmap, reg, val);
	}

	if (rc)
		pr_err("Failed reading reg=%u rc=%d\n", reg, rc);

	return rc;
}

static int ncp6335x_write(struct ncp6335d_info *dd, unsigned int reg,
						unsigned int val)
{
	int i = 0, rc = 0;

	rc = regmap_write(dd->regmap, reg, val);
	for (i = 0; rc && i < ARRAY_SIZE(delay_array); i++) {
		pr_debug("Failed writing reg=%u - retry(%d)\n", reg, i);
		msleep(delay_array[i]);
		rc = regmap_write(dd->regmap, reg, val);
	}

	if (rc)
		pr_err("Failed writing reg=%u rc=%d\n", reg, rc);

	return rc;
}

static int ncp6335x_update_bits(struct ncp6335d_info *dd, unsigned int reg,
					unsigned int mask, unsigned int val)
{
	int i = 0, rc = 0;

	rc = regmap_update_bits(dd->regmap, reg, mask, val);
	for (i = 0; rc && i < ARRAY_SIZE(delay_array); i++) {
		pr_debug("Failed updating reg=%u- retry(%d)\n", reg, i);
		msleep(delay_array[i]);
		rc = regmap_update_bits(dd->regmap, reg, mask, val);
	}

	if (rc)
		pr_err("Failed updating reg=%u rc=%d\n", reg, rc);

	return rc;
}

static void dump_registers(struct ncp6335d_info *dd,
			unsigned int reg, const char *reg_str, const char * func)
{
	unsigned int val = 0;

	ncp6335x_read(dd, reg, &val);
	dev_info(dd->dev, "NCP6335D: Reg %s [0x%x] 0x%x %s()\n", reg_str,reg,val,func);
}

//ASUS_BSP +++ Zhengwei_Cai "porting buck convertor for RK"
typedef struct
{
	unsigned int reg_addr;
	unsigned int reg_val;
}reg_table_t;

static reg_table_t valid_reg_table[] =
{
	{0x00,0x00},//INT_ACK
	//{0x01,0x00},//INT_SEN
	{0x02,0xff},//INT_MASK
	//{0x03,0x15},//PID
	//{0x04,0x00},//RID
	//{0x05,0x00},//FID
	{0x10,0xD8},//PROGVSEL1
	{0x11,0xD0},//PROGVSEL0
	{0x12,0x10},//PGOOD
	{0x13,0x19},//TIME
	{0x14,0x81},//COMMAND
	{0x15,0x80},//MODULE
	{0x16,0x23}//LIMCONF
};

static int ncp6335d_set_reg_to_default(struct ncp6335d_info *dd)
{
	int i;
	unsigned int value;
	int rc;
	for(i=0;i<sizeof(valid_reg_table)/sizeof(reg_table_t);i++)
	{
		rc = ncp6335x_read(dd, valid_reg_table[i].reg_addr, &value);
		if(rc)
		{
			dev_err(dd->dev, "read reg[0x%x] failed! rc %d\n",valid_reg_table[i].reg_addr,rc);
			return -1;
		}
		if(value!=valid_reg_table[i].reg_val)
		{
			if(valid_reg_table[i].reg_addr == 0x11 && dd->is_voltage_configured)
			{
				//voltage configured by RK driver, not set to default value
				dev_info(dd->dev,"voltage val 0x%x configured by regulator, not set to default\n",value);
				continue;
			}
			//dev_info(dd->dev, "update reg[0x%x] value 0x%x --> 0x%x\n",valid_reg_table[i].reg_addr,value,valid_reg_table[i].reg_val);
			rc = ncp6335x_write(dd, valid_reg_table[i].reg_addr, valid_reg_table[i].reg_val);
			if(rc)
			{
				dev_err(dd->dev, "write reg[0x%x] failed! rc %d\n",valid_reg_table[i].reg_addr,rc);
				return -2;
			}
		}
	}
	return 0;
}
#if 0
static void dump_all_registers(struct ncp6335d_info *dd)
{
	int i;
	unsigned int val;
	unsigned int reg_table[]= {
								0x00,//INT_ACK
								0x01,//INT_SEN
								0x02,//INT_MASK
								0x03,//PID
								0x04,//RID
								0x05,//FID
								0x10,//PROGVSEL1
								0x11,//PROGVSEL0
								0x12,//PGOOD
								0x13,//TIME
								0x14,//COMMAND
								0x15,//MODULE
								0x16//LIMCONF
							  };
	for(i=0;i<sizeof(reg_table)/sizeof(unsigned int);i++)
	{
		ncp6335x_read(dd, reg_table[i], &val);
		dev_info(dd->dev, "NCP6335D: Reg[0x%x] 0x%x\n",reg_table[i],val);
	}
}
#endif
//ASUS_BSP --- Zhengwei_Cai "porting buck convertor for RK"
static void ncp633d_slew_delay(struct ncp6335d_info *dd,
					int prev_uV, int new_uV)
{
	u8 val;
	int delay;

	val = abs(prev_uV - new_uV) / dd->step_size;
	delay = ((val * dd->slew_rate) / 1000) + 1;

	dev_dbg(dd->dev, "Slew Delay = %d\n", delay);

	udelay(delay);
}
//ASUS_BSP +++ Zhengwei_Cai "porting buck convertor for RK"

static void ncp6335d_set_en_gpio(struct ncp6335d_info *dd,int enable)
{
	int val;
	gpio_set_value_cansleep(dd->en_gpio,enable?GPIOF_OUT_INIT_HIGH:GPIOF_OUT_INIT_LOW);
	val = gpio_get_value(dd->en_gpio);
	dev_info(dd->dev,"en gpio is %d now\n",val);
}

int ncp6335d_enable_output(struct ncp6335d_info *dd)
{
	if(dd)
	{
		dd->client_count++;
		if(dd->client_count == 1)
		{
			ncp6335d_set_reg_to_default(dd);
			ncp6335d_set_en_gpio(dd,1);
		}
		else
		{
			pr_info("ncp6335d client %d, ignore enable output\n",dd->client_count);
		}
	}
	else
	{
		pr_err("ncp6335d probe failed, not available!\n");
		return -1;
	}
	return 0;
}

int ncp6335d_disable_output(struct ncp6335d_info *dd)
{
	if(dd)
	{
		dd->client_count--;
		if(dd->client_count == 0)
		{
			ncp6335d_set_en_gpio(dd,0);
			dd->is_voltage_configured = 0;
		}
		else
		{
			pr_info("ncp6335d client %d, ignore disable output\n",dd->client_count);
			if(dd->client_count < 0)
			{
				dd->client_count = 0;
			}
		}
	}
	else
	{
		pr_err("ncp6335d probe failed, not available!\n");
		return -1;
	}
	return 0;
}
int ncp6335d_enable_power(void)
{
	return ncp6335d_enable_output(the_ncp6335d_info);
}
int ncp6335d_disable_power(void)
{
	return ncp6335d_disable_output(the_ncp6335d_info);
}
static int ncp6335d_is_enabled(struct regulator_dev * rdev)
{
	int rc;
	struct ncp6335d_info *dd = rdev_get_drvdata(rdev);
	rc = gpio_get_value(dd->en_gpio);
	//dev_info(dd->dev,"en gpio get is %d\n",rc);
	return rc;
}

//ASUS_BSP --- Zhengwei_Cai "porting buck convertor for RK"

static int ncp6335d_enable(struct regulator_dev *rdev)
{
#if 0
	int rc;
	struct ncp6335d_info *dd = rdev_get_drvdata(rdev);

	rc = ncp6335x_update_bits(dd, dd->vsel_reg,
			NCP6335D_ENABLE, NCP6335D_ENABLE);
	if (rc)
		dev_err(dd->dev, "Unable to enable regualtor rc(%d)", rc);

	dump_registers(dd, dd->vsel_reg, "PROGVSEL0", __func__);

	return rc;
#else
	struct ncp6335d_info *dd = rdev_get_drvdata(rdev);
	return ncp6335d_enable_output(dd);
#endif
}

static int ncp6335d_disable(struct regulator_dev *rdev)
{
	int rc;
#if 0
	struct ncp6335d_info *dd = rdev_get_drvdata(rdev);

	rc = ncp6335x_update_bits(dd, dd->vsel_reg,
					NCP6335D_ENABLE, 0);
	if (rc)
		dev_err(dd->dev, "Unable to disable regualtor rc(%d)", rc);

	dump_registers(dd, dd->vsel_reg, "PROGVSEL0", __func__);
#else
	struct ncp6335d_info *dd = rdev_get_drvdata(rdev);
	rc = ncp6335d_disable_output(dd);
#endif
	return rc;
}

static int ncp6335d_get_voltage(struct regulator_dev *rdev)
{
	unsigned int val;
	int rc;
	struct ncp6335d_info *dd = rdev_get_drvdata(rdev);

	rc = ncp6335x_read(dd, dd->vsel_reg, &val);
	if (rc) {
		dev_err(dd->dev, "Unable to get volatge rc(%d)", rc);
		return rc;
	}
	dd->curr_voltage = ((val & NCP6335D_VOUT_SEL_MASK) * dd->step_size) +
				dd->min_voltage;

	//dump_registers(dd, dd->vsel_reg, "PROGVSEL0", __func__);

	return dd->curr_voltage;
}

static int ncp6335d_set_voltage(struct regulator_dev *rdev,
			int min_uV, int max_uV, unsigned *selector)
{
	int rc, set_val, new_uV;
	struct ncp6335d_info *dd = rdev_get_drvdata(rdev);

	set_val = DIV_ROUND_UP(min_uV - dd->min_voltage, dd->step_size);
	new_uV = (set_val * dd->step_size) + dd->min_voltage;
	if (new_uV > max_uV) {
		dev_err(dd->dev, "Unable to set volatge (%d %d)\n",
							min_uV, max_uV);
		return -EINVAL;
	}

	rc = ncp6335x_update_bits(dd, dd->vsel_reg,
		NCP6335D_VOUT_SEL_MASK, (set_val & NCP6335D_VOUT_SEL_MASK));
	if (rc) {
		dev_err(dd->dev, "Unable to set volatge (%d %d)\n",
							min_uV, max_uV);
	} else {
		ncp633d_slew_delay(dd, dd->curr_voltage, new_uV);
		dd->curr_voltage = new_uV;
		dd->is_voltage_configured = 1;
		dev_info(dd->dev,"set voltage to %d uV from range[%d,%d], setval %d",
					dd->curr_voltage,
					min_uV,max_uV,
					set_val
				);
	}

	//dump_registers(dd, dd->vsel_reg, "PROGVSEL0", __func__);

	return rc;
}
int ncp6335d_set_voltage_for_preisp(int voltage)
{
	if(!the_ncp6335d_info)
	    return -1;
	return ncp6335d_set_voltage(the_ncp6335d_info->regulator,voltage,the_ncp6335d_info->init_data->constraints.max_uV,NULL);
}
static int ncp6335d_list_voltage(struct regulator_dev *rdev,
					unsigned selector)
{
	struct ncp6335d_info *dd = rdev_get_drvdata(rdev);

	if (selector >= NCP6335D_VOLTAGE_STEPS)
		return 0;

	return selector * dd->step_size + dd->min_voltage;
}

static int ncp6335d_set_mode(struct regulator_dev *rdev,
					unsigned int mode)
{
	int rc;
	struct ncp6335d_info *dd = rdev_get_drvdata(rdev);

	/* only FAST and NORMAL mode types are supported */
	if (mode != REGULATOR_MODE_FAST && mode != REGULATOR_MODE_NORMAL) {
		dev_err(dd->dev, "Mode %d not supported\n", mode);
		return -EINVAL;
	}

	rc = ncp6335x_update_bits(dd, REG_NCP6335D_COMMAND, dd->mode_bit,
			(mode == REGULATOR_MODE_FAST) ? dd->mode_bit : 0);
	if (rc) {
		dev_err(dd->dev, "Unable to set operating mode rc(%d)", rc);
		return rc;
	}

	rc = ncp6335x_update_bits(dd, REG_NCP6335D_COMMAND,
					NCP6335D_DVS_PWM_MODE,
					(mode == REGULATOR_MODE_FAST) ?
					NCP6335D_DVS_PWM_MODE : 0);
	if (rc)
		dev_err(dd->dev, "Unable to set DVS trans. mode rc(%d)", rc);

	dump_registers(dd, REG_NCP6335D_COMMAND, "COMMAND", __func__);

	return rc;
}

static unsigned int ncp6335d_get_mode(struct regulator_dev *rdev)
{
	unsigned int val;
	int rc;
	struct ncp6335d_info *dd = rdev_get_drvdata(rdev);

	rc = ncp6335x_read(dd, REG_NCP6335D_COMMAND, &val);
	if (rc) {
		dev_err(dd->dev, "Unable to get regulator mode rc(%d)\n", rc);
		return rc;
	}

	dump_registers(dd, REG_NCP6335D_COMMAND, "COMMAND", __func__);

	if (val & dd->mode_bit)
		return REGULATOR_MODE_FAST;

	return REGULATOR_MODE_NORMAL;
}

static struct regulator_ops ncp6335d_ops = {
	.set_voltage = ncp6335d_set_voltage,
	.get_voltage = ncp6335d_get_voltage,
	.list_voltage = ncp6335d_list_voltage,
	.enable = ncp6335d_enable,
	.disable = ncp6335d_disable,
	.set_mode = ncp6335d_set_mode,
	.get_mode = ncp6335d_get_mode,
	.is_enabled = ncp6335d_is_enabled //ASUS_BSP Zhengwei_Cai "porting buck convertor for RK"
};

static struct regulator_desc rdesc = {
	.name = "ncp6335d",
	.owner = THIS_MODULE,
	.n_voltages = NCP6335D_VOLTAGE_STEPS,
	.ops = &ncp6335d_ops,
};

static int ncp6335d_restore_working_reg(struct device_node *node,
					struct ncp6335d_info *dd)
{
	int ret;
	unsigned int val;

	/* Restore register from back up register */
	ret = ncp6335x_read(dd, dd->vsel_backup_reg, &val);
	if (ret < 0) {
		dev_err(dd->dev, "Failed to get backup data from reg %d, ret = %d\n",
			dd->vsel_backup_reg, ret);
		return ret;
	}

	ret = ncp6335x_update_bits(dd, dd->vsel_reg,
					NCP6335D_VOUT_SEL_MASK, val);
	if (ret < 0) {
		dev_err(dd->dev, "Failed to update working reg %d, ret = %d\n",
			dd->vsel_reg,  ret);
		return ret;
	}

	return ret;
}

static int ncp6335d_parse_gpio(struct device_node *node,
					struct ncp6335d_info *dd)
{
	int ret = 0;
	enum of_gpio_flags flags;

	if (!of_find_property(node, "onnn,vsel-gpio", NULL))
		return ret;

	/* Get GPIO connected to vsel and set its output */
	dd->vsel_gpio = of_get_named_gpio_flags(node,
			"onnn,vsel-gpio", 0, &flags);
	if (!gpio_is_valid(dd->vsel_gpio)) {
		if (dd->vsel_gpio != -EPROBE_DEFER)
			dev_err(dd->dev, "Could not get vsel gpio, ret = %d\n",
				dd->vsel_gpio);
		return dd->vsel_gpio;
	}

	ret = devm_gpio_request(dd->dev, dd->vsel_gpio, "ncp6335d_vsel");
	if (ret) {
		dev_err(dd->dev, "Failed to obtain gpio %d ret = %d\n",
				dd->vsel_gpio, ret);
			return ret;
	}

	ret = gpio_direction_output(dd->vsel_gpio, flags & OF_GPIO_ACTIVE_LOW ? 0 : 1);//always set to work, based on logic
	if (ret) {
		dev_err(dd->dev, "Failed to set GPIO %d to: %s, ret = %d",
				dd->vsel_gpio, flags & OF_GPIO_ACTIVE_LOW ?
				"GPIO_LOW" : "GPIO_HIGH", ret);
		return ret;
	}
	dev_info(dd->dev,"vsel gpio %d set to %d, flags %d, Mask %d\n",
			dd->vsel_gpio,flags & OF_GPIO_ACTIVE_LOW ? 0 : 1,flags,OF_GPIO_ACTIVE_LOW);

	//ASUS_BSP +++ Zhengwei_Cai "porting buck convertor for RK"
	if (!of_find_property(node, "onnn,en-gpio", NULL))
		return ret;

	dd->en_gpio = of_get_named_gpio(node,"onnn,en-gpio", 0);
	if (!gpio_is_valid(dd->en_gpio)) {
		if (dd->en_gpio != -EPROBE_DEFER)
			dev_err(dd->dev, "Could not get enable gpio, ret = %d\n",
				dd->en_gpio);
		return dd->en_gpio;
	}

	ret = devm_gpio_request(dd->dev, dd->en_gpio, "ncp6335d_en");
	if (ret) {
		dev_err(dd->dev, "Failed to obtain gpio %d ret = %d\n",
				dd->en_gpio, ret);
			return ret;
	}
	ret = gpio_direction_output(dd->en_gpio, 0);
	if (ret) {
		dev_err(dd->dev, "Failed to set GPIO %d to 0, ret = %d",
				dd->en_gpio, ret);
		return ret;
	}
	//ASUS_BSP --- Zhengwei_Cai "porting buck convertor for RK"

	return ret;
}

//ASUS_BSP +++ Zhengwei_Cai "porting buck convertor for RK"

static int ncp6335d_parse_vreg(struct device* dev,struct ncp6335d_info *dd)
{
	int ret;

	dd->vreg = regulator_get(dev,"vdd_i2c");
	if (!dd->vreg) {
		dev_err(dd->dev, "Unable to get vdd-i2c\n");
		return -1;
	}

	if (regulator_count_voltages(dd->vreg) > 0) {
		ret = regulator_set_voltage(dd->vreg, 1800000,1800000);
			if (ret){
				dev_err(dd->dev,"Unable to set voltage on vdd-i2c\n");
				regulator_put(dd->vreg);
				dd->vreg = NULL;
				return -2;
			}
			ret = regulator_enable(dd->vreg);
			if(ret)
			{
				dev_err(dd->dev,"Unable to enable vdd-i2c\n");
				return -3;
			}
	}
	return ret;
}

static unsigned int voltage_to_reg_val(unsigned int voltage,struct ncp6335d_info *dd)//uV
{
	if(voltage < 600000)
		voltage = 600000;
	else if(voltage > 1393750)
		voltage = 1393750;
	return (((voltage - dd->min_voltage)/dd->step_size));
}

static unsigned int reg_val_to_voltage(unsigned int reg_val,struct ncp6335d_info *dd)//uV
{
	return ((reg_val & NCP6335D_VOUT_SEL_MASK) * dd->step_size) + dd->min_voltage;
}
#if 0
static void ncp6335d_check_register_read_write(struct ncp6335d_info *dd)
{
	unsigned int val;
	unsigned int voltage;
	int rc;

	rc = ncp6335x_read(dd, dd->vsel_reg, &val);
	if (rc) {
		dev_err(dd->dev, "Unable to get volatge rc(%d)", rc);
	}
	voltage = ((val & NCP6335D_VOUT_SEL_MASK) *
				dd->step_size) + dd->min_voltage;

	dev_info(dd->dev,"R/W test, Current voltage is %d uv, %d mv",voltage,voltage/1000);

	voltage = reg_val_to_voltage(0xD9);
	pr_info("R/W test, voltage converted is %d\n",voltage);
	val = voltage_to_reg_val(voltage);
	pr_info("R/W test, reg converted is 0x%x\n",val);
	rc = ncp6335x_write(dd, dd->vsel_reg, val);
	if (rc) {
		dev_err(dd->dev, "Unable to set volatge rc(%d)", rc);
	}

	rc = ncp6335x_read(dd, dd->vsel_reg, &val);
	if (rc) {
		dev_err(dd->dev, "Unable to get volatge rc(%d)", rc);
	}
	pr_info("R/W test, reg read is 0x%x\n",val);
	voltage = reg_val_to_voltage(0xD9);
	pr_info("R/W test, voltage converted is %d uV\n",voltage);
	voltage = ((val & NCP6335D_VOUT_SEL_MASK) *
				dd->step_size) + dd->min_voltage;
	dev_info(dd->dev,"R/W test,Current voltage is %d uv, %d mv",voltage,voltage/1000);
}
#endif
//ASUS_BSP --- Zhengwei_Cai "porting buck convertor for RK"

static int ncp6335d_init(struct i2c_client *client, struct ncp6335d_info *dd,
			const struct ncp6335d_platform_data *pdata)
{
	int rc;
	unsigned int val;

	switch (pdata->default_vsel) {
	case NCP6335D_VSEL0:
		dd->vsel_reg = REG_NCP6335D_PROGVSEL0;
		dd->vsel_backup_reg = REG_NCP6335D_PROGVSEL1;
		dd->mode_bit = NCP6335D_PWM_MODE0;
	break;
	case NCP6335D_VSEL1:
		dd->vsel_reg = REG_NCP6335D_PROGVSEL1;
		dd->vsel_backup_reg = REG_NCP6335D_PROGVSEL0;
		dd->mode_bit = NCP6335D_PWM_MODE1;
	break;
	default:
		dev_err(dd->dev, "Invalid VSEL ID %d\n", pdata->default_vsel);
		return -EINVAL;
	}

	if (of_property_read_bool(client->dev.of_node, "onnn,restore-reg")) {
		rc = ncp6335d_restore_working_reg(client->dev.of_node, dd);//read vsel_backup_reg, write to vsel_reg
		if (rc)
			return rc;
	}

	/* set voltage to 1.1v and set enable bit to 1 */
	rc = ncp6335x_write(dd, dd->vsel_reg, (voltage_to_reg_val(1100000,dd)|NCP6335D_ENABLE));
	if (rc) {
		dev_err(dd->dev, "Unable to set val to vsel reg rc(%d)", rc);
		return -EINVAL;
	}

	/* get the current programmed voltage */
	rc = ncp6335x_read(dd, dd->vsel_reg, &val);
	if (rc) {
		dev_err(dd->dev, "Unable to get volatge rc(%d)", rc);
		return rc;
	}
	dd->curr_voltage = reg_val_to_voltage(val,dd);
	dev_info(dd->dev,"Current voltage is %d uv, %d mv",dd->curr_voltage,dd->curr_voltage/1000);

	/* set discharge */
	rc = ncp6335x_update_bits(dd, REG_NCP6335D_PGOOD,
					NCP6335D_PGOOD_DISCHG,
					(pdata->discharge_enable ?
					NCP6335D_PGOOD_DISCHG : 0));
	if (rc) {
		dev_err(dd->dev, "Unable to set Active Discharge rc(%d)\n", rc);
		return -EINVAL;
	}

	/* set slew rate */
	if (pdata->slew_rate_ns < dd->min_slew_ns ||
			pdata->slew_rate_ns > dd->max_slew_ns) {
		dev_err(dd->dev, "Invalid slew rate %d\n", pdata->slew_rate_ns);
		return -EINVAL;
	}

	dd->slew_rate = pdata->slew_rate_ns;
	val = DIV_ROUND_UP(pdata->slew_rate_ns, dd->min_slew_ns);
	val = ilog2(val);

	rc = ncp6335x_update_bits(dd, REG_NCP6335D_TIMING,
			NCP6335D_SLEW_MASK, val << NCP6335D_SLEW_SHIFT);
	if (rc)
		dev_err(dd->dev, "Unable to set slew rate rc(%d)\n", rc);

	/* Set Sleep mode bit */
	rc = ncp6335x_update_bits(dd, REG_NCP6335D_COMMAND,
				NCP6335D_SLEEP_MODE, pdata->sleep_enable ?
						NCP6335D_SLEEP_MODE : 0);
	if (rc)
		dev_err(dd->dev, "Unable to set sleep mode (%d)\n", rc);

	/* Set forced PWM*/
	rc = ncp6335x_update_bits(dd, REG_NCP6335D_COMMAND,
								NCP6335D_PWM_MODE0, NCP6335D_PWM_MODE0);
	if (rc)
		dev_err(dd->dev, "Unable to set forced PWM mode (%d)\n", rc);

	/* set 5.2 A peak for 3.5 A output current */
	rc = ncp6335x_write(dd, REG_NCP6335D_LIMIT, 0x23);
	if (rc) {
		dev_err(dd->dev, "Unable to set val 0x23 to limit reg rc(%d)", rc);
		return -EINVAL;
	}

#if 0
	dump_registers(dd, REG_NCP6335D_COMMAND,"COMMAND",__func__);
	dump_registers(dd, REG_NCP6335D_PROGVSEL0,"PROGVSEL0",__func__);
	dump_registers(dd, REG_NCP6335D_PROGVSEL1,"PROGVSEL1",__func__);
	dump_registers(dd, REG_NCP6335D_TIMING,"TIMING",__func__);
	dump_registers(dd, REG_NCP6335D_PGOOD,"PGOOD",__func__);
	dump_registers(dd, REG_NCP6335D_LIMIT,"LIMIT",__func__);
#endif
	return rc;
}

static struct regmap_config ncp6335d_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int ncp6335d_parse_dt(struct i2c_client *client,
				struct ncp6335d_info *dd)
{
	int rc;

	rc = of_property_read_u32(client->dev.of_node,
			"onnn,step-size", &dd->step_size);
	if (rc < 0) {
		dev_err(&client->dev, "step size missing: rc = %d.\n", rc);
		return rc;
	}

	rc = of_property_read_u32(client->dev.of_node,
			"onnn,min-slew-ns", &dd->min_slew_ns);
	if (rc < 0) {
		dev_err(&client->dev, "min slew us missing: rc = %d.\n", rc);
		return rc;
	}

	rc = of_property_read_u32(client->dev.of_node,
			"onnn,max-slew-ns", &dd->max_slew_ns);
	if (rc < 0) {
		dev_err(&client->dev, "max slew us missing: rc = %d.\n", rc);
		return rc;
	}

	rc = of_property_read_u32(client->dev.of_node,
			"onnn,min-setpoint", &dd->min_voltage);
	if (rc < 0) {
		dev_err(&client->dev, "min set point missing: rc = %d.\n", rc);
		return rc;
	}

	rc = ncp6335d_parse_gpio(client->dev.of_node, dd);
	if (rc)
	{
		dev_err(&client->dev, "failed parse gpio: rc = %d.\n", rc);
		return rc;
	}

	rc = ncp6335d_parse_vreg(&client->dev, dd);
	if (rc)
	{
		dev_err(&client->dev, "failed parse vreg: rc = %d.\n", rc);
		return rc;
	}

	return rc;
}
static struct ncp6335d_platform_data *
	ncp6335d_get_of_platform_data(struct i2c_client *client)
{
	struct ncp6335d_platform_data *pdata = NULL;
	struct regulator_init_data *init_data;
	const char *mode_name;
	int rc;

	init_data = of_get_regulator_init_data(&client->dev,
				client->dev.of_node);
	if (!init_data) {
		dev_err(&client->dev, "regulator init data is missing\n");
		return pdata;
	}

	pdata = devm_kzalloc(&client->dev,
			sizeof(struct ncp6335d_platform_data), GFP_KERNEL);
	if (!pdata) {
		dev_err(&client->dev, "ncp6335d_platform_data allocation failed.\n");
		return pdata;
	}

	rc = of_property_read_u32(client->dev.of_node,
			"onnn,vsel", &pdata->default_vsel);//default vsel 0
	if (rc < 0) {
		dev_err(&client->dev, "onnn,vsel property missing: rc = %d.\n",
			rc);
		return NULL;
	}

	rc = of_property_read_u32(client->dev.of_node,
			"onnn,slew-ns", &pdata->slew_rate_ns);
	if (rc < 0) {
		dev_err(&client->dev, "onnn,slew-ns property missing: rc = %d.\n",
			rc);
		return NULL;
	}

	pdata->discharge_enable = of_property_read_bool(client->dev.of_node,
						"onnn,discharge-enable");

	pdata->sleep_enable = of_property_read_bool(client->dev.of_node,
						"onnn,sleep-enable");

	pdata->init_data = init_data;

	init_data->constraints.input_uV = init_data->constraints.max_uV;
	init_data->constraints.valid_ops_mask =
				REGULATOR_CHANGE_VOLTAGE |
				REGULATOR_CHANGE_STATUS |
				REGULATOR_CHANGE_MODE;
	init_data->constraints.valid_modes_mask =
				REGULATOR_MODE_NORMAL |
				REGULATOR_MODE_FAST;

	rc = of_property_read_string(client->dev.of_node, "onnn,mode",
					&mode_name);
	if (!rc) {
		if (strcmp("pwm", mode_name) == 0) {
			init_data->constraints.initial_mode =
							REGULATOR_MODE_FAST;
		} else if (strcmp("auto", mode_name) == 0) {
			init_data->constraints.initial_mode =
							REGULATOR_MODE_NORMAL;
		} else {
			dev_err(&client->dev, "onnn,mode, unknown regulator mode: %s\n",
				mode_name);
			return NULL;
		}
	}

	return pdata;
}

static int get_reg(void *data, u64 *val)
{
	struct ncp6335d_info *dd = data;
	int rc;
	unsigned int temp = 0;

	rc = ncp6335x_read(dd, dd->peek_poke_address, &temp);
	if (rc < 0)
		dev_err(dd->dev, "Couldn't read reg %x rc = %d\n",
				dd->peek_poke_address, rc);
	else
		*val = temp;

	dev_info(dd->dev,"read reg[0x%02x] val 0x%02x\n",dd->peek_poke_address,temp);
	return rc;
}

static int set_reg(void *data, u64 val)
{
	struct ncp6335d_info *dd = data;
	int rc;
	unsigned int temp = 0;

	temp = (unsigned int) val;
	rc = ncp6335x_write(dd, dd->peek_poke_address, temp);
	if (rc < 0)
		dev_err(dd->dev, "Couldn't write 0x%02x to 0x%02x rc= %d\n",
			dd->peek_poke_address, temp, rc);

	dev_info(dd->dev,"write reg[0x%02x] val 0x%02x\n",dd->peek_poke_address,temp);
	return rc;
}
DEFINE_SIMPLE_ATTRIBUTE(poke_poke_debug_ops, get_reg, set_reg, "0x%02llx\n");

static uint16_t address_table[]={0x1c,0x18,0x14,0x10};
static int ncp6335d_regulator_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{

	int rc;
	unsigned int val = 0;
	struct ncp6335d_info *dd;
	const struct ncp6335d_platform_data *pdata;
	struct regulator_config config = { };
	int i,j;
	int is_find;
	//dev_info(&client->dev, "NCP6335D Probe start!\n");
	if (client->dev.of_node)
		pdata = ncp6335d_get_of_platform_data(client);
	else
		pdata = client->dev.platform_data;

	if (!pdata) {
		dev_err(&client->dev, "Platform data not specified\n");
		return -EINVAL;
	}

	dd = devm_kzalloc(&client->dev, sizeof(*dd), GFP_KERNEL);
	if (!dd) {
		dev_err(&client->dev, "Unable to allocate memory\n");
		return -ENOMEM;
	}

	dd->init_data = pdata->init_data;
	dd->dev = &client->dev;//dev!

	if (client->dev.of_node) {
		rc = ncp6335d_parse_dt(client, dd);
		if (rc)
			return rc;
	} else {
		dd->step_size	= NCP6335D_STEP_VOLTAGE_UV;
		dd->min_voltage	= NCP6335D_MIN_VOLTAGE_UV;
		dd->min_slew_ns	= NCP6335D_MIN_SLEW_NS;
		dd->max_slew_ns	= NCP6335D_MAX_SLEW_NS;
	}

	dd->regmap = devm_regmap_init_i2c(client, &ncp6335d_regmap_config);
	if (IS_ERR(dd->regmap)) {
		dev_err(&client->dev, "Error allocating regmap\n");
		return PTR_ERR(dd->regmap);
	}
	i2c_set_clientdata(client, dd);

	is_find = 0;
	for(i=0;i<sizeof(address_table)/sizeof(uint16_t);i++)
	{
		client->addr = address_table[i];
		dev_info(&client->dev,"i2c detect address is 0x%x\n",client->addr);

		for(j=0;j<3;j++)
		{
			rc = ncp6335x_read(dd, REG_NCP6335D_PID, &val);
			if (rc) {
				dev_err(&client->dev, "Address 0x%x, Unable to identify NCP6335D, rc(%d), retry later...count %d\n",
											client->addr,rc,j);
				usleep_range(30*1000,30*1000);
			}
			else
			{
				//dev_info(&client->dev, "Detected Regulator NCP6335D PID = 0x%x\n", val);
				if(val != 0x15)
				{
					dev_err(&client->dev,"Address 0x%x, Read PID 0x%x is not 0x15!\n",client->addr,val);
					return rc;
				}
				else
				{
					is_find=1;
					break;
				}
			}
		}
		if(is_find)
		{
			dev_info(&client->dev,"Address 0x%x, detect NCP6335D!\n",client->addr);
			break;
		}
		else
		{
			dev_info(&client->dev,"Address 0x%x, not detect NCP6335D!\n",client->addr);
		}
	}
	if(!is_find)
	{
		dev_err(&client->dev, "Unable to identify NCP6335D, rc(%d)\n",
							rc);
		return rc;
	}

	//dev_info(dd->dev, "NCP6335D dump registers befor init\n");
	//dump_all_registers(dd);
	rc = ncp6335d_init(client, dd, pdata);
	if (rc) {
		dev_err(&client->dev, "Unable to intialize the regulator\n");
		rc = regulator_disable(dd->vreg);
		return -EINVAL;
	}
	ncp6335d_set_reg_to_default(dd);
	//dev_info(dd->dev, "NCP6335D dump registers after init\n");
	//dump_all_registers(dd);

	config.dev = &client->dev;
	config.init_data = dd->init_data;
	config.regmap = dd->regmap;
	config.driver_data = dd;
	config.of_node = client->dev.of_node;

	dd->regulator = regulator_register(&rdesc, &config);

	if (IS_ERR(dd->regulator)) {
		dev_err(&client->dev, "Unable to register regulator rc(%ld)",
						PTR_ERR(dd->regulator));

		return PTR_ERR(dd->regulator);
	}

	dd->debug_root = debugfs_create_dir("ncp6335x", NULL);
	if (!dd->debug_root)
		dev_err(&client->dev, "Couldn't create debug dir\n");

	if (dd->debug_root) {
		struct dentry *ent;

		ent = debugfs_create_x32("address", S_IFREG | S_IWUSR | S_IRUGO,
					  dd->debug_root,
					  &(dd->peek_poke_address));
		if (!ent)
			dev_err(&client->dev, "Couldn't create address debug file rc = %d\n",
									rc);

		ent = debugfs_create_file("data", S_IFREG | S_IWUSR | S_IRUGO,
					  dd->debug_root, dd,
					  &poke_poke_debug_ops);
		if (!ent)
			dev_err(&client->dev, "Couldn't create data debug file rc = %d\n",
									rc);
	}

    the_ncp6335d_info = dd;
	dev_info(&client->dev, "NCP6335D Probe succeed!\n");


	return 0;
}

static int ncp6335d_regulator_remove(struct i2c_client *client)
{
	struct ncp6335d_info *dd = i2c_get_clientdata(client);

	regulator_unregister(dd->regulator);

	debugfs_remove_recursive(dd->debug_root);

	return 0;
}

static struct of_device_id ncp6335d_match_table[] = {
	{ .compatible = "onnn,ncp6335d-regulator", },
	{},
};
MODULE_DEVICE_TABLE(of, ncp6335d_match_table);

static const struct i2c_device_id ncp6335d_id[] = {
	{"ncp6335d", -1},
	{ },
};

static struct i2c_driver ncp6335d_regulator_driver = {
	.driver = {
		.name = "ncp6335d-regulator",
		.owner = THIS_MODULE,
		.of_match_table = ncp6335d_match_table,
	},
	.probe = ncp6335d_regulator_probe,
	.remove = ncp6335d_regulator_remove,
	.id_table = ncp6335d_id,
};

/**
 * ncp6335d_regulator_init() - initialized ncp6335d regulator driver
 * This function registers the ncp6335d regulator platform driver.
 *
 * Returns 0 on success or errno on failure.
 */
int __init ncp6335d_regulator_init(void)
{
	static bool initialized;
	if (initialized)
		return 0;
	else
		initialized = true;
	return i2c_add_driver(&ncp6335d_regulator_driver);
}
EXPORT_SYMBOL(ncp6335d_regulator_init);
arch_initcall(ncp6335d_regulator_init);

static void __exit ncp6335d_regulator_exit(void)
{
	regulator_disable(the_ncp6335d_info->vreg);
	regulator_put(the_ncp6335d_info->vreg);
	gpio_free(the_ncp6335d_info->en_gpio);
	gpio_free(the_ncp6335d_info->vsel_gpio);

	i2c_del_driver(&ncp6335d_regulator_driver);
}
module_exit(ncp6335d_regulator_exit);
MODULE_DESCRIPTION("OnSemi-NCP6335D regulator driver");
MODULE_LICENSE("GPL v2");
