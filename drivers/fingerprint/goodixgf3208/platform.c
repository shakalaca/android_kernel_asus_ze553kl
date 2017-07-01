#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/err.h>

#include "gf_spi.h"

#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif

/*GPIO pins reference.*/
int gf_parse_dts(struct gf_dev* gf_dev)
{
    int rc = 0;
    /*get pwr resource*/
/*
    gf_dev->pwr_gpio = of_get_named_gpio(gf_dev->spi->dev.of_node,"goodix,gpio_pwr",0);
    if(!gpio_is_valid(gf_dev->pwr_gpio)) {
        pr_info("PWR GPIO is invalid.\n");
        return -1;
    }
    rc = gpio_request(gf_dev->pwr_gpio, "goodix_pwr");
    if(rc) {
        dev_err(&gf_dev->spi->dev, "Failed to request PWR GPIO. rc = %d\n", rc);
        return -1;
    }
*/
	printk("[Goodix] : gf_parse_dts ENTER!\n");
	gf_dev->vreg = regulator_get(&gf_dev->spi->dev,"vcc_spi");
	if (!gf_dev->vreg) {
		dev_err(&gf_dev->spi->dev, "Unable to get vcc_spi\n");
		return -1;
	}
	
	if (regulator_count_voltages(gf_dev->vreg) > 0) {
		rc = regulator_set_voltage(gf_dev->vreg, 1800000,1800000);
			if (rc){
				dev_err(&gf_dev->spi->dev,"Unable to set voltage on vcc_spi");
				return -1;
			}
	}

	gf_dev->areg = regulator_get(&gf_dev->spi->dev,"vdd_ana");
	if (!gf_dev->areg) {
		dev_err(&gf_dev->spi->dev, "Unable to get vdd_ana\n");
		return -1;
	}
	rc = regulator_set_voltage(gf_dev->areg, 3300000,3300000);
	if (rc){
		dev_err(&gf_dev->spi->dev,"Unable to set voltage on vdd_ana");
		return -1;
	}

	rc = regulator_enable(gf_dev->vreg);
	if (rc) {
		dev_err(&gf_dev->spi->dev, "error enabling vcc_spi %d\n",rc);
		regulator_put(gf_dev->vreg);
		gf_dev->vreg = NULL;
	}
	dev_dbg(&gf_dev->spi->dev,"Set voltage on vcc_spi for goodix fingerprint");

	rc = regulator_enable(gf_dev->areg);
	if (rc) {
		dev_err(&gf_dev->spi->dev, "error enabling vdd_ana %d\n",rc);
		regulator_put(gf_dev->areg);
		gf_dev->areg = NULL;
	}
	dev_dbg(&gf_dev->spi->dev,"Set voltage on vdd_ana for goodix fingerprint");

    /*get reset resource*/
    gf_dev->reset_gpio = of_get_named_gpio(gf_dev->spi->dev.of_node,"goodix,gpio_reset",0);
    if(!gpio_is_valid(gf_dev->reset_gpio)) {
        pr_info("RESET GPIO is invalid.\n");
        return -1;
    }
    rc = gpio_request(gf_dev->reset_gpio, "goodix_reset");
    if(rc) {
        dev_err(&gf_dev->spi->dev, "Failed to request RESET GPIO. rc = %d\n", rc);
        return -1;
    }
    gpio_direction_output(gf_dev->reset_gpio, 1);

    /*get irq resourece*/
    gf_dev->irq_gpio = of_get_named_gpio(gf_dev->spi->dev.of_node,"goodix,gpio_irq",0);
    pr_info("gf:irq_gpio:%d\n", gf_dev->irq_gpio);
    if(!gpio_is_valid(gf_dev->irq_gpio)) {
        pr_info("IRQ GPIO is invalid.\n");
        return -1;
    }

    rc = gpio_request(gf_dev->irq_gpio, "goodix_irq");
    if(rc) {
        dev_err(&gf_dev->spi->dev, "Failed to request IRQ GPIO. rc = %d\n", rc);
        return -1;
    }
    gpio_direction_input(gf_dev->irq_gpio);

	
    //power on
	//gpio_direction_output(gf_dev->pwr_gpio, 1);

	printk("[Goodix] : gf_parse_dts END!\n");
    return 0;
}

