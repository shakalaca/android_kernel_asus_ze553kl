/************************************************************************
* Copyright (C) 2012-2015, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: Global.c
*
* Author: Software Development Team, AE
*
* Created: 2015-07-14
*
* Abstract: global function for test
*
************************************************************************/
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/printk.h>

#include <linux/delay.h>//msleep

#include "ini.h"
#include "Global.h"

#include "test_lib.h"

#define DEVIDE_MODE_ADDR	0x00

//add by leo ++
// struct fts_ts_data *private_ts; 

extern struct i2c_client *selftest_client;

extern int fts_i2c_write_cap_sensors(struct i2c_client *client, char *writebuf, int writelen);
extern int fts_i2c_read_cap_sensors(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen);
// add by leo --

struct StruScreenSeting g_ScreenSetParam_cap_sensors; //screen configure parameter
struct stTestItem g_stTestItem_cap_sensors[1][MAX_TEST_ITEM];
//struct structSCapConf g_stSCapConf;
int g_TestItemNum_cap_sensors = 0;
char g_strIcName_cap_sensors[20] ={0};

int GetPrivateProfileString_cap_sensors(char *section, char *ItemName, char *defaultvalue, char *returnValue, char *IniFile){
	char value[512] = {0};
	int len = 0;
	
	if(NULL == returnValue)
	{
		printk("[FTS] returnValue==NULL in function %s.", __func__);
		return 0;
	}
	
	if(ini_get_key_cap_sensors(IniFile, section, ItemName, value) < 0) 
	{
		if(NULL != defaultvalue) memcpy(value, defaultvalue, strlen(defaultvalue));
		sprintf(returnValue, "%s", value);
		return 0;
	} else {
		len = sprintf(returnValue, "%s", value);
	}
		
	return len;	
}

