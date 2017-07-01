#include "rumba_interface.h"
#include "rumba_i2c.h"
#include "utils.h"

#undef  pr_fmt
#define pr_fmt(fmt) " [sz_cam_ois] RUMBA %s() " fmt, __func__

static bool rumba_is_idle_state(struct msm_ois_ctrl_t * ctrl);


static int rumba_is_servo_on(struct msm_ois_ctrl_t * ctrl);
static int rumba_servo_on(struct msm_ois_ctrl_t * ctrl);
static int rumba_servo_off(struct msm_ois_ctrl_t * ctrl);

static int rumba_wait_value_with_timeout(struct msm_ois_ctrl_t * ctrl, uint16_t reg_addr, uint16_t mask, uint16_t expected_value, uint32_t timeout_value, const char* func_name);

//static int rumba_sw_reset(struct msm_ois_ctrl_t * ctrl);

static int rumba_switch_mode_internal(struct msm_ois_ctrl_t *ctrl, uint8_t mode);

int rumba_is_servo_on(struct msm_ois_ctrl_t * ctrl)
{
	uint16_t reg_data = 0xeeee;
	rumba_read_byte(ctrl,0x0000,&reg_data);
	return (reg_data == 0x01);
}

int rumba_servo_on(struct msm_ois_ctrl_t * ctrl)
{
	int rc;
	rc = rumba_write_byte(ctrl,0x0000,1);
	delay_ms(10);
	return rc;
}
int rumba_servo_off(struct msm_ois_ctrl_t * ctrl)
{
	int rc = rumba_write_byte(ctrl,0x0000,0);
	delay_ms(10);
	return rc;
}

int rumba_servo_go_on(struct msm_ois_ctrl_t * ctrl)
{
	if(!rumba_is_servo_on(ctrl))
	{
		return rumba_servo_on(ctrl);
	}
	return 0;
}


int rumba_servo_go_off(struct msm_ois_ctrl_t * ctrl)
{
	if(rumba_is_servo_on(ctrl))
	{
		return rumba_servo_off(ctrl);
	}
	return 0;
}


int rumba_get_servo_state(struct msm_ois_ctrl_t * ctrl, int *state)
{
	if(rumba_is_servo_on(ctrl))
	{
		*state = 1;
	}
	else
	{
		*state = 0;
	}

	return 0;
}

int rumba_restore_servo_state(struct msm_ois_ctrl_t * ctrl, int state)
{
	int rc;
	int current_state = -1;
	uint16_t val;

	rc = rumba_get_servo_state(ctrl,&current_state);
	if(current_state != state)
	{
		val = (state?0x1:0x0);
		rc = rumba_write_byte(ctrl,0x0000,val);
		delay_ms(10);
	}
	else
	{
		//pr_info("current state %d equal to new state, not set command\n",current_state);
		rc = 0;
	}
	return rc;
}

int rumba_switch_mode(struct msm_ois_ctrl_t *ctrl, uint8_t mode)
{
	int rc = 0;
	uint8_t internal_mode = 0x05;
	switch(mode)
	{
		case 0:
			internal_mode = 0x05;//centering mode
			break;
		case 1:
			internal_mode = 0x10;//preivew & video mode
			break;
		case 2:
			internal_mode = 0x11;//capture mode
			break;
		case 3:
			internal_mode = 0x12;//factory test mode
			break;
		case 4:
			internal_mode = 0x13;//backup mode
			break;
		default:
			pr_err("Not supported ois mode %d\n",mode);
			rc = -1;
	}
	if(rc == 0)
	{
		pr_info("Mode changed to 0x%x\n",internal_mode);
#ifdef ASUS_FACTORY_BUILD
		rc = rumba_servo_go_off(ctrl);
#else
		//pr_info("Shipping mode, not turn ois off to switch mode\n");
#endif
		rc = rumba_switch_mode_internal(ctrl,internal_mode);
#ifdef ASUS_FACTORY_BUILD
		rc = rumba_servo_on(ctrl);//OIS on
#else
		//pr_info("Shipping mode, not turn ois on to switch mode\n");
#endif
	}
	return rc;
}

static int rumba_switch_mode_internal(struct msm_ois_ctrl_t *ctrl, uint8_t mode)
{
	int rc;
	uint16_t reg_data = 0xeeee;
	if( (mode >=2 && mode <=7) || (mode >= 16 && mode <=19) )
	{
		rc = rumba_write_byte(ctrl,0x0002,(uint16_t)mode);
		delay_ms(20);
		if(rc == 0)
		{
			rc = rumba_read_byte(ctrl,0x0002,&reg_data);
			if(reg_data == mode)
			{
				//pr_info("Read back mode is the same with setting mode %d\n",mode);
			}
			else
			{
				pr_err("Read back mode %d not same with setting mode %d\n",reg_data,mode);
			}
		}
	}
	else
	{
		pr_err("invalid mode %d for rumba-s4a, valid value is [2,7],[16,19]\n",mode);
		rc = -1;
	}
	return rc;
}

int rumba_set_i2c_mode(struct msm_ois_ctrl_t *ctrl,enum i2c_freq_mode_t mode)
{
	int rc;
	uint16_t reg_data = 0xeeee;
	uint16_t verify_data = 0xeeee;
	switch(mode)
	{
		case I2C_STANDARD_MODE:
			 reg_data = 0x2;
			 break;
		case I2C_FAST_MODE:
			 reg_data = 0x0;
			 break;
		case I2C_FAST_PLUS_MODE:
			 reg_data = 0x1;
			 break;
		default:
			 pr_err("not supported i2c mode %d, set to FAST MODE",mode);
			 reg_data = 0x0;//default set to 0x0
			 break;
	}
	rc = rumba_write_byte(ctrl,0x03FD,reg_data);
	if(rc == 0)
	{
		rc = rumba_read_byte(ctrl,0x03FD,&verify_data);
		if(verify_data == reg_data)
		{
			pr_info("Set I2C Mode Succeeded!\n");
			rc = 0;
		}
		else
		{
			pr_err("read back i2c mode 0x%x not equal with set i2c mode 0x%x\n",
					verify_data,reg_data
			);
			rc = -1;
		}
	}
	return rc;
}

int rumba_backup_ois_data_to_file(struct msm_ois_ctrl_t * ctrl, char * file)
{
	bool rc = false;
	uint8_t * pOisData;
	pOisData = kzalloc(sizeof(uint8_t)*1024,GFP_KERNEL);
	if (!pOisData)
	{
		pr_err("no memory!\n");
		return -1;
	}
	rc =rumba_read_seq_bytes(ctrl,0x0200,pOisData,1024);
	if(rc == 0)
		rc = sysfs_write_byte_seq(file,pOisData,1024);
	else
		pr_err("read 0x0200 ois data failed!\n");

	kfree(pOisData);
	if(rc == true)
	{
		rc = 0;
	}
	else
	{
		rc = -2;
	}
	return rc;
}