void gf_cleanup(struct gf_dev	* gf_dev)
{
    pr_info("[info] %s\n",__func__);
    if (gpio_is_valid(gf_dev->irq_gpio))
    {
        gpio_free(gf_dev->irq_gpio);
        pr_info("remove irq_gpio success\n");
    }
    if (gpio_is_valid(gf_dev->reset_gpio))
    {
        gpio_free(gf_dev->reset_gpio);
        pr_info("remove reset_gpio success\n");
    }
 /*   if (gpio_is_valid(gf_dev->pwr_gpio))
    {
        gpio_free(gf_dev->pwr_gpio);
        pr_info("remove pwr_gpio success\n");
    }*/
    regulator_put(gf_dev->vreg);
	gf_dev->vreg=NULL;
	regulator_put(gf_dev->areg);
	gf_dev->areg=NULL;
	pr_info("[info] %s end\n",__func__);
}

/*power management*/
int gf_power_on(struct gf_dev* gf_dev)
{
    int rc = 0;
/*
    if (gpio_is_valid(gf_dev->pwr_gpio)) {
        gpio_set_value(gf_dev->pwr_gpio, 1);
    }*/
	if(gf_dev->vreg!=NULL)
    {
		rc = regulator_enable(gf_dev->vreg);
		if (rc) {
			dev_err(&gf_dev->spi->dev, "error enabling vcc_spi %d\n",rc);
			regulator_put(gf_dev->vreg);
			gf_dev->vreg = NULL;
		}
		dev_dbg(&gf_dev->spi->dev,"Set voltage on vcc_spi for goodix fingerprint");
	}

	if(gf_dev->areg!=NULL)
    {
		rc = regulator_enable(gf_dev->areg);
		if (rc) {
			dev_err(&gf_dev->spi->dev, "error enabling vdd_ana %d\n",rc);
			regulator_put(gf_dev->areg);
			gf_dev->areg = NULL;
		}
		dev_dbg(&gf_dev->spi->dev,"Set voltage on vdd_ana for goodix fingerprint");
	}
    msleep(10);
    pr_info("---- power on ok ----\n");

    return rc;
}

int gf_power_off(struct gf_dev* gf_dev)
{
    int rc = 0;			
 /*   if (gpio_is_valid(gf_dev->pwr_gpio)) {
        gpio_set_value(gf_dev->pwr_gpio, 1);
    }*/
	if(gf_dev->vreg!=NULL)
    {
		rc = regulator_disable(gf_dev->vreg);
		if (rc) {
			dev_err(&gf_dev->spi->dev, "error disabling vdd_spi %d\n",rc);
		}
	}
	
	if(gf_dev->areg!=NULL)
    {
		rc = regulator_disable(gf_dev->areg);
		if (rc) {
			dev_err(&gf_dev->spi->dev, "error disabling vdd_ana %d\n",rc);
		}
	}
	
    pr_info("---- power off ----\n");
    return rc;
}

/********************************************************************
 *CPU output low level in RST pin to reset GF. This is the MUST action for GF.
 *Take care of this function. IO Pin driver strength / glitch and so on.
 ********************************************************************/
int gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms)
{	
    if(gf_dev == NULL) {
        pr_info("Input buff is NULL.\n");
        return -1;
    }
    gpio_direction_output(gf_dev->reset_gpio, 1);
    gpio_set_value(gf_dev->reset_gpio, 0);
    mdelay(3);
    gpio_set_value(gf_dev->reset_gpio, 1);
    mdelay(delay_ms);
    return 0;
}

int gf_irq_num(struct gf_dev *gf_dev)
{
    if(gf_dev == NULL) {
        pr_info("Input buff is NULL.\n");
        return -1;
    } else {
        return gpio_to_irq(gf_dev->irq_gpio);
    }
}

