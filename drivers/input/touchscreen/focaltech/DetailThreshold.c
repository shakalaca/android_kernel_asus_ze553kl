/************************************************************************
* Copyright (C) 2012-2015, Focaltech Systems (R)，All Rights Reserved.
*
* File Name: DetailThreshold.c
*
* Author: Software Development Team, AE
*
* Created: 2015-07-14
*
* Abstract: Set Detail Threshold for all IC
*
************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>

#include "ini.h"
#include "DetailThreshold.h"
#include "test_lib.h"
#include "Global.h"

struct stCfg_MCap_DetailThreshold g_stCfg_MCap_DetailThreshold_cap_sensors;
struct stCfg_SCap_DetailThreshold g_stCfg_SCap_DetailThreshold_cap_sensors;


void set_max_channel_num_cap_sensors(void)
{
	switch(g_ScreenSetParam_cap_sensors.iSelectedIC>>4)
		{
		case IC_FT5822>>4:
			g_ScreenSetParam_cap_sensors.iUsedMaxTxNum= TX_NUM_MAX;
			g_ScreenSetParam_cap_sensors.iUsedMaxRxNum = RX_NUM_MAX;
			break;
		default:
			g_ScreenSetParam_cap_sensors.iUsedMaxTxNum = 30;
			g_ScreenSetParam_cap_sensors.iUsedMaxRxNum = 30;
			break;
		}

}
void OnInit_SCap_DetailThreshold_cap_sensors(char *strIniFile)
{
	OnGetTestItemParam_cap_sensors("RawDataTest_Max", strIniFile, 12500);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.RawDataTest_Max, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));
	OnGetTestItemParam_cap_sensors("RawDataTest_Min", strIniFile, 16500);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.RawDataTest_Min, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));
	OnGetTestItemParam_cap_sensors("CiTest_Max", strIniFile, 5);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.CiTest_Max, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));
	OnGetTestItemParam_cap_sensors("CiTest_Min", strIniFile, 250);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.CiTest_Min, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));
	OnGetTestItemParam_cap_sensors("DeltaCiTest_Base", strIniFile, 0);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.DeltaCiTest_Base, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));
	OnGetTestItemParam_cap_sensors("DeltaCiTest_AnotherBase1", strIniFile, 0);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.DeltaCiTest_AnotherBase1, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));
	OnGetTestItemParam_cap_sensors("DeltaCiTest_AnotherBase2", strIniFile, 0);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.DeltaCiTest_AnotherBase2, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));
	OnGetTestItemParam_cap_sensors("NoiseTest_Max", strIniFile, 20);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.NoiseTest_Max, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));

	//OnGetTestItemParam_cap_sensors("CiDeviation_Base", strIniFile);
	OnGetTestItemParam_cap_sensors("CiDeviation_Base", strIniFile,0);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.CiDeviationTest_Base, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));

	OnGetTestItemParam_cap_sensors("DeltaCxTest_Sort", strIniFile, 1);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.DeltaCxTest_Sort, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));

	OnGetTestItemParam_cap_sensors("DeltaCxTest_Area", strIniFile, 1);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.DeltaCxTest_Area, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));

	//6x36
	//OnGetTestItemParam_cap_sensors("CbTest_Max", strIniFile);
	OnGetTestItemParam_cap_sensors("CbTest_Max", strIniFile, 0);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.CbTest_Max, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));
	
	//OnGetTestItemParam_cap_sensors("CbTest_Min", strIniFile);
	OnGetTestItemParam_cap_sensors("CbTest_Min", strIniFile, 0);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.CbTest_Min, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));

	//OnGetTestItemParam_cap_sensors("DeltaCbTest_Base", strIniFile);
	OnGetTestItemParam_cap_sensors("DeltaCbTest_Base", strIniFile, 0);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.DeltaCbTest_Base, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));

	//OnGetTestItemParam_cap_sensors("DifferTest_Base", strIniFile);
	OnGetTestItemParam_cap_sensors("DifferTest_Base", strIniFile, 0);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.DifferTest_Base, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));

	//OnGetTestItemParam_cap_sensors("CBDeviation_Base", strIniFile);
	OnGetTestItemParam_cap_sensors("CBDeviation_Base", strIniFile, 0);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.CBDeviationTest_Base, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));

	//OnGetTestItemParam_cap_sensors("K1DifferTest_Base", strIniFile);
	OnGetTestItemParam_cap_sensors("K1DifferTest_Base", strIniFile, 0);
	memcpy(g_stCfg_SCap_DetailThreshold_cap_sensors.K1DifferTest_Base, g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, MAX_CHANNEL_NUM*sizeof(int));
}

void OnGetTestItemParam_cap_sensors(char *strItemName, char *strIniFile, int iDefautValue)
{
	//char str[430];
	char strValue[800];
	char str_tmp[128];
	int iValue = 0;
	int dividerPos=0; 
	int index = 0;
	int i = 0, j=0, k = 0;
	memset(g_stCfg_SCap_DetailThreshold_cap_sensors.TempData, 0, sizeof(g_stCfg_SCap_DetailThreshold_cap_sensors.TempData));
	sprintf(str_tmp, "%d", iDefautValue);
	GetPrivateProfileString_cap_sensors( "Basic_Threshold", strItemName, str_tmp, strValue, strIniFile); 
	iValue = atoi(strValue);
	for(i = 0; i < MAX_CHANNEL_NUM; i++)
	{
		g_stCfg_SCap_DetailThreshold_cap_sensors.TempData[i] = iValue;
	}
	
	dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", strItemName, "", strValue, strIniFile); 
	//sprintf(strValue, "%s", str);	
	if(dividerPos > 0)	
	{		
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_SCap_DetailThreshold_cap_sensors.TempData[k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}
		//If (k+1) < MAX_CHANNEL_NUM, set Default Vaule to Other
		//for(i = k+1; i < MAX_CHANNEL_NUM; i++)
		//{
		//	g_stCfg_SCap_DetailThreshold_cap_sensors.TempData[i] = iValue;
		//}
	}
}
void OnInit_MCap_DetailThreshold_cap_sensors(char *strIniFile)
{
	set_max_channel_num_cap_sensors();//set used TxRx
	
	OnInit_InvalidNode_cap_sensors(strIniFile);
	OnInit_DThreshold_RawDataTest_cap_sensors(strIniFile);
	OnInit_DThreshold_SCapRawDataTest_Cap_Sensors(strIniFile);
	OnInit_DThreshold_SCapCbTest_cap_sensors(strIniFile);

	/*OnInit_DThreshold_ForceTouch_SCapRawDataTest(strIniFile);
	OnInit_DThreshold_ForceTouch_SCapCbTest(strIniFile);*/
	