int rumba_restore_ois_data_from_file(struct msm_ois_ctrl_t * ctrl, char * file)
{
	int rc;
	uint8_t * pOisData;
	int old_state;
	pOisData = kzalloc(sizeof(uint8_t)*1024,GFP_KERNEL);//scanf(%x) require to be uint32
	if (!pOisData)
	{
		pr_err("no memory!\n");
		return -1;
	}

	rc = sysfs_read_byte_seq(file,pOisData,1024);

	if(rc > 0)
	{
	/*
	index[124], reg 0x27c changed, 0x0 -> 0x94
	index[125], reg 0x27d changed, 0x0 -> 0xbf
	index[126], reg 0x27e changed, 0x2 -> 0x3
	index[148], reg 0x294 changed, 0x0 -> 0x8e
	index[149], reg 0x295 changed, 0x50 -> 0x34
	index[150], reg 0x296 changed, 0x1 -> 0x3
	*/

	/*
	 index[14], reg 0x20e changed, 0x3d -> 0x3a
	 index[15], reg 0x20f changed, 0x40 -> 0x41
	index[16], reg 0x210 changed, 0x6f -> 0x70
	 index[17], reg 0x211 changed, 0x6a -> 0x63
	index[18], reg 0x212 changed, 0x3e -> 0x45
	 index[20], reg 0x214 changed, 0xa2 -> 0xb4
	index[22], reg 0x216 changed, 0x20 -> 0x10
	index[24], reg 0x218 changed, 0x85 -> 0x83
	 index[26], reg 0x21a changed, 0x0 -> 0xfc
	 index[27], reg 0x21b changed, 0x8 -> 0x7
	 index[28], reg 0x21c changed, 0x0 -> 0xc9
	  index[29], reg 0x21d changed, 0x8 -> 0x7
	*/
#if 0
		//X9B
		rc = rumba_write_seq_bytes(ctrl,0x027C,pOisData+124,4);
		//Y9B
		if(rc == 0)
			rc = rumba_write_seq_bytes(ctrl,0x0294,pOisData+148,4);

		//14~29
		if(rc == 0)
			rc = rumba_write_seq_bytes(ctrl,0x020E,pOisData+14,16);
#else
		rc = rumba_write_seq_bytes(ctrl,0x0200,pOisData,1024);//some registers can not be written...changed after power up
#endif
		if(rc == 0)
		{
			rumba_get_servo_state(ctrl,&old_state);
			rumba_servo_go_off(ctrl);
			rc = rumba_check_flash_write_result(ctrl,OIS_DATA);
			rumba_restore_servo_state(ctrl,old_state);
		}
	}
	else
	{
		pr_err("Read ois data from %s failed, rc = %d\n",file,rc);
	}

	kfree(pOisData);

	return rc;
}

int rumba_backup_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_addr, uint32_t size, uint8_t *buffer)
{
	return rumba_read_seq_bytes(ctrl,reg_addr,buffer,size);
}

int rumba_compare_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_addr, uint32_t size, uint8_t *buffer)
{
	uint8_t * pNewData = NULL;
	uint8_t * pOldData = buffer;
	int i;
	int rc = 1;
	pNewData = kzalloc(sizeof(uint8_t)*size,GFP_KERNEL);
	if (!pNewData)
	{
		pr_err("no memory!\n");
		return -ENOMEM;
	}

	rumba_read_seq_bytes(ctrl,reg_addr,pNewData,size);

	for(i=0;i<size;i++)
	{
		if(pNewData[i] != pOldData[i])
		{
			pr_info("index[%d], reg 0x%04x changed, 0x%02x -> 0x%02x\n",
			i, reg_addr+i,
			pOldData[i],pNewData[i]
			);
			rc = 0;
		}
	}

	kfree(pNewData);

	return rc;
}
int rumba_gyro_adjust_gain(struct msm_ois_ctrl_t * ctrl)
{
	int rc;
	int index = 0;
	static uint32_t float_value_integer_map[] =
	{
		0x3f800000,//1.0f
		0x3fc00000,//1.5f
		0x40000000,//2.0f
		0x40200000,//2.5f
		0x40400000,//3.0f
		0x40600000,//3.5f
		0x40800000,//4.0f
		0x40900000,//4.5f
		0x40a00000,//5.0f
		0x40b00000,//5.5f
		0x40c00000,//6.0f
		0x40d00000,//6.5f
		0x40e00000,//7.0f
		0x40f00000//7.5f
	};

	static uint32_t integer_values[14];
	/*OIS On*/
	rc = rumba_write_byte(ctrl,0x0002,0x10);//Mode 0

	rc = rumba_write_byte(ctrl,0x0000,0x01);//OIS EN set

	//Gyro Gain Tunning X
	for(index=0;index<sizeof(float_value_integer_map)/sizeof(uint32_t);index++)
	{
		rc = rumba_write_dword(ctrl,0x0254,float_value_integer_map[index]);//XGG register, uint32 as float
		rc = rumba_write_byte(ctrl,0x0015,0x10);//GGA CTRL XGGEN=1
		delay_ms(500);

		rumba_read_dword(ctrl,0x0254,&integer_values[index]);
		/*
		if()//WHAT EXPRESSION ?
		{
			break;
		}
		*/
	}
#if 0
	for(index = 0;index < 14;index++)
	{
		pr_info("read dword value is 0x%x\n",integer_values[index]);
	}
#endif
	//Gyro Gain Tunning Y
	for(index=0;index<sizeof(float_value_integer_map)/sizeof(uint32_t);index++)
	{
		rc = rumba_write_dword(ctrl,0x0258,float_value_integer_map[index]);//XGG register, uint32 as float
		rc = rumba_write_byte(ctrl,0x0015,0x20);//GGA CTRL YGGEN=1
		delay_ms(500);
		/*
		if()//WHAT EXPRESSION ?
		{
			break;
		}
		*/
	}
	/*Lens Cotrol End*/

	rc = rumba_write_byte(ctrl,0x0000,0x00);//OIS CTRL OIS EN clear

	return rc;

}
int rumba_gyro_read_data(struct msm_ois_ctrl_t * ctrl,uint16_t* gyro_data, uint32_t size)
{
	int rc;
	int old_servo_state;
	uint16_t gyro_x_config,gyro_y_config;
	uint16_t gyro_x_value,gyro_y_value;
	uint16_t gyro_min,gyro_max,gyro_x_K,gyro_y_K;

	uint16_t acc_x,acc_y;
	if(size<10)
	{
		pr_err("size %d not enough to hold gyro data\n",size);
		return -1;
	}

	rumba_get_servo_state(ctrl,&old_servo_state);

	rumba_servo_go_on(ctrl);

	rumba_fw_info_enable(ctrl);

	rumba_read_word(ctrl,0x0082,&gyro_x_value);
	rumba_read_word(ctrl,0x0084,&gyro_y_value);

	rumba_read_acc_position(ctrl,&acc_x,&acc_y);

	rumba_fw_info_disable(ctrl);

	rumba_servo_off(ctrl);

	if(!rumba_is_idle_state(ctrl))
		return -2;

	rc = rumba_write_byte(ctrl,0x00F5,0x00);//gyro sensor register address set
	rc = rumba_write_byte(ctrl,0x00F4,0x01);//gyro sensor register read start
	rc = rumba_wait_value_with_timeout(ctrl,0x00F4,0x01,0,10000,__func__);
	if(rc < 0)
	{
		pr_err("Read Gyro Sensor Register X Config Data Fail!\n");
		return -3;
	}

	if(rc >= 0)
	{
		rc = rumba_read_byte(ctrl,0x00F6,&gyro_x_config);
		if(rc >=0)
		{
			pr_info("Got gyro x config data 0x%x\n",gyro_x_config);
		}
	}

	rc = rumba_write_byte(ctrl,0x00F5,0x01);//gyro sensor register address set
	rc = rumba_write_byte(ctrl,0x00F4,0x01);//gyro sensor register read start
	rc = rumba_wait_value_with_timeout(ctrl,0x00F4,0x01,0,10000,__func__);
	if(rc < 0)
	{
		pr_err("Read Gyro Sensor Register Y Config Data Fail!\n");
		return -4;
	}

	if(rc >= 0)
	{
		rc = rumba_read_byte(ctrl,0x00F6,&gyro_y_config);
		if(rc >=0)
		{
			pr_info("Got gyro y config data 0x%x\n",gyro_y_config);
		}
	}

	rumba_read_word(ctrl,0x0244,&gyro_max);
	rumba_read_word(ctrl,0x0246,&gyro_min);
	rumba_read_word(ctrl,0x0248,&gyro_x_K);
	rumba_read_word(ctrl,0x024A,&gyro_y_K);

	pr_info("Got gyro value(%d, %d), min %d, max %d, x_K %d, y_K %d, config(0x%x, 0x%x)\n",
			(int16_t)gyro_x_value,(int16_t)gyro_y_value,
			(int16_t)gyro_min,(int16_t)gyro_max,
			(int16_t)gyro_x_K,(int16_t)gyro_y_K,
			gyro_x_config,gyro_x_config
			);
	gyro_data[0] = gyro_x_value;
	gyro_data[1] = gyro_y_value;
	gyro_data[2] = gyro_min;
	gyro_data[3] = gyro_max;
	gyro_data[4] = gyro_x_K;
	gyro_data[5] = gyro_y_K;
	gyro_data[6] = gyro_x_config;
	gyro_data[7] = gyro_y_config;

	gyro_data[8] = acc_x;
	gyro_data[9] = acc_y;

	rumba_restore_servo_state(ctrl,old_servo_state);

	return rc;
}

