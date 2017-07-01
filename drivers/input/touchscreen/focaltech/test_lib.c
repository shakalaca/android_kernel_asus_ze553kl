/************************************************************************
* Copyright (C) 2012-2015, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: Test_lib.c
*
* Author: Software Development Team, AE
*
* Created: 2015-07-14
*
* Abstract: test entry for all IC
*
************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/slab.h>

#include "test_lib.h"
#include "Global.h"

#include "Config_FT6X36.h"
#include "Test_FT6X36.h"

#include "ini.h"

#define FTS_DRIVER_LIB_INFO  "Test_Lib_Version  V1.4.0 2015-12-03"

FTS_I2C_READ_FUNCTION fts_i2c_read_test_cap_sensors;
FTS_I2C_WRITE_FUNCTION fts_i2c_write_test_cap_sensors;

char *g_testparamstring_cap_sensors = NULL;

/////////////////////IIC communication
int init_i2c_read_func_cap_sensors(FTS_I2C_READ_FUNCTION fpI2C_Read)
{
	fts_i2c_read_test_cap_sensors= fpI2C_Read;
	return 0;
}

int init_i2c_write_func_cap_sensors(FTS_I2C_WRITE_FUNCTION fpI2C_Write)
{
	fts_i2c_write_test_cap_sensors = fpI2C_Write;
	return 0;
}

/************************************************************************
* Name: set_param_data_cap_sensors
* Brief:  load Config. Set IC series, init test items, init basic threshold, int detailThreshold, and set order of test items
* Input: TestParamData, from ini file.
* Output: none
* Return: 0. No sense, just according to the old format.
***********************************************************************/
int set_param_data_cap_sensors(char * TestParamData)
{
	//int time_use = 0;//ms
	//struct timeval time_start;
	//struct timeval time_end;

	//gettimeofday(&time_start, NULL);//Start time
	
	printk("Enter  set_param_data_cap_sensors \n");
	g_testparamstring_cap_sensors = TestParamData;//get param of ini file
	ini_get_key_data_cap_sensors(g_testparamstring_cap_sensors);//get param to struct
	
    //from configure load select chip set
	//Set g_ScreenSetParam_cap_sensors.iSelectedIC
	OnInit_InterfaceCfg_cap_sensors(g_testparamstring_cap_sensors);

	/*Get IC Name*/
	get_ic_name(g_ScreenSetParam_cap_sensors.iSelectedIC, g_strIcName_cap_sensors);

    //test configure
	
	//if(IC_FT3267>>4 == g_ScreenSetParam_cap_sensors.iSelectedIC>>4)
	//{
		OnInit_FT6X36_TestItem(g_testparamstring_cap_sensors);
		OnInit_FT6X36_BasicThreshold(g_testparamstring_cap_sensors);
		OnInit_SCap_DetailThreshold_cap_sensors(g_testparamstring_cap_sensors);
		SetTestItem_FT6X36();
	//}
	
	/*gettimeofday(&time_end, NULL);//End time
	time_use = (time_end.tv_sec - time_start.tv_sec)*1000 + (time_end.tv_usec - time_start.tv_usec)/1000;
	printk("Load Config, use time = %d ms \n", time_use);
	*/
	return 0;
}

/************************************************************************
* Name: start_test_tp_cap_sensors
* Brief:  Test entry. Select test items based on IC series
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/

boolean start_test_tp_cap_sensors(void) 
{
	boolean bTestResult = false;
	printk("[focal] %s \n", FTS_DRIVER_LIB_INFO);	//show lib version
	printk("[focal] %s start \n", __func__);
	printk("[focal] IC_%s Test\n", g_strIcName_cap_sensors);
	
	/*switch(g_ScreenSetParam_cap_sensors.iSelectedIC>>4)
		{
		case IC_FT3267>>4:*/
			bTestResult = FT6X36_StartTest();
			/*break;
		default:
			printk("[focal]  Error IC, IC Name: %s, IC Code:  %d\n", g_strIcName_cap_sensors, g_ScreenSetParam_cap_sensors.iSelectedIC);
			break;
		}*/
    printk("[focal] IC Name: %s, IC Code:  %d\n", g_strIcName_cap_sensors, g_ScreenSetParam_cap_sensors.iSelectedIC);

	return bTestResult;
}
/************************************************************************
* Name: get_test_data_cap_sensors
* Brief:  Get test data based on IC series
* Input: none
* Output: pTestData, External application for memory, buff size >= 1024*8
* Return: the length of test data. if length > 0, got data;else ERR.
***********************************************************************/
int get_test_data_cap_sensors(char *pTestData)
{
	int iLen = 0;
	printk("[focal] %s start \n", __func__);	
	/*switch(g_ScreenSetParam_cap_sensors.iSelectedIC>>4)
		{
		case IC_FT3267>>4:*/
			iLen = FT6X36_get_test_data(pTestData);
			/*break;
		default:
			printk("[focal]  Error IC, IC Name: %s, IC Code:  %d\n", g_strIcName_cap_sensors, g_ScreenSetParam_cap_sensors.iSelectedIC);
			break;
		}*/
    printk("[focal] IC Name: %s, IC Code:  %d\n", g_strIcName_cap_sensors, g_ScreenSetParam_cap_sensors.iSelectedIC);

	return iLen;	
}
/************************************************************************
* Name: free_test_param_data
* Brief:  release printer memory
* Input: none
* Output: none
* Return: none. 
***********************************************************************/
void free_test_param_data(void)
{
	if(g_testparamstring_cap_sensors)
		kfree(g_testparamstring_cap_sensors);

	g_testparamstring_cap_sensors = NULL;
}

/************************************************************************
* Name: show_lib_ver
* Brief:  get lib version
* Input: none
* Output: pLibVer
* Return: the length of lib version. 
***********************************************************************/
int show_lib_ver(char *pLibVer)
{
	int num_read_chars = 0;
	
	num_read_chars = snprintf(pLibVer, 128,"%s \n", FTS_DRIVER_LIB_INFO);

	return num_read_chars;
}