/*	OnInit_DThreshold_RxCrosstalkTest(strIniFile);
	OnInit_DThreshold_PanelDifferTest(strIniFile);*/
	OnInit_DThreshold_RxLinearityTest_cap_sensors(strIniFile);
	OnInit_DThreshold_TxLinearityTest_cap_sensors(strIniFile);
/*	OnInit_DThreshold_TxShortTest(strIniFile);


	OnInit_DThreshold_CMTest(strIniFile);

    OnInit_DThreshold_NoiseTest(strIniFile);

	//5422 SITO_RAWDATA_UNIFORMITY_TEST
	OnInit_DThreshold_SITORawdata_RxLinearityTest(strIniFile);
	OnInit_DThreshold_SITORawdata_TxLinearityTest(strIniFile);

	OnInit_DThreshold_SITO_RxLinearityTest(strIniFile);
	OnInit_DThreshold_SITO_TxLinearityTest(strIniFile);

	OnInit_DThreshold_UniformityRxLinearityTest(strIniFile);
	OnInit_DThreshold_UniformityTxLinearityTest(strIniFile);
*/
}
void OnInit_InvalidNode_cap_sensors(char *strIniFile)
{
	
	char str[MAX_PATH] = {0},strTemp[MAX_PATH] = {0};
	int i = 0, j=0;
	//memset(str , 0x00, sizeof(str));
	//memset(strTemp , 0x00, sizeof(strTemp));	
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			sprintf(strTemp, "InvalidNode[%d][%d]", (i+1), (j+1));
			
			GetPrivateProfileString_cap_sensors("INVALID_NODE",strTemp,"1",str, strIniFile);
			if(atoi(str) == 0)
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.InvalidNode[i][j] = 0;
			}
			else if( atoi( str ) == 2 )
			{
             			g_stCfg_MCap_DetailThreshold_cap_sensors.InvalidNode[i][j] = 2;
			}
			else
             			g_stCfg_MCap_DetailThreshold_cap_sensors.InvalidNode[i][j] = 1;
		}
	}

	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			sprintf(strTemp, "InvalidNodeS[%d][%d]", (i+1), (j+1));
			GetPrivateProfileString_cap_sensors("INVALID_NODES",strTemp,"1",str, strIniFile);
			if(atoi(str) == 0)
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.InvalidNode_SC[i][j] = 0;
			}
			else if( atoi( str ) == 2 )
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.InvalidNode_SC[i][j] = 2;
			}
			else
				g_stCfg_MCap_DetailThreshold_cap_sensors.InvalidNode_SC[i][j] = 1;
		}
		
	}
}