int rumba_gyro_read_K_related_data(struct msm_ois_ctrl_t * ctrl,uint16_t* gyro_data, uint32_t size)
{
	int rc;
	int old_servo_state;

	uint16_t gyro_x_K,gyro_y_K;

	if(size<2)
	{
		pr_err("size %d not enough to carry gyro data, should be 2\n",size);
		return -1;
	}

	rumba_get_servo_state(ctrl,&old_servo_state);

	rc = rumba_read_word(ctrl,0x0248,&gyro_x_K);
	if(rc == 0)
		rc = rumba_read_word(ctrl,0x024A,&gyro_y_K);

	rumba_restore_servo_state(ctrl,old_servo_state);

	gyro_data[0] = gyro_x_K;
	gyro_data[1] = gyro_y_K;

	return rc;
}

int rumba_fw_info_enable(struct msm_ois_ctrl_t * ctrl)
{
	int rc = rumba_write_byte(ctrl,0x0080,0x01);//FWINFO_CTRL Enable
	delay_ms(2);
	return rc;
}
int rumba_fw_info_disable(struct msm_ois_ctrl_t * ctrl)
{
	int rc = rumba_write_byte(ctrl,0x0080,0x00);//FWINFO_CTRL Disable
	delay_ms(2);
	return rc;
}
int rumba_gyro_read_xy(struct msm_ois_ctrl_t * ctrl,uint16_t *x_value, uint16_t *y_value)
{
	int rc;

	rc = rumba_read_word(ctrl,0x0082,x_value);
	if(rc == 0)
		rc = rumba_read_word(ctrl,0x0084,y_value);
	return rc;
}
int rumba_read_acc_position(struct msm_ois_ctrl_t * ctrl,uint16_t *x_value, uint16_t *y_value)
{
	int rc;

	rc = rumba_read_word(ctrl,0x044C,x_value);
	if(rc == 0)
		rc = rumba_read_word(ctrl,0x044E,y_value);

	return rc;
}
int rumba_read_pair_sensor_data(struct msm_ois_ctrl_t * ctrl,
								 uint16_t reg_addr_x,uint16_t reg_addr_y,
								 uint16_t *value_x,uint16_t *value_y)
{

	int rc;
	rc = rumba_read_word(ctrl,reg_addr_x,value_x);
	if(rc == 0)
		rc = rumba_read_word(ctrl,reg_addr_y,value_y);
	return rc;
}
int rumba_gyro_calibration(struct msm_ois_ctrl_t * ctrl)
{
	uint16_t reg_data = 0xeeee;
	int rc;
	uint32_t fw_version;
	uint16_t gyro_value_max,gyro_value_min;
	uint16_t gyro_value_x,gyro_value_y;
	int i;
	//rumba_sw_reset(ctrl);Timeout...

	if(!rumba_is_idle_state(ctrl))
		return -1;

	rumba_read_dword(ctrl,0x00FC,&fw_version);
	rumba_read_word(ctrl,0x0244,&gyro_value_max);
	rumba_read_word(ctrl,0x0246,&gyro_value_min);
	pr_info("FW rev %d, gyro valid value range is [%d %d]",
			fw_version,
			(int16_t)gyro_value_min, (int16_t)gyro_value_max
			);
	//rumba_gyro_communication_check(ctrl);

	rumba_write_byte(ctrl,0x0014,0x0001);//GCCTRL GSCEN set, calibration start

	rc = rumba_wait_value_with_timeout(ctrl,0x0014,0xff,0,10000,__func__);
	if(rc < 0)
	{
		pr_err("Calibration Sequence Not End !?\n");
		return -2;
	}

	/* Result check */
	rumba_read_byte(ctrl,0x0004,&reg_data);
	/* OISERR Read */
	if( (reg_data & 0x23) == 0x0)// 1 0 0 0 1 1
	/* OISERR register GXZEROERR & GYZEROERR & GCOMERR Bit = 0(No Error) */
	{
		pr_info("Gyro Calibration Complete!\n");
		/* Write Gyro Calibration result to OIS DATA SECTION */
		rc = rumba_check_flash_write_result(ctrl,OIS_DATA);
	}
	else
	{
		pr_err("Gyro Calibration Failed, reg 0x0004 val is 0x%x\n",reg_data);
		// x x x x x x 1 1
		if((reg_data & 0x03) != 0)
		{
			pr_err("Gyro Output value not valid!\n");
			rumba_fw_info_enable(ctrl);
			for(i=0;i<10;i++)
			{
				rumba_read_word(ctrl,0x0082,&gyro_value_x);
				rumba_read_word(ctrl,0x0084,&gyro_value_y);
				pr_info("check %d time, gyro x,y value [%d %d]\n",i,(int16_t)gyro_value_x,(int16_t)gyro_value_y);
		    }
			rumba_fw_info_disable(ctrl);
		}
		rc = -1;
	}
	//rumba_idle2standy_state(ctrl);
	//rumba_standy2idle_state(ctrl);
	return  rc;

}