void focal_msleep_cap_sensors(int ms)
{
	msleep(ms);
}
void SysDelay_cap_sensors(int ms)
{
	msleep(ms);
}
int focal_abs_cap_sensors(int value)
{
	if(value < 0)
		value = 0 - value;

	return value;
}
/////////////////////////////////////////////////////////////////////////////
////Get IC mapping code
/////////////////////////////////////////////////////////////////////////////
unsigned char get_ic_code(char * strIcName)
{

	if(strncmp(strIcName,"FT5X36",6) == 0) return IC_FT5X36;
	if(strncmp(strIcName, "FT5X36i",7) == 0) return IC_FT5X36i;
	if(strncmp(strIcName, "FT3X16",6) == 0) return IC_FT3X16;
	if(strncmp(strIcName, "FT3X26",6) == 0) return IC_FT3X26;

	if(strncmp(strIcName, "FT5X46",6) == 0) return IC_FT5X46;
	if(strncmp(strIcName, "FT5X46i",7) == 0) return IC_FT5X46i;
	if(strncmp(strIcName, "FT5526",6) == 0) return IC_FT5526;
	if(strncmp(strIcName, "FT3X17",6) == 0) return IC_FT3X17;
	if(strncmp(strIcName, "FT5436",6) == 0) return IC_FT5436;
	if(strncmp(strIcName, "FT3X27",6) == 0) return IC_FT3X27;
	if(strncmp(strIcName, "FT5526i",7) == 0) return IC_FT5526I;
	if(strncmp(strIcName, "FT5416",6) == 0) return IC_FT5416;
	if(strncmp(strIcName, "FT5426",6) == 0) return IC_FT5426;
	if(strncmp(strIcName, "FT5435",6) == 0) return IC_FT5435;
	
	if(strncmp(strIcName, "FT6X06",6) == 0) return IC_FT6X06;
	if(strncmp(strIcName, "FT3X06",6) == 0) return IC_FT3X06;

	if(strncmp(strIcName, "FT6X36",6) == 0) return IC_FT6X36;
	if(strncmp(strIcName, "FT3X07",6) == 0) return IC_FT3X07;
	if(strncmp(strIcName, "FT6416",6) == 0) return IC_FT6416;
    if(strncmp(strIcName, "FT3267",6) == 0) return IC_FT3267;
	if(strncmp(strIcName, "FT6336G/U",9) == 0) return IC_FT6426;

	if(strncmp(strIcName, "FT5X16",6) == 0) return IC_FT5X16;
	if(strncmp(strIcName, "FT5X12",6) == 0) return IC_FT5X12;

	if(strncmp(strIcName, "FT5506",6) == 0) return IC_FT5506;
	if(strncmp(strIcName, "FT5606",6) == 0) return IC_FT5606;
	if(strncmp(strIcName, "FT5816",6) == 0) return IC_FT5816;

	if(strncmp(strIcName, "FT5822",6) == 0) return IC_FT5822;
	if(strncmp(strIcName, "FT5626",6) == 0) return IC_FT5626;
	if(strncmp(strIcName, "FT5726",6) == 0) return IC_FT5726;
	if(strncmp(strIcName, "FT5826B",7) == 0) return IC_FT5826B;
	if(strncmp(strIcName, "FT5826S",7) == 0) return IC_FT5826S;

	if(strncmp(strIcName, "FT5306",6) == 0) return IC_FT5306;
	if(strncmp(strIcName, "FT5406",6) == 0) return IC_FT5406;

	if(strncmp(strIcName, "FT8606",6) == 0) return IC_FT8606;
	
	//if(strIcName == "FT8606")return IC_FT8606;
	if(strncmp(strIcName,"FT8606",6) == 0) return IC_FT8606;
	
	if(strncmp(strIcName, "FT3C47U",7) == 0) return IC_FT3C47U;

	return 0xff;
}
/////////////////////////////////////////////////////////////////////////////
////Get IC name
/////////////////////////////////////////////////////////////////////////////
void get_ic_name(unsigned char ucIcCode, char * strIcName)
{
	if(NULL == strIcName)return;

	sprintf(strIcName, "%s", "NA");/*if can't find IC , set 'NA'*/
	printk("[focal] ucIcCode = 0x%02x\n", ucIcCode);
    ucIcCode = 0x48;
	if(ucIcCode == IC_FT5X36)sprintf(strIcName, "%s", "FT5X36");
	if(ucIcCode == IC_FT5X36i)sprintf(strIcName, "%s",  "FT5X36i");
	if(ucIcCode == IC_FT3X16)sprintf(strIcName, "%s",  "FT3X16");
	if(ucIcCode == IC_FT3X26)sprintf(strIcName, "%s",  "FT3X26");

	if(ucIcCode == IC_FT5X46)sprintf(strIcName, "%s",  "FT5X22");
	if(ucIcCode == IC_FT5X46) sprintf(strIcName, "%s",  "FT5X46");
	if(ucIcCode == IC_FT5X46i) sprintf(strIcName, "%s",  "FT5X46i");
	if(ucIcCode == IC_FT5526) sprintf(strIcName, "%s",  "FT5526");
	if(ucIcCode == IC_FT3X17)  sprintf(strIcName, "%s",  "FT3X17");
	if(ucIcCode == IC_FT5436) sprintf(strIcName, "%s",  "FT5436");
	if(ucIcCode == IC_FT3X27)  sprintf(strIcName, "%s",  "FT3X27");
	if(ucIcCode == IC_FT5526I) sprintf(strIcName, "%s",  "FT5526i");
	if(ucIcCode == IC_FT5416) sprintf(strIcName, "%s",  "FT5416");
	if(ucIcCode == IC_FT5426) sprintf(strIcName, "%s",  "FT5426");
	if(ucIcCode == IC_FT5435) sprintf(strIcName, "%s",  "FT5435");
	
	if(ucIcCode == IC_FT6X06)sprintf(strIcName, "%s",  "FT6X06");
	if(ucIcCode == IC_FT3X06)sprintf(strIcName, "%s",  "FT3X06");

	if(ucIcCode == IC_FT6X36)sprintf(strIcName, "%s",  "FT6X36");
	if(ucIcCode == IC_FT3X07)sprintf(strIcName, "%s",  "FT3X07");
	if(ucIcCode == IC_FT6416)sprintf(strIcName, "%s",  "FT6416");
	if(ucIcCode == IC_FT6426)sprintf(strIcName, "%s",  "FT6336G/U");
    if(ucIcCode == IC_FT3267)sprintf(strIcName, "%s",  "FT3267");

	if(ucIcCode == IC_FT5X16)sprintf(strIcName, "%s",  "FT5X16");
	if(ucIcCode == IC_FT5X12)sprintf(strIcName, "%s",  "FT5X12");

	if(ucIcCode == IC_FT5506)sprintf(strIcName, "%s",  "FT5506");
	if(ucIcCode == IC_FT5606)sprintf(strIcName, "%s",  "FT5606");
	if(ucIcCode == IC_FT5816)sprintf(strIcName, "%s",  "FT5816");

	if(ucIcCode == IC_FT5822)sprintf(strIcName, "%s",  "FT5822");
	if(ucIcCode == IC_FT5626)sprintf(strIcName, "%s",  "FT5626");
	if(ucIcCode == IC_FT5726)sprintf(strIcName, "%s",  "FT5726");
	if(ucIcCode == IC_FT5826B)sprintf(strIcName, "%s",  "FT5826B");
	if(ucIcCode == IC_FT5826S)sprintf(strIcName, "%s",  "FT5826S");

	if(ucIcCode == IC_FT5306)sprintf(strIcName, "%s",  "FT5306");
	if(ucIcCode == IC_FT5406)sprintf(strIcName, "%s",  "FT5406");

	if(ucIcCode == IC_FT8606)sprintf(strIcName, "%s",  "FT8606");
	//if(ucIcCode == IC_FT8716)sprintf(strIcName, "%s",  "FT8716");

	if(ucIcCode == IC_FT3C47U) sprintf(strIcName, "%s",  "FT3C47U");
	
	return ;
}
void OnInit_InterfaceCfg_cap_sensors(char * strIniFile)
{
	char str[128];

	///////////////////////////IC_Type
	GetPrivateProfileString_cap_sensors("Interface","IC_Type","FT5X36",str,strIniFile);
	g_ScreenSetParam_cap_sensors.iSelectedIC = get_ic_code(str);

	/////////////////////////Normalize Type
	GetPrivateProfileString_cap_sensors("Interface","Normalize_Type",0,str,strIniFile);
	g_ScreenSetParam_cap_sensors.isNormalize = atoi(str);	

}
/************************************************************************
* Name: ReadReg_cap_sensors(Same function name as FT_MultipleTest)
* Brief:  Read Register
* Input: RegAddr
* Output: RegData
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
int ReadReg_cap_sensors(unsigned char RegAddr, unsigned char *RegData)
{
	int iRet;
	/*
	if(NULL == fts_i2c_read_cap_sensors)
	{
		printk("[focal] %s fts_i2c_read_cap_sensors == NULL \n", __func__);
		return (ERROR_CODE_INVALID_COMMAND);
	}
	*/
	iRet = fts_i2c_read_cap_sensors(selftest_client, &RegAddr, 1, RegData, 1);

	if(iRet >= 0)
		return (ERROR_CODE_OK);
	else
		return (ERROR_CODE_COMM_ERROR);
}
/************************************************************************
* Name: WriteReg_cap_sensors(Same function name as FT_MultipleTest)
* Brief:  Write Register
* Input: RegAddr, RegData
* Output: null
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
int WriteReg_cap_sensors(unsigned char RegAddr, unsigned char RegData)
{
	int iRet;
	unsigned char cmd[2] = {0};

	/*
	if(NULL == fts_i2c_write_cap_sensors)
	{
		printk("[focal] %s fts_i2c_write_cap_sensors == NULL \n", __func__);
		return (ERROR_CODE_INVALID_COMMAND);
	}
	*/
	
	cmd[0] = RegAddr;
	cmd[1] = RegData;
	iRet = fts_i2c_write_cap_sensors(selftest_client, cmd, 2);

	if(iRet >= 0)
		return (ERROR_CODE_OK);
	else
		return (ERROR_CODE_COMM_ERROR);
}
/************************************************************************
* Name: Comm_Base_IIC_IO_cap_sensors(Same function name as FT_MultipleTest)
* Brief:  Write/Read Data by IIC
* Input: pWriteBuffer, iBytesToWrite, iBytesToRead
* Output: pReadBuffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char Comm_Base_IIC_IO_cap_sensors(unsigned char *pWriteBuffer, int  iBytesToWrite, unsigned char *pReadBuffer, int iBytesToRead)
{
	int iRet;	
	/*
	if(NULL == fts_i2c_read_cap_sensors)
	{
		printk("[focal] %s fts_i2c_read_cap_sensors == NULL \n", __func__);
		return (ERROR_CODE_INVALID_COMMAND);
	}
	*/	
	iRet = fts_i2c_read_cap_sensors(selftest_client, pWriteBuffer, iBytesToWrite, pReadBuffer, iBytesToRead);

	if(iRet >= 0)
		return (ERROR_CODE_OK);
	else
		return (ERROR_CODE_COMM_ERROR);
}
/************************************************************************
* Name: EnterWork_cap_sensors(Same function name as FT_MultipleTest)
* Brief:  Enter Work Mode
* Input: null
* Output: null
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char EnterWork_cap_sensors(void)
{
	unsigned char RunState = 0;
	unsigned char ReCode = ERROR_CODE_COMM_ERROR;

	ReCode = ReadReg_cap_sensors(DEVIDE_MODE_ADDR, &RunState);
	if(ReCode == ERROR_CODE_OK)
	{
		if(((RunState>>4)&0x07) == 0x00)	//work
		{
			ReCode = ERROR_CODE_OK;
		}
		else
		{
			ReCode = WriteReg_cap_sensors(DEVIDE_MODE_ADDR, 0);
			if(ReCode == ERROR_CODE_OK)
			{
				ReCode = ReadReg_cap_sensors(DEVIDE_MODE_ADDR, &RunState);
				if(ReCode == ERROR_CODE_OK)
				{	
					if(((RunState>>4)&0x07) == 0x00)	ReCode = ERROR_CODE_OK;
					else	ReCode = ERROR_CODE_COMM_ERROR;
				}
			}
		}
	}

	return ReCode;
	
}
/************************************************************************
* Name: EnterFactory_cap_sensors
* Brief:  enter Fcatory Mode
* Input: null
* Output: null
* Return: Comm Code. Code = 0 is OK, else fail.
***********************************************************************/
unsigned char EnterFactory_cap_sensors(void)
{
	unsigned char RunState = 0;
	unsigned char ReCode = ERROR_CODE_COMM_ERROR;

	ReCode = ReadReg_cap_sensors(DEVIDE_MODE_ADDR, &RunState);
	if(ReCode == ERROR_CODE_OK)
	{
		if(((RunState>>4)&0x07) == 0x04)	//factory
		{
			ReCode = ERROR_CODE_OK;
		}
		else
		{
			ReCode = WriteReg_cap_sensors(DEVIDE_MODE_ADDR, 0x40);
			if(ReCode == ERROR_CODE_OK)
			{
				ReCode = ReadReg_cap_sensors(DEVIDE_MODE_ADDR, &RunState);
				if(ReCode == ERROR_CODE_OK)
				{	
					if(((RunState>>4)&0x07) == 0x04)	ReCode = ERROR_CODE_OK;
					else	ReCode = ERROR_CODE_COMM_ERROR;
				}
				else
					printk("EnterFactory_cap_sensors read DEVIDE_MODE_ADDR error 3.\n");
			}
			else
				printk("EnterFactory_cap_sensors write DEVIDE_MODE_ADDR error 2.\n");
		}
	}
	else
		printk("EnterFactory_cap_sensors read DEVIDE_MODE_ADDR error 1.\n");

	return ReCode;
}