void OnInit_DThreshold_RawDataTest_cap_sensors(char *strIniFile)
{
	char str[128], strTemp[MAX_PATH],strValue[MAX_PATH];
	int MaxValue, MinValue;
	int   dividerPos=0; 
	char str_tmp[128];
	int index = 0;
	int  k = 0, i = 0, j = 0;
	////////////////////////////RawData Test
	GetPrivateProfileString_cap_sensors( "Basic_Threshold","RawDataTest_Max","10000",str, strIniFile);
	MaxValue = atoi(str);

	//printk("MaxValue = %d  \n", MaxValue);
		
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.RawDataTest_Max[i][j] = MaxValue;
		}
	}
	
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		sprintf(str, "RawData_Max_Tx%d", (i + 1));
		//printk("%s \n", str);
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "111",strTemp, strIniFile);
		//printk("GetPrivateProfileString_cap_sensors = %d \n", dividerPos);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;		
		memset(str_tmp, 0, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.RawDataTest_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}
		
	}

	GetPrivateProfileString_cap_sensors("Basic_Threshold","RawDataTest_Min","7000",str, strIniFile);
	MinValue = atoi(str);

	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.RawDataTest_Min[i][j] = MinValue;
		}
	}
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		sprintf(str, "RawData_Min_Tx%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.RawDataTest_Min[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}
	}

	//RawData Test Low
	GetPrivateProfileString_cap_sensors( "Basic_Threshold","RawDataTest_Low_Max","15000",str, strIniFile);
	MaxValue = atoi(str);

	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.RawDataTest_Low_Max[i][j] = MaxValue;
		}
	}
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		sprintf(str, "RawData_Max_Low_Tx%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.RawDataTest_Low_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}

	GetPrivateProfileString_cap_sensors("Basic_Threshold","RawDataTest_Low_Min","3000",str, strIniFile);
	MinValue = atoi(str);

	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.RawDataTest_Low_Min[i][j] = MinValue;
		}
	}
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		sprintf(str, "RawData_Min_Low_Tx%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.RawDataTest_Low_Min[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}
	}

	//RawData Test High
	GetPrivateProfileString_cap_sensors( "Basic_Threshold","RawDataTest_High_Max","15000",str, strIniFile);
	MaxValue = atoi(str);

	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.RawDataTest_High_Max[i][j] = MaxValue;
		}
	}
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		sprintf(str, "RawData_Max_High_Tx%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.RawDataTest_High_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}
	}

	GetPrivateProfileString_cap_sensors("Basic_Threshold","RawDataTest_High_Min","3000",str, strIniFile);
	MinValue = atoi(str);

	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.RawDataTest_High_Min[i][j] = MinValue;
		}
	}
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		sprintf(str, "RawData_Min_High_Tx%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.RawDataTest_High_Min[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}
	}