int rumba_hall_check_polarity(struct msm_ois_ctrl_t * ctrl)
{
	int rc;
	uint16_t reg_data = 0xeeee;

	if(!rumba_is_idle_state(ctrl))
		return -1;

	rc = rumba_write_byte(ctrl,0x0012,0x01);/* HPCTRL HPEN set */

	rc = rumba_wait_value_with_timeout(ctrl,0x0012,0xff,0,10000,__func__);
	if(rc < 0)
	{
		pr_err("Hall Check Polarity Not End ?!\n");
		return rc;
	}

	rc = rumba_read_byte(ctrl,0x0200,&reg_data);

	if( (reg_data & 0x0C ) == 0x0 )
	{
		pr_err("Hall check Polarity Complete!\n");
		rc = rumba_check_flash_write_result(ctrl,OIS_DATA);
	}
	else
	{
		pr_err("Hall check Polarity Failed!\n");
		rc = -1;
	}
	return rc;

}

int rumba_hall_calibration(struct msm_ois_ctrl_t * ctrl)
{
	int rc;
	uint16_t reg_data = 0xeeee;

	if(!rumba_is_idle_state(ctrl))
		return -1;

	rc = rumba_write_byte(ctrl,0x0013,0x01);/* HCCTRL HCEN set */

	rc = rumba_wait_value_with_timeout(ctrl,0x0013,0xff,0,10000,__func__);
	if(rc < 0)
	{
		pr_err("Hall Calibration Not End ?!\n");
		return rc;
	}

	rc = rumba_read_byte(ctrl,0x04,&reg_data);/* OISERR Read */

	if( (reg_data & 0x0c) == 0x0 )
	{
		pr_info("Hall Calibration Complete!\n");
		rc = rumba_check_flash_write_result(ctrl,OIS_DATA);
	}
	else
	{
		pr_info("Hall Calibration Failed! reg_data 0x%x\n",reg_data);
		rc = -1;
		//Todo Change Hall Parameter
	}

	return rc;
}

int rumba_check_flash_write_result(struct msm_ois_ctrl_t * ctrl,flash_write_data_type type)
{
	uint16_t reg_data = 0xeeee;
	int rc;

	if(type != OIS_DATA && type != PID_PARAM && type != USER_DATA)
	{
		pr_err("Not supported check type %d!\n",type);
		return -1;
	}

	/* Set Result Check Data */
	rc = rumba_write_byte(ctrl,0x0027,0xAA);


	switch(type)
	{
		case OIS_DATA:
			/* Exec OIS DATA AREA Write */
			rc = rumba_write_byte(ctrl,0x0003,0x01);/* set OIS_W = 1 */
			delay_ms(170);/* Wait for Flash ROM Write */
			rc = rumba_wait_value_with_timeout(ctrl,0x0003,0xff,0,10000,__func__);
			if(rc < 0)
			{
				pr_err("Flash ROM Write Not End?!\n");
				return -1;
			}
			break;

		case PID_PARAM:
			/* Exec PID parameter Init */
			rc = rumba_write_byte(ctrl,0x0036,0x01);/* PIDPARAMINIT INIT_EN set */
			delay_ms(170); /* Wait for Flash ROM Write */
			rc = rumba_wait_value_with_timeout(ctrl,0x0036,0xff,0,10000,__func__);
			if(rc < 0)
			{
				pr_err("Flash ROM Write Not End?!\n");
				return -1;
			}
			break;

		case USER_DATA:
			/* Exec User Data Area Write */
			rc = rumba_write_byte(ctrl,0x000E,0x07);
			delay_ms(170); /* Wait for Flash ROM Write */
			rc = rumba_wait_value_with_timeout(ctrl,0x000E,0xff,0x17,10000,__func__);
			if(rc < 0)
			{
				pr_err("Flash ROM Write Not End?!\n");
				return -1;
			}
			break;

		default:
			pr_err("Not supported check type %d!\n",type);
	}

	rc = rumba_read_byte(ctrl,0x0027,&reg_data);

	if(reg_data != 0x00AA )
	{
		/* Flash Write Error */
		pr_err("Flash Write FAIL!\n");
		rc = -1;
	}
	else
	{
		/* Flash Write Success */
		pr_info("Flash Write OK!\n");
		rc = 0;
	}
	return rc;
}

int rumba_gyro_communication_check(struct msm_ois_ctrl_t * ctrl)
{
	uint16_t reg_data = 0xeeee;
	int rc;

	if(!rumba_is_idle_state(ctrl))
		return -1;

	/* Gyro Calibration Start */
	rumba_write_byte(ctrl,0x0014,0x02);//GCCTRL GSCEN set, calibration start

	rc = rumba_wait_value_with_timeout(ctrl,0x0014,0xff,0,10000,__func__);
	if(rc < 0)
	{
		pr_err("Calibration Sequence Not End ?\n");
		return rc;
	}

	rc = rumba_read_byte(ctrl,0x0004,&reg_data);
	if( (reg_data & 0x20) != 0x0 )
	{
		pr_err("Communication error!\n");
		rc = -1;
	}
	else
	{
		pr_info("Communication OK!\n");
		rc = 0;
	}
	return rc;
	/* OISERR register GCOMERR Bit != 0(Communication Error Found!!) */
}
#if 0
static int rumba_sw_reset(struct msm_ois_ctrl_t * ctrl)
{
	int rc = 0;

	/* RUMBA SW Reset Sequence */
	rumba_write_byte(ctrl,0x000D,0x01);

	rc = rumba_wait_value_with_timeout(ctrl,0x0001,0xff,0x9,10000,__func__);
	if(rc < 0)
	{
		pr_err("OISSTS State not 0x9 ?\n");
		return rc;
	}

	/* 0x9 == DFLS_UPDATE STATE */

	/* DFLSCMD=6 (Reset) */
	rumba_write_byte(ctrl,0x000E,0x06);

	delay_ms(20);

	return rc;
}
#endif
int rumba_measure_loop_gain(struct msm_ois_ctrl_t * ctrl)
{
	int rc;

	uint16_t read_data_short[4];
	uint8_t  read_data_char[8];
	if(!rumba_is_idle_state(ctrl))
	{
		return -1;
	}
	/*set setting parameter(example of configuration)*/
	rc = rumba_write_byte(ctrl,0x00D1,0x64);//LGFREQ set
	rc = rumba_write_byte(ctrl,0x00D2,0x03);//LGNUM set
	rc = rumba_write_byte(ctrl,0x00D3,0x04);//LGSKIPNUM set
	rc = rumba_write_word(ctrl,0x00D4,0x0049);//LGAMP set

	/*X-axis loop gain measurement start*/
	rc = rumba_write_byte(ctrl,0x00D0,0x01);//LGCTRL LGSEL(0:X-axis) and LGEN set

	/*check end*/
	rc = rumba_wait_value_with_timeout(ctrl,0x00D0,0xff,0x0,10000,__func__);
	if(rc == 0)
	{
		/*check result*/
		rc = rumba_read_seq_bytes(ctrl,0x00C0,read_data_char,8);//LGMON1AMP,LGMON2AMP,LGMON1PHASE,LGMON2PHASE

		read_data_short[0] = (uint16_t)(read_data_char[1] << 8) | read_data_char[0];
		read_data_short[1] = (uint16_t)(read_data_char[3] << 8) | read_data_char[2];
		read_data_short[2] = (uint16_t)(read_data_char[5] << 8) | read_data_char[4];
		read_data_short[3] = (uint16_t)(read_data_char[7] << 8) | read_data_char[6];

		pr_info("X loop gain measure, got 0x%x 0x%x 0x%x 0x%x\n",
			read_data_short[0],
			read_data_short[1],
			read_data_short[2],
			read_data_short[3]
		);
	}
	else
	{
		pr_err("X-axis loop gain measurement NOT END!\n");
		return -2;
	}

	/*Y-axis loop gain measurement start*/
	rc = rumba_write_byte(ctrl,0x00D0,0x11);//LGCTRL LGSEL(1:Y-axis) and LGEN set

	/*check end*/
	rc = rumba_wait_value_with_timeout(ctrl,0x00D0,0xff,0x10,10000,__func__);
	if(rc == 0)
	{
		/*check result*/
		rc = rumba_read_seq_bytes(ctrl,0x00C0,read_data_char,8);

		read_data_short[0] = (uint16_t)(read_data_char[1] << 8) | read_data_char[0];
		read_data_short[1] = (uint16_t)(read_data_char[3] << 8) | read_data_char[2];
		read_data_short[2] = (uint16_t)(read_data_char[5] << 8) | read_data_char[4];
		read_data_short[3] = (uint16_t)(read_data_char[7] << 8) | read_data_char[6];

		pr_info("Y loop gain measure, got 0x%x 0x%x 0x%x 0x%x\n",
			read_data_short[0],
			read_data_short[1],
			read_data_short[2],
			read_data_short[3]
		);
	}
	else
	{
		pr_err("Y-axis loop gain measurement NOT END!\n");
		return -3;
	}
	return rc;
}

int rumba_init_all_params(struct msm_ois_ctrl_t * ctrl)
{
	int rc;
	int old_servo_state;

	rumba_get_servo_state(ctrl,&old_servo_state);

	rumba_servo_go_off(ctrl);

	if(!rumba_is_idle_state(ctrl))
		return -2;

	rc = rumba_write_byte(ctrl,0x0036,0x81);//all params initialization enabled

	delay_ms(170);

	rc = rumba_wait_value_with_timeout(ctrl,0x0036,0xff,0x00,10000,__func__);
	if(rc < 0)
	{
		pr_err("all params initialization not end?!\n");
	}
	else
	{
		pr_info("all params initialization done!\n");
	}

	rumba_restore_servo_state(ctrl,old_servo_state);

	return rc;
}
int rumba_init_PID_params(struct msm_ois_ctrl_t * ctrl)
{
	int rc;
	int old_servo_state;
	uint32_t X9B,Y9B;

	rc = rumba_read_dword(ctrl,0x027C,&X9B);
	rc = rumba_read_dword(ctrl,0x0294,&Y9B);

	if( X9B == 0x20000 && Y9B == 0x15000 )
	{
		pr_info("X9B and Y9B are init values, no need to INIT PID Params\n");
		return 0;
	}
	else
	{
		pr_info("X9B 0x%08x, Y9B 0x%08x, pair are not init values, need to INIT PID Params\n",X9B,Y9B);
	}

	rumba_get_servo_state(ctrl,&old_servo_state);

	rumba_servo_go_off(ctrl);

	if(!rumba_is_idle_state(ctrl))
		return -2;

	rc = rumba_write_byte(ctrl,0x0036,0x01);//PID initialization enabled

	delay_ms(170);

	rc = rumba_wait_value_with_timeout(ctrl,0x0036,0xff,0x00,10000,__func__);
	if(rc < 0)
	{
		pr_err("PID initialization not end?!\n");
	}
	else
	{
		pr_info("PID initialization done!\n");
	}

	rumba_restore_servo_state(ctrl,old_servo_state);

	return rc;
}

int rumba_adjust_loop_gain(struct msm_ois_ctrl_t * ctrl)
{
	int rc;

	if(!rumba_is_idle_state(ctrl))
		return -1;

	/*set setting parameter(example of configuration)*/
	rc = rumba_write_byte(ctrl,0x00D1,0x64);//LGFREQ set
	rc = rumba_write_byte(ctrl,0x00D2,0x03);//LGNUM set
	rc = rumba_write_byte(ctrl,0x00D3,0x04);//LGSKIPNUM set
	rc = rumba_write_word(ctrl,0x00D4,0x0049);//LGAMP set
	rc = rumba_write_dword(ctrl,0x00C8,0x418E432A);//LGTGX set
	rc = rumba_write_dword(ctrl,0x00CC,0x41B33333);//LGTGY set

	//X-axis loop gain adjustment start
	rc = rumba_write_byte(ctrl,0x00D0,0x03);//LGCTRL LGSEL(0:X-axis), LGAEN and LGEN set

	//check end
	rc = rumba_wait_value_with_timeout(ctrl,0x00D0,0xff,0x02,10000,__func__);
	if(rc < 0)
	{
		pr_err("X-axis loop gain adjustment failed!\n");
		rc = -2;
	}
	rc = rumba_write_byte(ctrl,0x00D0,0x13);//LGCTRL LGSEL(1:Y-axis),LGAEN and LGEN set

	rc = rumba_wait_value_with_timeout(ctrl,0x00D0,0xff,0x12,10000,__func__);
	if(rc < 0)
	{
		pr_err("Y-axis loop gain adjustment failed!\n");
	}
	if(rc == 0)
		rc = rumba_check_flash_write_result(ctrl,OIS_DATA);

	return rc;

}



#define STRFORMAT(enum_name) case enum_name: return ""#enum_name"";

const char * rumba_get_state_str(rumba_ois_state state)
{
	switch(state)
	{
		STRFORMAT(INIT_STATE)
		STRFORMAT(IDLE_STATE)
		STRFORMAT(RUN_STATE)
		STRFORMAT(HALL_CALI_STATE)
		STRFORMAT(HALL_POLA_STATE)
		STRFORMAT(GYRO_CALI_STATE)
		STRFORMAT(RESERVED1_STATE)
		STRFORMAT(RESERVED2_STATE)
		STRFORMAT(PWM_DUTY_FIX_STATE)
		STRFORMAT(DFLS_UPDATE_STATE)//user data area update state
		STRFORMAT(STANDBY_STATE)//low power consumption state
		STRFORMAT(GYRO_COMM_CHECK_STATE)
		STRFORMAT(RESERVED3_STATE)
		STRFORMAT(RESERVED4_STATE)
		STRFORMAT(LOOP_GAIN_CTRL_STATE)
		STRFORMAT(RESERVED5_STATE)
		STRFORMAT(GYRO_SELF_TEST_STATE)
		STRFORMAT(OIS_DATA_WRITE_STATE)
		STRFORMAT(PID_PARAM_INIT_STATE)
		STRFORMAT(GYRO_WAKEUP_WAIT_STATE)
		default:
		return "Unknown State";
	}
}