/*
	//TxShortAdvance Test
    //different from other test
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.TxShortAdvance[i][j] = 0;
		}
	}
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		sprintf(str, "TxShortAdvanceThreshold_Tx%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.TxShortAdvance[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}*/	
}
void OnInit_DThreshold_SCapRawDataTest_Cap_Sensors(char *strIniFile)
{
	char str[128], strTemp[MAX_PATH],strValue[MAX_PATH];
	int MaxValue, MinValue;
	int   dividerPos=0; 
	char str_tmp[128];
	int index = 0;
	int  k = 0, i = 0, j = 0;

	//////////////////OFF
	GetPrivateProfileString_cap_sensors("Basic_Threshold","SCapRawDataTest_OFF_Min","150",str,strIniFile);
	MinValue = atoi(str);
	GetPrivateProfileString_cap_sensors("Basic_Threshold","SCapRawDataTest_OFF_Max","1000",str,strIniFile);
	MaxValue = atoi(str);
	
	///Max
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_OFF_Max[i][j] = MaxValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ScapRawData_OFF_Max_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_OFF_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}
	////Min
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_OFF_Min[i][j] = MinValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ScapRawData_OFF_Min_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_OFF_Min[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}	
/*
    //load threshold is not specific that will basic threshold instead.
	for(int i = 0; i < 2; i++)
	{
		if(0 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxRxNum;
		if(1 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxTxNum;

		str.Format("ScapRawData_OFF_Max_%d", i + 1);
		GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp.GetBuffer(BUFFER_LENGTH),BUFFER_LENGTH, strIniFile);
		strValue.Format("%s",strTemp);		
		dividerPos = strValue.Find(',',0); 
		if(dividerPos > 0)
		{
			for(int j = 0; j < iLenght; j++)
			{
				AfxExtractSubString(SingleItem, strValue, j ,  cDivider);
				if(!SingleItem.IsEmpty())
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_OFF_Max[i][j] = atoi(SingleItem);
				}
				else
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_OFF_Max[i][j] = iBasicMax;
				}
			}
		}
		else
		{
			for(int j = 0; j < iLenght; j++)
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_OFF_Max[i][j] = iBasicMax;
			}
		}
	}

	for(int i = 0; i < 2; i++)
	{						
		if(0 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxRxNum;
		if(1 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxTxNum;

		str.Format("ScapRawData_OFF_Min_%d", i + 1);
		GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp.GetBuffer(BUFFER_LENGTH),BUFFER_LENGTH, strIniFile);
		strValue.Format("%s",strTemp);		
		dividerPos = strValue.Find(',',0); 
		if(dividerPos > 0)
		{
			for(int j = 0; j < iLenght; j++)
			{
				AfxExtractSubString(SingleItem, strValue, j ,  cDivider);
				if(!SingleItem.IsEmpty())
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_OFF_Min[i][j] = atoi(SingleItem);
				}
				else
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_OFF_Min[i][j] = iBasicMin;
				}
			}
		}
		else
		{
			for(int j = 0; j < iLenght; j++)
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_OFF_Min[i][j] = iBasicMin;
			}
		}
	}
*/
	//////////////////ON
	GetPrivateProfileString_cap_sensors("Basic_Threshold","SCapRawDataTest_ON_Min","150",str,strIniFile);
	MinValue = atoi(str);
	GetPrivateProfileString_cap_sensors("Basic_Threshold","SCapRawDataTest_ON_Max","1000",str,strIniFile);
	MaxValue = atoi(str);

	//load threshold is not specific that will basic threshold instead.
	
	///Max
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_ON_Max[i][j] = MaxValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ScapRawData_ON_Max_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_ON_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}
	////Min
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_ON_Min[i][j] = MinValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ScapRawData_ON_Min_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_ON_Min[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}
	/*
	for(int i = 0; i < 2; i++)
	{
		if(0 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxRxNum;
		if(1 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxTxNum;

		str.Format("ScapRawData_ON_Max_%d", i + 1);
		GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp.GetBuffer(BUFFER_LENGTH),BUFFER_LENGTH, strIniFile);
		strValue.Format("%s",strTemp);		
		dividerPos = strValue.Find(',',0); 
		if(dividerPos > 0)
		{
			for(int j = 0; j < iLenght; j++)
			{
				AfxExtractSubString(SingleItem, strValue, j ,  cDivider);
				if(!SingleItem.IsEmpty())
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_ON_Max[i][j] = atoi(SingleItem);
				}
				else
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_ON_Max[i][j] = iBasicMax;
				}
			}
		}
		else
		{
			for(int j = 0; j < iLenght; j++)
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_ON_Max[i][j] = iBasicMax;
			}
		}
	}

	for(int i = 0; i < 2; i++)
	{
		if(0 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxRxNum;
		if(1 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxTxNum;

		str.Format("ScapRawData_ON_Min_%d", i + 1);
		GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp.GetBuffer(BUFFER_LENGTH),BUFFER_LENGTH, strIniFile);
		strValue.Format("%s",strTemp);		
		dividerPos = strValue.Find(',',0); 
		if(dividerPos > 0)
		{
			for(int j = 0; j < iLenght; j++)
			{
				AfxExtractSubString(SingleItem, strValue, j ,  cDivider);
				if(!SingleItem.IsEmpty())
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_ON_Min[i][j] = atoi(SingleItem);
				}
				else
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_ON_Min[i][j] = iBasicMin;
				}
			}
		}
		else
		{
			for(int j = 0; j < iLenght; j++)
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapRawDataTest_ON_Min[i][j] = iBasicMin;
			}
		}
	}
*/
}
void OnInit_DThreshold_SCapCbTest_cap_sensors(char *strIniFile)
{
	char str[128], strTemp[MAX_PATH],strValue[MAX_PATH];
	int MaxValue, MinValue;
	int   dividerPos=0; 
	char str_tmp[128];
	int index = 0;
	int  k = 0, i = 0, j = 0;

	GetPrivateProfileString_cap_sensors("Basic_Threshold","SCapCbTest_ON_Min","0",str,strIniFile);
	MinValue = atoi(str);
	GetPrivateProfileString_cap_sensors("Basic_Threshold","SCapCbTest_ON_Max","240",str,strIniFile);
	MaxValue = atoi(str);
	//load threshold is not specific that will basic threshold instead.
	
	///Max
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_ON_Max[i][j] = MaxValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ScapCB_ON_Max_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_ON_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}
	////Min
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_ON_Min[i][j] = MinValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ScapCB_ON_Min_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_ON_Min[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}
	GetPrivateProfileString_cap_sensors("Basic_Threshold","SCapCbTest_OFF_Min","0",str,strIniFile);
	MinValue = atoi(str);
	GetPrivateProfileString_cap_sensors("Basic_Threshold","SCapCbTest_OFF_Max","240",str,strIniFile);
	MaxValue = atoi(str);
	///Max
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_OFF_Max[i][j] = MaxValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ScapCB_OFF_Max_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_OFF_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}
	////Min
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_OFF_Min[i][j] = MinValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ScapCB_OFF_Min_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_OFF_Min[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}	
/*
	int iLenght = 0;

	//load threshold is not specific that will basic threshold instead.

	for(int i = 0; i < 2; i++)
	{
		if(0 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxRxNum;
		if(1 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxTxNum;

		str.Format("ScapCB_ON_Max_%d", i + 1);
		GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp.GetBuffer(BUFFER_LENGTH),BUFFER_LENGTH, strIniFile);
		strValue.Format("%s",strTemp);		
		dividerPos = strValue.Find(',',0); 
		if(dividerPos > 0)
		{
			for(int j = 0; j < iLenght; j++)
			{
				AfxExtractSubString(SingleItem, strValue, j ,  cDivider);
				if(!SingleItem.IsEmpty())
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_ON_Max[i][j] = atoi(SingleItem);
				}
				else
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_ON_Max[i][j] = iBasicMax_on;
				}
			}
		}
		else
		{
			for(int j = 0; j < iLenght; j++)
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_ON_Max[i][j] = iBasicMax_on;
			}
		}
	}

	for(int i = 0; i < 2; i++)
	{
		if(0 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxRxNum;
		if(1 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxTxNum;

		str.Format("ScapCB_ON_Min_%d", i + 1);
		GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp.GetBuffer(BUFFER_LENGTH),BUFFER_LENGTH, strIniFile);
		strValue.Format("%s",strTemp);		
		dividerPos = strValue.Find(',',0); 
		if(dividerPos > 0)
		{
			for(int j = 0; j < iLenght; j++)
			{
				AfxExtractSubString(SingleItem, strValue, j ,  cDivider);
				if(!SingleItem.IsEmpty())
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_ON_Min[i][j] = atoi(SingleItem);
				}
				else
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_ON_Min[i][j] = iBasicMin_on;
				}
			}
		}
		else
		{
			for(int j = 0; j < iLenght; j++)
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_ON_Min[i][j] = iBasicMin_on;
			}
		}
	}

	for(int i = 0; i < 2; i++)
	{
		if(0 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxRxNum;
		if(1 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxTxNum;

		str.Format("ScapCB_OFF_Max_%d", i + 1);
		GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp.GetBuffer(BUFFER_LENGTH),BUFFER_LENGTH, strIniFile);
		strValue.Format("%s",strTemp);		
		dividerPos = strValue.Find(',',0); 
		if(dividerPos > 0)
		{
			for(int j = 0; j < iLenght; j++)
			{
				AfxExtractSubString(SingleItem, strValue, j ,  cDivider);
				if(!SingleItem.IsEmpty())
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_OFF_Max[i][j] = atoi(SingleItem);
				}
				else
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_OFF_Max[i][j] = iBasicMax_off;
				}
			}
		}
		else
		{
			for(int j = 0; j < iLenght; j++)
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_OFF_Max[i][j] = iBasicMax_off;
			}
		}
	}

	for(int i = 0; i < 2; i++)
	{
		if(0 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxRxNum;
		if(1 == i)iLenght = g_ScreenSetParam_cap_sensors.iUsedMaxTxNum;

		str.Format("ScapCB_OFF_Min_%d", i + 1);
		GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp.GetBuffer(BUFFER_LENGTH),BUFFER_LENGTH, strIniFile);
		strValue.Format("%s",strTemp);		
		dividerPos = strValue.Find(',',0); 
		if(dividerPos > 0)
		{
			for(int j = 0; j < iLenght; j++)
			{
				AfxExtractSubString(SingleItem, strValue, j ,  cDivider);
				if(!SingleItem.IsEmpty())
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_OFF_Min[i][j] = atoi(SingleItem);
				}
				else
				{
					g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_OFF_Min[i][j] = iBasicMin_off;
				}
			}
		}
		else
		{
			for(int j = 0; j < iLenght; j++)
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.SCapCbTest_OFF_Min[i][j] = iBasicMin_off;
			}
		}
	}
*/

}