rumba_ois_state rumba_get_state(struct msm_ois_ctrl_t * ctrl)
{
	uint16_t reg_data = 0xeeee;
	int rc;

	/* OIS Status Check */
	rc = rumba_read_byte(ctrl,0x0001,&reg_data);
	if(rc == 0)
		return (rumba_ois_state)reg_data;
	else
		return 0xffff;
}

bool rumba_is_idle_state(struct msm_ois_ctrl_t * ctrl)
{
	uint16_t reg_data = 0xeeee;
	int rc;

	/* OIS Status Check */
	rc = rumba_read_byte(ctrl,0x0001,&reg_data);
	pr_info("rumba state now is %s\n",rumba_get_state_str((rumba_ois_state)reg_data));

	if(rc == 0)
		return reg_data == 0x01;
	else
		return false;
}

int rumba_idle2standy_state(struct msm_ois_ctrl_t * ctrl)
{
	if(!rumba_is_idle_state(ctrl))
		return -1;

	return rumba_write_byte(ctrl,0x0030,0x01);//POWER CTRL STANDBY EN set
}
int rumba_standy2idle_state(struct msm_ois_ctrl_t * ctrl)
{
	return rumba_wait_value_with_timeout(ctrl,0x0001,0xff,1,10000,__func__);
}

static int rumba_wait_value_with_timeout(struct msm_ois_ctrl_t * ctrl, uint16_t reg_addr, uint16_t mask, uint16_t expected_value, uint32_t timeout_value,const char * func_name)
{
	int rc = 0;
	int count = 0;
	uint16_t reg_data;

	do
	{
		count ++;
		if(count > 1)
		{
			delay_ms(1);
		}
		rc = rumba_read_byte(ctrl,reg_addr,&reg_data);
		if(count % 300 == 0)
			pr_info("%s() read reg 0x%04x %d times, reg_data 0x%02x\n",func_name,reg_addr,count,reg_data);

	}while( (reg_data & mask) != expected_value && count < timeout_value && rc >= 0 );

	if((reg_data & mask) == expected_value)
	{
		pr_info("%s() Got expected value 0x%x (mask 0x%x, key_value 0x%x) from reg 0x%04x, times %d\n",
			func_name,
			reg_data,
			mask,
			expected_value,
			reg_addr,
			count
		);
		rc = 0;
	}
	else if(count >= timeout_value)
	{
		pr_err("%s() Time out! Excceed %d ms\n",func_name,timeout_value);
		rc = -1;
	}
	else
	{
		pr_err("%s() I2C is Bad?!\n",func_name);
		rc = -2;
	}
	return rc;
}


int rumba_write_user_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_offset,uint8_t* data, uint32_t size)
{
	uint16_t reg_data = 0xeeee;
	uint8_t * log_buffer = NULL;
	int rc;

	if(reg_offset + size/4 -1 > 0xFF)
	{
		pr_err("offset 0x%x + size %d, target offset reg address 0x%x too large, max offset value is 0xFF!\n",
		reg_offset,
		size,
		reg_offset + size/4);
		return -1;
	}

	if(!rumba_is_idle_state(ctrl))
	{
		return -2;
	}

	rumba_write_byte(ctrl,0x000D,0x01);//Set DFLSEN=1

	if(rumba_get_state(ctrl) != DFLS_UPDATE_STATE)
	{
		pr_err("Enter User Data Area Write Mode Failed! State now is %s\n",
		rumba_get_state_str(rumba_get_state(ctrl))
		);
		rumba_write_byte(ctrl,0x000E,0x05);//END Command Set
		return -3;
	}

	/*User Data Write*/

	rumba_write_byte(ctrl,0x000F,size/4);
	rumba_write_word(ctrl,0x0010,reg_offset);//0x7400~0x74FF
	rumba_write_seq_bytes(ctrl,0x0100,data,size);

	rumba_write_byte(ctrl,0x000E,0x01);//DFLSCMD WRITE

	rumba_read_byte(ctrl,0x000E,&reg_data);

	if(reg_data == 0x11)
	{
		pr_info("Write User Data Complete! offset 0x%x, size %d\n",
			reg_offset,
			size
		);
		rc = rumba_check_flash_write_result(ctrl,USER_DATA);
		if(rc == 0)
		{
			log_buffer = kzalloc(sizeof(uint8_t)*size*8,GFP_KERNEL);
			if (!log_buffer)
			{
				pr_err("no memory!\n");
				rumba_write_byte(ctrl,0x000E,0x05);//END Command Set
				return -ENOMEM;
			}
			if(format_hex_string(log_buffer,sizeof(uint8_t)*size*8,data,size)!= -1)
			{
				pr_info("Write User Data as follows, size %d\n",size);
				pr_info("%s",log_buffer);
			}

			kfree(log_buffer);
		}
	}
	else
	{
		pr_err("Write user data command failed!\n");
		rc = -4;
	}
	rumba_write_byte(ctrl,0x000E,0x05);//END Command Set
	return rc;

}
int rumba_read_user_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_offset,uint8_t* data, uint32_t size)
{
	int rc;
	int old_servo_state;
	//uint16_t reg_data = 0xeeee;
	uint8_t * log_buffer = NULL;
	int i;
	if(reg_offset + size/4 -1 > 0xFF)
	{
		pr_err("offset 0x%x + size %d, target offset reg address 0x%x too large, max offset value is 0xFF!\n",
		reg_offset,
		size,
		reg_offset + size/4);
		return -1;
	}

	rumba_get_servo_state(ctrl,&old_servo_state);

	rumba_servo_go_off(ctrl);


	rumba_write_byte(ctrl,0x000F,size/4);//data read 256 bytes, 64, one unit is 4 bytes
	rumba_write_word(ctrl,0x0010,reg_offset);//data read start address offset,0x7400-0x74ff
	rumba_write_byte(ctrl,0x000E,0x04);//Read command set

	//rumba_read_byte(ctrl,0x000E,&reg_data);//DFLSCMD READ
	rc = rumba_wait_value_with_timeout(ctrl,0x000E,0xff,0x14,10000,__func__);

	if(rc == 0)
	{
		pr_info("Read User Data Complete!\n");
		rumba_read_seq_bytes(ctrl,0x0100,data,size);

		log_buffer = kzalloc(sizeof(uint8_t)*size*8,GFP_KERNEL);
		if (!log_buffer)
		{
			pr_err("no memory!\n");
			return -ENOMEM;
		}
		if(format_hex_string(log_buffer,sizeof(uint8_t)*size*8,data,size)!= -1)
		{
			pr_info("Read User Data as follows, size %d\n",size);
			//pr_info("%s",log_buffer);//can not be too large ....
			for(i=0;i<size;i++)
			{
				pr_info("reg 0x%04x, val 0x%02x\n",reg_offset+i,data[i]);
			}
		}

		kfree(log_buffer);
		rc = 0;

	}
	else
	{
		pr_err("Read user data command failed\n");
		rc = -2;
	}

	rumba_restore_servo_state(ctrl,old_servo_state);

	return rc;
}


static uint32_t get_fw_revision_from_code_data(uint8_t* fw_code_data)
{
	uint32_t revision;
	revision = ((uint32_t)(fw_code_data[0x6FF7] <<24)|
			 (uint32_t)(fw_code_data[0x6FF6] <<16)|
			 (uint32_t)(fw_code_data[0x6FF5] <<8)|
			 (uint32_t)(fw_code_data[0x6FF4] )
			);
	return revision;
}
#if 0
static uint16_t get_check_sum(uint8_t* data, uint16_t size)
{
	uint16_t i;
	uint16_t check_sum;

	check_sum = 0;
	for(i=0;i<size;i++)
	{
		check_sum+=data[i];
	}
	pr_info("check sum of char type is 0x%x %hu, size %hu\n",check_sum,check_sum,size);
	return check_sum;
}
#endif


static uint16_t get_check_sum_short(uint16_t* data, uint16_t size)
{
	uint16_t i;
	uint16_t check_sum;

	check_sum = 0;
	for(i=0;i<size/2;i++)
	{
		check_sum+=data[i];
	}
	pr_info("check sum of short type is 0x%x %hu\n",check_sum,check_sum);
	return check_sum;
}

int rumba_get_bin_fw_revision(const char * file_path, uint32_t* read_fw_rev)
{
	struct kstat stat;
	int rc;

	mm_segment_t fs;
	struct file *fp;
	loff_t pos;
	uint8_t rev_data[4];

	stat.size = 0;

	fp =filp_open(file_path,O_RDONLY,S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR(fp)){
		pr_err("open(%s) failed\n",file_path);
		return -1;
	}

	fs =get_fs();
	set_fs(KERNEL_DS);

	rc = vfs_stat(file_path,&stat);
	if(rc < 0)
	{
		pr_err("vfs_stat(%s) failed, rc = %d\n",file_path,rc);
		rc = -2;
		goto END;
	}

	if(stat.size != 28672)
	{
		pr_err("%s size is not 28672!\n",file_path);
		rc = -3;
		goto END;
	}

	//vfs_llseek(fp, 0x6FF4, SEEK_SET);

	pos = 0x6FF4;
	rc = vfs_read(fp,rev_data,4,&pos);
	if(rc == 4)
	{
		*read_fw_rev = ((uint32_t)(rev_data[3] <<24)|
						(uint32_t)(rev_data[2] <<16)|
						(uint32_t)(rev_data[1] <<8)|
						(uint32_t)(rev_data[0] )
					   );
		pr_info("get bin fw version 0x%x %d\n",*read_fw_rev,*read_fw_rev);
		rc = 0;
	}
	else
	{
		pr_err("get bin fw version failed!\n");
		rc = -4;
		goto END;
	}
END:
	set_fs(fs);
	filp_close(fp,NULL);

	return rc;
}
static int is_fw_type2(uint32_t fw_rev)
{
	uint32_t i;
	uint32_t type2_fw_rev_list[] = {
										16174,
										16039,
										15953,
										15945
								    };
	for(i=0;i<sizeof(type2_fw_rev_list)/sizeof(uint32_t);i++)
	{
		if(fw_rev == type2_fw_rev_list[i])
		{
			return 1;
		}
	}
	return 0;
}

static int is_fw_valid(uint32_t fw_rev)
{
	uint32_t i;
	uint32_t valid_fw_rev_list[] = {
										16174,
										16173,
										16039,
										15964,
										15959,
										15953,
										15945,
										15647,
										15569,
										15456,
										15197,
										14819,
										14490
								   };
	for(i=0;i<sizeof(valid_fw_rev_list)/sizeof(uint32_t);i++)
	{
		if(fw_rev == valid_fw_rev_list[i])
		{
			return 1;
		}
	}
	return 0;
}

int rumba_can_update_fw(uint32_t dev_fw_rev, uint32_t bin_fw_rev, int force_update)
{
	int rc = 1;

	int module_type;
	int fw_type;

	module_type = is_fw_type2(dev_fw_rev)?2:1;
	fw_type = is_fw_type2(bin_fw_rev)?2:1;

	if(module_type != fw_type)
	{
		pr_err("type %d module fw 0x%X %d can not update to type %d fw rev 0x%X %d\n",
				module_type,dev_fw_rev,dev_fw_rev,
				fw_type,bin_fw_rev,bin_fw_rev);
		rc = 0;
	}

	//check version number
	if(rc == 1 && !force_update)
	{
		if( bin_fw_rev <= dev_fw_rev )
		{
			pr_err("bin_fw_rev 0x%X %d not higher than dev_fw_rev 0x%x %d, can not do FW updating\n",
					bin_fw_rev,bin_fw_rev,dev_fw_rev,dev_fw_rev);
			rc = 0;
		}
		if(!is_fw_valid(bin_fw_rev))
		{
			pr_err("bin_fw_rev 0x%X %d not valid, can not do FW updating\n",bin_fw_rev,bin_fw_rev);
			rc = 0;
		}
	}

	return rc;
}