void OnInit_DThreshold_RxLinearityTest_cap_sensors(char *strIniFile)
{
	char str[128], strTemp[MAX_PATH],strValue[MAX_PATH];
	int MaxValue = 0;
	int   dividerPos=0; 
	char str_tmp[128];
	int index = 0;
	int  k = 0, i = 0, j = 0;
	////////////////////////////Rx_Linearity Test
	GetPrivateProfileString_cap_sensors( "Basic_Threshold","RxLinearityTest_Max", "50",str, strIniFile);
	MaxValue = atoi(str);

	//printk("MaxValue = %d  \n", MaxValue);
		
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.RxLinearityTest_Max[i][j] = MaxValue;
		}
	}
	
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		sprintf(str, "Rx_Linearity_Max_Tx%d", (i + 1));
		//printk("%s \n", str);
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "111",strTemp, strIniFile);
		//printk("GetPrivateProfileString_cap_sensors = %d \n", dividerPos);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;		
		memset(str_tmp, 0, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.RxLinearityTest_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}
		
	}
}

void OnInit_DThreshold_TxLinearityTest_cap_sensors(char *strIniFile)
{
	char str[128], strTemp[MAX_PATH],strValue[MAX_PATH];
	int MaxValue = 0;
	int   dividerPos=0; 
	char str_tmp[128];
	int index = 0;
	int  k = 0, i = 0, j = 0;
	////////////////////////////Tx_Linearity Test
	GetPrivateProfileString_cap_sensors( "Basic_Threshold","TxLinearityTest_Max", "50",str, strIniFile);
	MaxValue = atoi(str);

	//printk("MaxValue = %d  \n", MaxValue);
		
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.TxLinearityTest_Max[i][j] = MaxValue;
		}
	}
	
	for(i = 0; i < g_ScreenSetParam_cap_sensors.iUsedMaxTxNum; i++)
	{
		sprintf(str, "Tx_Linearity_Max_Tx%d", (i + 1));
		//printk("%s \n", str);
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "111",strTemp, strIniFile);
		//printk("GetPrivateProfileString_cap_sensors = %d \n", dividerPos);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;		
		memset(str_tmp, 0, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.TxLinearityTest_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}
		
	}
}