int rumba_update_fw(struct msm_ois_ctrl_t *ctrl, const char *file_path)
{
	#define SEND_SIZE 128
	int rc;
	uint64_t fw_file_size;
	uint8_t* fw_code_data;
	uint32_t fw_revision;
	uint32_t reg_fw_revision;
	int i;
	uint16_t reg_data = 0xeeee;
	uint16_t check_sum;
	uint8_t  send_data[4];

#ifdef DUMP_CHECK
	char dumpFileName[64];
	uint8_t  send_block_data[SEND_SIZE];
#endif

	if(!rumba_is_idle_state(ctrl))
	{
		return -1;
	}

	rc = get_file_size(file_path,&fw_file_size);
	if(rc < 0)
	{
		pr_err("fw file %s open failed!",file_path);
		return -12;
	}

	if(fw_file_size != 28672)
	{
		pr_err("fw file size %lld is not 28672!\n",fw_file_size);
		return -9;
	}

	fw_code_data = kzalloc(sizeof(uint8_t)*fw_file_size,GFP_KERNEL);
	if(!fw_code_data)
	{
		pr_err("no memory!\n");
		return -ENOMEM;
	}

	if(read_file_into_buffer(file_path,fw_code_data,fw_file_size) < 0)
	{
		rc = -3;
		pr_err("read fw data to buffer failed!\n");
		goto Error;
	}

	fw_revision = get_fw_revision_from_code_data(fw_code_data);

	rumba_read_dword(ctrl,0x00FC,&reg_fw_revision);

	pr_info("Going update FW, dev_fw_rev 0x%x %d --> bin_fw_rev 0x%x %d, bin file size %lld\n",
			reg_fw_revision,reg_fw_revision,
			fw_revision,fw_revision,
			fw_file_size
	);
#if 1
	// 0 1 1 1  x x x  1
	/*
	0: 64bytes
	1: 128bytes
	2: 256bytes
	3: 8bytes
	4: 16bytes
	5: 32bytes
	*/
	rumba_write_byte(ctrl,0x000C,0x73);//Update block size=7, FWUP_DSIZE=128bytes, FWUPEN=Start

	delay_ms(55);//Wait for FlashROM erase 7 blocks

	/* Write FW Code Data */
	for(i=0;i<fw_file_size/SEND_SIZE;i++)
	{
		rc = rumba_write_seq_bytes(ctrl,0x0100,fw_code_data+i*SEND_SIZE,SEND_SIZE);
#ifdef DUMP_CHECK
		snprintf(dumpFileName,sizeof(dumpFileName),"/sdcard/ois_fw/send/%d",i);
		sysfs_write_byte_seq(dumpFileName,fw_code_data+i*SEND_SIZE,SEND_SIZE);

		rc = rumba_read_seq_bytes(ctrl,0x0100,send_block_data,SEND_SIZE);
		snprintf(dumpFileName,sizeof(dumpFileName),"/sdcard/ois_fw/rec/%d",i);
		sysfs_write_byte_seq(dumpFileName,send_block_data,SEND_SIZE);
#endif
		if(rc < 0)
		{
			pr_err("send code data failed! rc = %d\n",rc);
			rc = -4;
			goto Error;
		}
		delay_ms(5);//Wait for FlashROM writing 64bytes*2
	}

	/* Write Error Status Check*/
	rumba_read_word(ctrl,0x0006,&reg_data);
	if(reg_data == 0x0000)
	{
		check_sum = get_check_sum_short((uint16_t*)fw_code_data,fw_file_size);
		//get_check_sum(fw_code_data,fw_file_size);
		send_data[0] = (check_sum & 0x00FF);
		send_data[1] = (check_sum & 0xFF00) >> 8;
		send_data[2] = 0;
		send_data[3] = 0x80; //Self Reset Request

		rumba_write_seq_bytes(ctrl,0x0008,send_data,4);

		delay_ms(190);//Wait Self Reset + OIS Data Section init, 20+170 ms

		rumba_read_word(ctrl,0x0006,&reg_data);//Error Status read

		if(reg_data == 0x0000)
		{
			rumba_read_dword(ctrl,0x00FC,&reg_fw_revision);
			if(reg_fw_revision == fw_revision)
			{
				pr_info("FW revision 0x%x %d update Succeeded!\n",
					reg_fw_revision,
					reg_fw_revision
					);
			}
			else
			{
				pr_err("FW revision (code fw 0x%x reg fw 0x%x) update failed!\n",
					fw_revision,
					reg_fw_revision
					);
				rc = -5;
				goto Error;
			}
		}
		else
		{
			pr_err("FW Update Error! reg 0x0006 val 0x%x\n",reg_data);
			rc = -6;
			goto Error;
		}
	}
	else
	{
		pr_err("FW Code Write to Flash ROM Error! reg 0x0006 val 0x%x\n",reg_data);
		rc = -7;
		goto Error;
	}
#endif
	kfree(fw_code_data);
	return 0;
Error:
	kfree(fw_code_data);
	return rc;
}
typedef struct
{
	uint16_t reg_addr;
	uint32_t reg_val;
	uint8_t  length;
}reg_write_array_t;

int rumba_update_parameter_for_updating_fw(struct msm_ois_ctrl_t *ctrl,uint32_t src_fw_rev, uint32_t target_fw_rev)
{
	reg_write_array_t fw_init_setting_rev15456[] =
	{
		{0x0230,0x50,1},
		{0x0231,0x14,1},
		{0x0232,0x46,1},
		{0x0233,0x1E,1},
		{0x0318,0x3EA8F5C3,4},
		{0x031C,0x3F7F3B64,4},
		{0x0348,0x3F28F5C3,4},
		{0x034C,0x3F800000,4},
		{0x0378,0x3F800000,4},
		{0x0385,0x64,1},
		{0x0387,0x64,1},
		{0x03A8,0x3F800000,4},
		{0x03B5,0x64,1},
		{0x03B7,0x64,1},
		{0x0454,0x39AEC2CC,4},
		{0x0458,0x3F7F84B3,4},
		{0x045C,0x3AF6999E,4},
		{0x0460,0x000000FA,4}
	};
	reg_write_array_t fw_init_setting_rev15197[] =
	{
		{0x0244,0x098F,2},
		{0x0246,0xF671,2},
		{0x0430,0x43FA0000,4},
		{0x0454,0x3B03126F,4},
		{0x0458,0x3F7FD6D9,4},
		{0x045C,0x3A249B44,4},
		{0x0460,0x000001F4,4},
		{0x0464,0x00000000,4},
		{0x0468,0x00000000,4},
		{0x046C,0x00000000,4},
		{0x0470,0x00000000,4},
		{0x04AC,0x00000000,4}
	};
	reg_write_array_t *p_write_array;
	int i;
	int rc = 0;
	int size = 0;
	if((src_fw_rev == 15197 || src_fw_rev == 14819) && target_fw_rev == 15456)
	{
		p_write_array = fw_init_setting_rev15456;
		size = sizeof(fw_init_setting_rev15456)/sizeof(reg_write_array_t);
	}
	else if(src_fw_rev == 16173 && target_fw_rev == 15197)
	{
		p_write_array = fw_init_setting_rev15197;
		size = sizeof(fw_init_setting_rev15197)/sizeof(reg_write_array_t);
	}
	else
	{
		pr_err("can not apply any parameter updating register array!\n");
		return -1;
	}

	for(i=0;i<size;i++)
	{
		if(p_write_array[i].length == 1)
		{
			pr_info("Write 0x%hX to reg 0x%04X for FW init\n",(uint16_t)p_write_array[i].reg_val,p_write_array[i].reg_addr);
			rc = rumba_write_byte(ctrl,p_write_array[i].reg_addr,(uint16_t)p_write_array[i].reg_val);
		}
		else if(p_write_array[i].length == 2)
		{
			pr_info("Write 0x%hX to reg 0x%04X for FW init\n",(uint16_t)p_write_array[i].reg_val,p_write_array[i].reg_addr);
			rc = rumba_write_word(ctrl,p_write_array[i].reg_addr,(uint16_t)p_write_array[i].reg_val);
		}
		else if(p_write_array[i].length == 4)
		{
			pr_info("Write 0x%X to reg 0x%04X for FW init\n",(uint16_t)p_write_array[i].reg_val,p_write_array[i].reg_addr);
			rc = rumba_write_dword(ctrl,p_write_array[i].reg_addr,p_write_array[i].reg_val);
		}
		if(rc != 0)
		{
			break;
		}
	}

	if(rc == 0)
	{
		rc = rumba_check_flash_write_result(ctrl,OIS_DATA);
	}
	return rc;

}
int rumba_init_params_for_updating_fw(struct msm_ois_ctrl_t *ctrl,uint32_t fw_revision)
{
	int rc;

	if(fw_revision >= 15647)
		rc = rumba_write_byte(ctrl,0x0036,0x41);//above rev15647 set 0x41 instead of 0x81
	else
		rc = rumba_write_byte(ctrl,0x0036,0x81);
	if(rc == 0)
	{
		delay_ms(170);
	}
	return rc;
}