/*void OnInit_DThreshold_ForceTouch_SCapRawDataTest(char *strIniFile)
{
	char str[128], strTemp[MAX_PATH],strValue[MAX_PATH];
	int MaxValue, MinValue;
	int   dividerPos=0; 
	char str_tmp[128];
	int index = 0;
	int  k = 0, i = 0, j = 0;

	//////////////////OFF
	GetPrivateProfileString_cap_sensors("Basic_Threshold","ForceTouch_SCapRawDataTest_OFF_Min","150",str,strIniFile);
	MinValue = atoi(str);
	GetPrivateProfileString_cap_sensors("Basic_Threshold","ForceTouch_SCapRawDataTest_OFF_Max","1000",str,strIniFile);
	MaxValue = atoi(str);
	
	///Max
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapRawDataTest_OFF_Max[i][j] = MaxValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ForceTouch_ScapRawData_OFF_Max_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapRawDataTest_OFF_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}
	////Min
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapRawDataTest_OFF_Min[i][j] = MinValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ForceTouch_ScapRawData_OFF_Min_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapRawDataTest_OFF_Min[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}	

	//////////////////ON
	GetPrivateProfileString_cap_sensors("Basic_Threshold","ForceTouch_SCapRawDataTest_ON_Min","150",str,strIniFile);
	MinValue = atoi(str);
	GetPrivateProfileString_cap_sensors("Basic_Threshold","ForceTouch_SCapRawDataTest_ON_Max","1000",str,strIniFile);
	MaxValue = atoi(str);

//printk("%d:%d\r\n",MinValue, MaxValue);
	//////读取阈值，若无特殊设置，则以Basic_Threshold替代
	
	///Max
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapRawDataTest_ON_Max[i][j] = MaxValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ForceTouch_ScapRawData_ON_Max_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);//printk("%s:%s\r\n",str, strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapRawDataTest_ON_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}
	////Min
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapRawDataTest_ON_Min[i][j] = MinValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ForceTouch_ScapRawData_ON_Min_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);//printk("%s:%s\r\n",str, strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapRawDataTest_ON_Min[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}

}

void OnInit_DThreshold_ForceTouch_SCapCbTest(char *strIniFile)
{
	char str[128], strTemp[MAX_PATH],strValue[MAX_PATH];
	int MaxValue, MinValue;
	int   dividerPos=0; 
	char str_tmp[128];
	int index = 0;
	int  k = 0, i = 0, j = 0;

	GetPrivateProfileString_cap_sensors("Basic_Threshold","ForceTouch_SCapCbTest_ON_Min","0",str,strIniFile);
	MinValue = atoi(str);
	GetPrivateProfileString_cap_sensors("Basic_Threshold","ForceTouch_SCapCbTest_ON_Max","240",str,strIniFile);
	MaxValue = atoi(str);

	//printk("%d:%d\r\n",MinValue, MaxValue);
	//load threshold is not specific that will basic threshold instead.
	
	///Max
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapCbTest_ON_Max[i][j] = MaxValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ForceTouch_ScapCB_ON_Max_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);//printk("%s:%s\r\n",str, strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapCbTest_ON_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}
	////Min
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapCbTest_ON_Min[i][j] = MinValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ForceTouch_ScapCB_ON_Min_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);//printk("%s:%s\r\n",str, strTemp);
		if(0 == dividerPos) continue;
		index = 0;printk("%s\r\n",strTemp);
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapCbTest_ON_Min[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}
	GetPrivateProfileString_cap_sensors("Basic_Threshold","ForceTouch_SCapCbTest_OFF_Min","0",str,strIniFile);
	MinValue = atoi(str);
	GetPrivateProfileString_cap_sensors("Basic_Threshold","ForceTouch_SCapCbTest_OFF_Max","240",str,strIniFile);
	MaxValue = atoi(str);
	///Max
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapCbTest_OFF_Max[i][j] = MaxValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ForceTouch_ScapCB_OFF_Max_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapCbTest_OFF_Max[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}
	////Min
	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < g_ScreenSetParam_cap_sensors.iUsedMaxRxNum; j++)
		{
			g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapCbTest_OFF_Min[i][j] = MinValue;
		}
	}
	for(i = 0; i < 2; i++)
	{
		sprintf(str, "ForceTouch_ScapCB_OFF_Min_%d", (i + 1));
		dividerPos = GetPrivateProfileString_cap_sensors( "SpecialSet", str, "NULL",strTemp, strIniFile);
		sprintf(strValue, "%s",strTemp);
		if(0 == dividerPos) continue;
		index = 0;
		k = 0;
		memset(str_tmp, 0x00, sizeof(str_tmp));
		for(j=0; j<dividerPos; j++) 
		{
			if(',' == strValue[j]) 
			{
				g_stCfg_MCap_DetailThreshold_cap_sensors.ForceTouch_SCapCbTest_OFF_Min[i][k] = (short)(atoi(str_tmp));
				index = 0;
				memset(str_tmp, 0x00, sizeof(str_tmp));
				k++;
			} 
			else 
			{
				if(' ' == strValue[j])
					continue;
				str_tmp[index] = strValue[j];
				index++;
			}
		}		
	}	

}*/

