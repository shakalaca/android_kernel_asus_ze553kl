#include "fac_camera.h"
#ifdef FAC_DEBUG
#undef CDBG
#define CDBG(fmt, args...) pr_debug(fmt, ##args)
#else
#undef CDBG
#define CDBG(fmt, args...) pr_debug(fmt, ##args)
#endif

#define PDAF_INFO_MAX_SIZE 1024
uint16_t g_sensor_id[MAX_CAMERAS]={0};
struct msm_sensor_ctrl_t *g_sensor_ctrl[MAX_CAMERAS]={0};
// Sheldon_Li add for init ctrl++
int fcamera_power_up( struct msm_sensor_ctrl_t *s_ctrl)
{  
	int rc = 0;
	CDBG("sz_cam_fac E\n");
	if (!s_ctrl) {
		pr_err("sz_cam_fac %s:%d failed: %p\n",
		__func__, __LINE__, s_ctrl);
		return -EINVAL;
	}
	if(s_ctrl->sensor_state == MSM_SENSOR_POWER_UP)
		return 0;
	rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
	if(rc == 0){
		s_ctrl->sensor_state = MSM_SENSOR_POWER_UP;
	}
	return rc;
}

int fcamera_power_down( struct msm_sensor_ctrl_t *s_ctrl)
{  
	int rc = 0;
	CDBG("sz_cam_fac E\n");
	if (!s_ctrl) {
		pr_err("sz_cam_fac %s:%d failed: %p\n",
		__func__, __LINE__, s_ctrl);
		return -EINVAL;
	}
	if(s_ctrl->sensor_state == MSM_SENSOR_POWER_DOWN)
		return 0;
	rc = s_ctrl->func_tbl->sensor_power_down(s_ctrl);			      
	if(rc == 0){
		s_ctrl->sensor_state = MSM_SENSOR_POWER_DOWN;
	}
	return rc;
}
// Sheldon_Li add for init ctrl--

int msm_sensor_testi2c(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	int power_self = 0;
	struct msm_camera_power_ctrl_t *power_info;
	struct msm_camera_i2c_client *sensor_i2c_client;
	struct msm_camera_slave_info *slave_info;
	const char *sensor_name;
	uint16_t chipid;
	uint32_t retry = 0;
	CDBG("sz_cam_fac E\n");
	if (!s_ctrl) {
		pr_err("sz_cam_fac %s:%d failed: %p\n",
			__func__, __LINE__, s_ctrl);
		return -EINVAL;
	}

	power_info = &s_ctrl->sensordata->power_info;
	sensor_i2c_client = s_ctrl->sensor_i2c_client;
	slave_info = s_ctrl->sensordata->slave_info;
	sensor_name = s_ctrl->sensordata->sensor_name;

	if (!power_info || !sensor_i2c_client || !slave_info ||
		!sensor_name) {
		pr_err("sz_cam_fac %s:%d failed: %p %p %p %p\n",
			__func__, __LINE__, power_info,
			sensor_i2c_client, slave_info, sensor_name);
		return -EINVAL;
	}

	     if(s_ctrl->sensor_state == MSM_SENSOR_POWER_DOWN){
		   rc = fcamera_power_up(s_ctrl); //power on first
		   if(rc < 0){
	  		pr_err("sz_cam_fac %s:%d camera power up failed!\n",__func__, __LINE__);
			return -EINVAL;
		   }
		   power_self = 1;
	     }
	   
	for (retry = 0; retry < 5; retry++) {
	  rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
		sensor_i2c_client, 0x0100,
		&chipid, MSM_CAMERA_I2C_WORD_DATA);
		if (rc < 0) {
			   rc = fcamera_power_down(s_ctrl); //power down if read failed
			   if(rc < 0){
		  		pr_err("sz_cam_fac %s:%d camera power down failed!\n",__func__, __LINE__);
				return rc;
			   }
			return -EINVAL;
		}

		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x0100,
		chipid, MSM_CAMERA_I2C_WORD_DATA);
		if (rc < 0) {		
			   rc = fcamera_power_down(s_ctrl); //power down if write failed
			   if(rc < 0){
		  		pr_err("sz_cam_fac %s:%d camera power down failed!\n",__func__, __LINE__);
				return rc;
			   }
			return -EINVAL;
		}
	}
	   if(s_ctrl->sensor_state == MSM_SENSOR_POWER_UP && power_self == 1){
		   rc = fcamera_power_down(s_ctrl); //power down end
		   if(rc < 0){
	  		pr_err("sz_cam_fac %s:%d camera power down failed!\n",__func__, __LINE__);
			return rc;
		   }
		   power_self = 0;
	   }
	return rc;
}
int t4k37_35_otp_read(struct msm_sensor_ctrl_t *s_ctrl ,u8 *out_buffer)
{
	struct msm_camera_i2c_client *sensor_i2c_client;
	int rc = 0,i,j;
	u16 reg_val = 0;
	u16 flag=0;
	u8 read_value[OTP_DATA_LEN_BYTE+1];
	CDBG("sz_cam_fac E\n");
	sensor_i2c_client = s_ctrl->sensor_i2c_client;
	//1.open stream
	rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
	sensor_i2c_client, 0x0100,&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
	rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
	sensor_i2c_client, 0x0100,reg_val|0x0001, MSM_CAMERA_I2C_BYTE_DATA);
	msleep(5);

	//2.get access to otp, and set read mode 1xxxx101
	//2.1 set clk
	rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
	sensor_i2c_client, 0x3545,&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
	rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
	sensor_i2c_client, 0x3545,(reg_val|0x0c)&0xec, MSM_CAMERA_I2C_BYTE_DATA);

	//3.read otp ,it will spend one page(64 Byte) to store 24 * 8 bite
	for ( j =0 ; j <3; j++){
		memset(read_value,0,sizeof(read_value));
		//3.1 set page num
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x3502,0x0000 + j, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) pr_err("select page failed\n");
		msleep(5);
		
		//3.2 get access to otp, and set read mode 1xxxx101
		rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
		sensor_i2c_client, 0x3500,&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x3500,(reg_val|0x85)&0xfd, MSM_CAMERA_I2C_BYTE_DATA);
		msleep(5);
		
		for ( i = 0;i < OTP_DATA_LEN_BYTE ; i++){
			//3.3 read page otp data
			rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
			sensor_i2c_client, 0x3504 + i,(uint16_t*)&read_value[i],MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0)pr_err( "failed to read OTP data j:%d,i:%d\n",j,i);
			if(read_value[i])
			{
				CDBG("find non zero data in otp j=%d\n",j);
				flag=1;
			}
		}
		if(flag)memcpy(&out_buffer[OTP_DATA_LEN_BYTE*j],read_value,OTP_DATA_LEN_BYTE);
	}
	//4.close otp access
	rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
	sensor_i2c_client, 0x3500,&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
	rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
	sensor_i2c_client, 0x3500,reg_val&0x007f, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0)pr_err("sz_cam_fac %s: close otp access failed\n", __func__);

	//5.close stream
	rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
	sensor_i2c_client, 0x0100,&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
	rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
	sensor_i2c_client, 0x0100,reg_val|0x0000, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0)pr_err("sz_cam_fac %s: close stream failed\n", __func__);

	return rc;	
}


// Sheldon_Li add for imx214\362 ++
int imx214_otp_read(struct msm_sensor_ctrl_t *s_ctrl ,u8 *out_buffer)
{
	int rc = 0,i=0,j=0,flag=0,count=0;
	int check_count=0;	
	u16 check_status=0;
	u8 read_value[OTP_DATA_LEN_BYTE+1];
	struct msm_camera_i2c_client *sensor_i2c_client;
	CDBG("sz_cam_fac E\n");
	if(!s_ctrl){
		pr_err("sz_cam_fac %s:%d failed: %p\n",__func__, __LINE__, s_ctrl);
		return -EINVAL;
	}
	sensor_i2c_client = s_ctrl->sensor_i2c_client;
	rc = fcamera_power_up(s_ctrl); //power on first
	if(rc < 0){
		pr_err("sz_cam_fac %s:%d camera power up failed!\n",__func__, __LINE__);
		return -EINVAL;
	}
	for(j = 0;  j < 15 &&  count<3;  j++)
	{
		//flag = 0; //clear flag
		//1.Set the NVM page number,N(=0-15)
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x0a02,(j), MSM_CAMERA_I2C_BYTE_DATA);
		if(rc < 0) pr_err("sz_cam_fac %s : Set the NVM page number failed!\n",__func__);

		//2.Turn ON OTP Read MODE
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x0a00,0x01, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0) pr_err("sz_cam_fac %s :  Turn ON OTP Read MODE failed!\n",__func__);
		check_count=0;
		do {
			rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
			sensor_i2c_client, 0x0A01,&check_status, MSM_CAMERA_I2C_BYTE_DATA);
			usleep_range(300, 500);
			check_count++;
		} while (((check_status & 0x1d) != 0x1d) && (check_count <= 10));
		
		//3.Read Data0-Data63 in the page N, as required.
		if((check_status & 0x1d) != 0x1d){
			memset(read_value,0,sizeof(read_value));
			for ( i = 0; i < OTP_DATA_LEN_BYTE ;  i++){
				// read page otp data
				rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
				sensor_i2c_client, 0x0a04 + i,(uint16_t*)&read_value[i], MSM_CAMERA_I2C_BYTE_DATA);
				if(rc<0)	pr_err( "sz_cam_fac failed to read OTP data j:%d,i:%d\n",j,i);
				if(read_value[i] != 0)flag = 1;
			}
			if(flag)memcpy(&out_buffer[OTP_DATA_LEN_BYTE*count++],read_value,OTP_DATA_LEN_BYTE);
		}
	}
	if( fcamera_power_down(s_ctrl)<0)pr_err("sz_cam_fac %s camera power down failed!\n",__func__);
	return rc;
}

int imx362_otp_read(struct msm_sensor_ctrl_t *s_ctrl ,u8 *out_buffer)
{
	int rc = 0,i=0,j=0;
	int check_count=0;
	u16 check_status=0;
	u8 read_value[OTP_DATA_LEN_BYTE+1];
	struct msm_camera_i2c_client *sensor_i2c_client;
	CDBG("sz_cam_fac E\n");
	if(!s_ctrl){
		pr_err("sz_cam_fac %s:%d failed: %p\n",__func__, __LINE__, s_ctrl);
		return -EINVAL;
	}
	sensor_i2c_client = s_ctrl->sensor_i2c_client;
	rc = fcamera_power_up(s_ctrl); //power on first
	if(rc < 0){
		pr_err("sz_cam_fac %s:%d camera power up failed!\n",__func__, __LINE__);
		return -EINVAL;
	}
	for(j = 0;  j < 3;  j++)
	{
		//1.Set the NVM page number,N(=0-3)
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x0a02,(j), MSM_CAMERA_I2C_BYTE_DATA);
		if(rc < 0) pr_err("sz_cam_fac %s : Set the NVM page number failed!\n",__func__);

		//2.Turn ON OTP Read MODE
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x0a00,0x01, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0) pr_err("sz_cam_fac %s :  Set up for NVM read failed!\n",__func__);
		check_count=0;
		do {
			rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
			sensor_i2c_client, 0x0A01,&check_status, MSM_CAMERA_I2C_BYTE_DATA);
			usleep_range(300, 500);
			check_count++;
		} while (((check_status & 0x1d) != 0x1d) && (check_count <= 10));

		//3.Read Data0-Data63 in the page N, as required.
		if((check_status & 0x1d) != 0x1d)
		{
			memset(read_value,0,sizeof(read_value));
			for ( i = 0; i < OTP_DATA_LEN_BYTE ;  i++)
			{
				// read page otp data
				rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
				sensor_i2c_client, 0x0a04 + i, (uint16_t*)&read_value[i], MSM_CAMERA_I2C_BYTE_DATA);
				if(rc<0)	pr_err( "sz_cam_fac failed to read OTP data j:%d,i:%d\n",j,i);
			}
			memcpy(&out_buffer[OTP_DATA_LEN_BYTE*j],read_value,OTP_DATA_LEN_BYTE);
		}
	}
	if( fcamera_power_down(s_ctrl)<0)pr_err("sz_cam_fac %s camera power down failed!\n",__func__);
	return rc;
}

// Sheldon_Li add for imx214\362 --


// Sheldon_Li add for s5k3m3_otp_read++ 
int s5k3m3_otp_read(struct msm_sensor_ctrl_t *s_ctrl ,u8 *out_buffer)
{
	int rc = 0,i=0,j=0,flag=0,count=0;
	u16 reg_val = 0;
	u8 read_value[OTP_DATA_LEN_BYTE+1];
	struct msm_camera_i2c_client *sensor_i2c_client;

	CDBG("sz_cam_fac E\n");
	if(!s_ctrl){
		pr_err("sz_cam_fac %s:%d failed: %p\n",__func__, __LINE__, s_ctrl);
		return -EINVAL;
	}

	sensor_i2c_client = s_ctrl->sensor_i2c_client;
	rc = fcamera_power_up(s_ctrl); //power on first
	if(rc < 0){
		pr_err("sz_cam_fac %s:%d camera power up failed!\n",__func__, __LINE__);
		return -EINVAL;
	}
	do{
		//0.open stream
		rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
		sensor_i2c_client, 0x0100,&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc < 0){pr_err("camera open stream failed!\n");break;}
			
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x0100,reg_val|0x01, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc < 0){pr_err("camera open stream failed!\n");break;}

		for(j = 0;  j < 63&&count<3;  j++)
		{
			//1.Set the NVM page number,N(=0-63)
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x0a02,(j<<8), MSM_CAMERA_I2C_WORD_DATA);
			if(rc < 0) pr_err("sz_cam_fac %s : Set the NVM page number failed!\n",__func__);

			//2.Turn ON OTP Read MODE
			rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
			sensor_i2c_client, 0x0a00,&reg_val, MSM_CAMERA_I2C_WORD_DATA);
			if(rc<0)  pr_err("sz_cam_fac %s : Read for NVM read failed!\n",__func__);
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x0a00,(reg_val|0x0100), MSM_CAMERA_I2C_WORD_DATA);
			if(rc<0) pr_err("sz_cam_fac %s :  Set up for NVM read failed!\n",__func__);

			//3.Read Data0-Data63 in the page N, as required.
			memset(read_value,0,sizeof(read_value));
			for ( i = 0; i < OTP_DATA_LEN_BYTE ;  i++){
				// read page otp data
				rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
				sensor_i2c_client, 0x0a04 + i,(uint16_t*)&read_value[i], MSM_CAMERA_I2C_BYTE_DATA);
				if(rc<0)pr_err( "failed to read OTP data j:%d,i:%d\n",j,i);
				if(read_value[i])
				{
					CDBG("sz_cam_fac find non zero data in otp j=%d\n",j);
					flag=1;
				}
			}
			if(flag)
				memcpy(&out_buffer[OTP_DATA_LEN_BYTE*count++],read_value,OTP_DATA_LEN_BYTE);
			//4.close stream
			rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
			sensor_i2c_client, 0x0100,&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x0100,reg_val|0x00, MSM_CAMERA_I2C_BYTE_DATA);

			//5.make initial state
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x0a00,0x0000, MSM_CAMERA_I2C_WORD_DATA);	
		}
		
	}while(0);
	if( fcamera_power_down(s_ctrl)<0)pr_err("sz_cam_fac %s camera power down failed!\n",__func__); 
	CDBG("X\n");  
	return rc;
}
// Sheldon_Li add for s5k3m3_otp_read --
int ov5670_otp_read(struct msm_sensor_ctrl_t *s_ctrl ,u8 *out_buffer)
{
	int rc = 0,i=0,j=0;
	u16 reg_val = 0;
	u16 start_addr, end_addr;
	u8 read_value[OTP_DATA_LEN_BYTE+1];
	struct msm_camera_i2c_client *sensor_i2c_client;

	CDBG("sz_cam_fac E s_ctrl=%p\n",s_ctrl);
	if(!s_ctrl){
	pr_err("sz_cam_fac %s:%d failed: %p\n",__func__, __LINE__, s_ctrl);
	return -EINVAL;
	}

	sensor_i2c_client = s_ctrl->sensor_i2c_client;
	if(s_ctrl->sensordata->slave_info->sensor_id!=SENSOR_ID_OV5670)
	{
	pr_err("sz_cam_fac sensor_id 0x%x mismatch,expected is 0x%x"
	,s_ctrl->sensordata->slave_info->sensor_id,SENSOR_ID_OV5670);
	return -EINVAL;
	}
	rc = fcamera_power_up(s_ctrl); //power on first
	if(rc < 0){
		pr_err("sz_cam_fac %s:%d camera power up failed!\n",__func__, __LINE__);
		return -EINVAL;
	}

	do{
		//1. Reset sensor as default
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x0103, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0){pr_err("sz_cam_fac %s : Reset sensor as default failed!\n",__func__);break;}
		
		//1.1 disable OTP_DPC
		rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
		sensor_i2c_client, 0x5002, &reg_val, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0){pr_err("sz_cam_fac %s : read otp option failed!\n",__func__);break;}
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x5002,reg_val&0xf7, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0){pr_err("sz_cam_fac %s : write otp option failed!\n",__func__);break;}

		for(j = 0; j < 3; j++)
		{
			start_addr = 0x7010+0x20*j;
			end_addr=start_addr -1;

			//4.Set OTP MODE Bit[6] : 0=Auto mode 1=Manual mode
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d84,0xc0, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write OTP_MODE_CTRL registor failed!\n",__func__);


			//5.Set OTP_REG85 : Bit[2]=load data enable  Bit[1]= load setting enable
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d85,0x06, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write OTP_REG85  failed!\n",__func__);

			//6.Set OTP read address
			//start address High
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d88,(start_addr >> 8) & 0xff, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write start address High  failed!\n",__func__);

			//start address Low
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d89,(start_addr & 0xff) , MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write start address Low  failed!\n",__func__);

			//end address High
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d8a,(end_addr >> 8) & 0xff, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write end address High  failed!\n",__func__);

			//end address Low
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d8b,(end_addr  & 0xff), MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write end address Low  failed!\n",__func__);

			/* trigger auto_load */
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x0100, 0x01,MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write trigger auto_load  failed!\n",__func__);

			//7.Load OTP data enable
			rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
			sensor_i2c_client, 0x3d81,&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Read data enable REG  failed!\n",__func__);

			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d81,(reg_val | 0x01), MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write data enable REG  failed!\n",__func__);
			usleep_range(500, 5000);
			memset(read_value,0,sizeof(read_value));
			for ( i = 0; i < OTP_DATA_LEN_BYTE ;  i++){
				reg_val = 0;
				// read page otp data
				rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
				sensor_i2c_client, start_addr + i,(uint16_t*)&read_value[i], MSM_CAMERA_I2C_BYTE_DATA); 
				if(rc<0)pr_err( "sz_cam_fac failed to read OTP data j:%d,i:%d\n",j,i);
			}
			memcpy(&out_buffer[OTP_DATA_LEN_BYTE*j], read_value, OTP_DATA_LEN_BYTE);
		}

	}while(0);
	rc = fcamera_power_down(s_ctrl); //power down end
	if(rc < 0)pr_err("sz_cam_fac %s:%d camera power down failed!\n",__func__, __LINE__);
	CDBG("X\n");
	return rc;
}

// Sheldon_Li add for ov8856 ++
int ov8856_otp_read(struct msm_sensor_ctrl_t *s_ctrl ,u8 *out_buffer)
{
	int rc = 0,i=0,j=0;
	u16 reg_val = 0;
	u8 flag=0;
	uint16_t count=0;
	u16 start_addr, end_addr;
	u8 read_value[OTP_DATA_LEN_BYTE+1];
	struct msm_camera_i2c_client *sensor_i2c_client;

	CDBG("sz_cam_fac E\n");
	if(!s_ctrl){
	pr_err("sz_cam_fac %s:%d failed: %p\n",__func__, __LINE__, s_ctrl);
	return -EINVAL;
	}

	sensor_i2c_client = s_ctrl->sensor_i2c_client;
	if(s_ctrl->sensordata->slave_info->sensor_id!=SENSOR_ID_OV8856)
	{
	pr_err("sensor_id 0x%x mismatch,expected is 0x%x"
	,s_ctrl->sensordata->slave_info->sensor_id,SENSOR_ID_OV8856);
	return -EINVAL;
	}

	rc = fcamera_power_up(s_ctrl); //power on first
	if(rc < 0){
		pr_err("sz_cam_fac %s:%d camera power up failed!\n",__func__, __LINE__);
		return -EINVAL;
	}

	do{
		//1. Reset sensor as default
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x0103, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0){pr_err("sz_cam_fac %s : Reset sensor as default failed!\n",__func__);break;}

		//2.otp_option_en = 1
		rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
		sensor_i2c_client, 0x5001, &reg_val, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0){pr_err("sz_cam_fac %s : read otp option failed!\n",__func__);break;}

		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x5001,((0x00 & 0x08) | (reg_val & (~0x08))), MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0){pr_err("sz_cam_fac %s : write otp option failed!\n",__func__);break;}

		for(j = 0;  j <MSM_CAMERA_I2C_BYTE_DATA/0x20  &&  count<3;  j++)
		{
			start_addr = 0x7010+0x20*j;
			end_addr=start_addr -1;

			//4.Set OTP MODE Bit[6] : 0=Auto mode 1=Manual mode
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d84,
			0xc0, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write OTP_MODE_CTRL registor failed!\n",__func__);


			//5.Set OTP_REG85 : Bit[2]=load data enable  Bit[1]= load setting enable
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d85,
			0x06, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write OTP_REG85  failed!\n",__func__);

			//6.Set OTP read address
			//start address High
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d88,
			(start_addr >> 8) & 0xff, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write start address High  failed!\n",__func__);

			//start address Low
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d89,
			(start_addr & 0xff) , MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write start address Low  failed!\n",__func__);

			//end address High
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d8a,
			(end_addr >> 8) & 0xff, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write end address High  failed!\n",__func__);

			//end address Low
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d8b,
			(end_addr  & 0xff), MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write end address Low  failed!\n",__func__);

			/* trigger auto_load */
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x0100, 0x01,MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write trigger auto_load  failed!\n",__func__);

			//7.Load OTP data enable
			rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
			sensor_i2c_client, 0x3d81,
			&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Read data enable REG  failed!\n",__func__);

			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3d81,
			(reg_val | 0x01), MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("sz_cam_fac %s : Write data enable REG  failed!\n",__func__);

			memset(read_value,0,sizeof(read_value));
			for ( i = 0; i < OTP_DATA_LEN_BYTE ;  i++){
				reg_val = 0;
				// read page otp data
				rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
				sensor_i2c_client, start_addr + i,(uint16_t*)&read_value[i], MSM_CAMERA_I2C_BYTE_DATA);
				if(rc<0)pr_err( "failed to read OTP data j:%d,i:%d\n",j,i);
				if(read_value[i])flag=1;
			}
			if(flag)memcpy(&out_buffer[OTP_DATA_LEN_BYTE*count++], read_value, OTP_DATA_LEN_BYTE);
		}

	}while(0);
	
	rc = fcamera_power_down(s_ctrl); //power down end
	if(rc < 0)pr_err("sz_cam_fac  camera power down failed!\n");
	return rc;
}
// Sheldon_Li add for ov8856 --

#if 0
static int rear_opt_1_read(struct seq_file *buf, void *v)
{	
	int i;
#ifdef HADES_OTP_MODE
	 if(g_rear_eeprom_mapdata.mapdata != NULL){
		 for ( i = 0; i < 0x08 ;  i++){  //copy AFC(Wide)
				 rear_otp_data[i] = g_rear_eeprom_mapdata.mapdata[i];
		 }
		
		 for ( i = 0x10; i < 0x28 ;  i++){	//copy Module Info
				 rear_otp_data[i-0x08] = g_rear_eeprom_mapdata.mapdata[i];
		 }	 
	}else{
			 pr_err("sz_cam_fac %s:%d failed\n",__func__, __LINE__);
	 }
#endif

	for( i = 0; i < OTP_DATA_LEN_BYTE; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_data[i]);
		if((i&7) == 7)
		seq_printf(buf ,"\n");
	}
	
	return 0;
}

static int rear_opt_2_read(struct seq_file *buf, void *v)
{	
	int i;
#ifdef HADES_OTP_MODE
	 if(g_rear_eeprom_mapdata.mapdata != NULL){
		 for ( i = 0x08; i < 0x10 ;  i++){    //copy AFC(Tele)
				 rear_otp_1_data[i-0x08] = g_rear_eeprom_mapdata.mapdata[i];
		 }
		
		 for ( i = 0x10; i < 0x28 ;  i++){	//copy Module Info
				 rear_otp_1_data[i-0x08] = g_rear_eeprom_mapdata.mapdata[i];
		 }	 
	}else{
			 pr_err("sz_cam_fac %s:%d failed\n",__func__, __LINE__);
	 }
#endif

	for( i = 0; i < OTP_DATA_LEN_BYTE; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_1_data[i]);
		if((i&7) == 7)
		seq_printf(buf ,"\n");
	}
	return 0;
}
// Sheldon_Li add for create camera otp proc file--
#endif
//ASUS_BSP_Sheldon +++ "read eeprom thermal data for DIT"
int float32Toint24(struct msm_eeprom_memory_block_t eeprom_block,int addr, int len)
{
	int i=0;
	int retVal=0;
	uint dataVal=0;
	uint negative,exponent,mantissa;
	u8 rawData[5] = {0};
	CDBG("sz_cam_fac E\n");
	if(eeprom_block.mapdata != NULL)
	{
		if(addr > eeprom_block.num_data)
		{
		pr_err("sz_cam_%s: Address out of range!\n",__func__);
		return 0;
		}

		for (i = 0; i < len;  i++)
		{
		rawData[i] = eeprom_block.mapdata[addr+len-i-1];
		}
		memcpy(&dataVal,rawData,len);

		negative = ((dataVal >> 31) & 0x1);      // 0x10000000
		exponent = ((dataVal >> 23) & 0xFF);  // 0x7F800000
		mantissa = ((dataVal >>  0) & 0x7FFFFF) | 0x800000 ; // 0x007FFFFF implicit bit
		retVal = mantissa >> (22 - (exponent - 0x80));   
	}else{
		pr_err("sz_cam_%s: g_rear_eeprom_mapdata.mapdata is NULL!\n",__func__);
	}
	if( !exponent )
		return 0;
	if( negative )
		return -retVal;
	else
		return  retVal;
}



//asus bsp ralf:porting camera sensor related proc files>>
extern int asus_project_id;
//common++
int common_craete_proc_file(struct proc_dir_entry **pde,char* file_name,void* p,struct file_operations *op)
{
	int ret=-EINVAL;
	CDBG("sz_cam_fac E file_names=%s\n",file_name);
	*pde = 
		proc_create_data(file_name, 0666, NULL, op,p);
	if(*pde) {
		CDBG("sz_cam_fac create module proc file for %s sucessed!\n",file_name);
		ret=0;
	} else {
		pr_err("sz_cam_fac create module proc file for %s FAIL.\n",file_name);
	}
	return ret;
}
void common_remove_sysfs_file(struct kobject *kobj,struct device_attribute dev_attr)
{
	sysfs_remove_file(kobj,&dev_attr.attr);
	kobject_del(kobj);
}
int common_create_sysfs_file(struct kobject **kobj,char* file_name,struct device_attribute *dev_attr)
{
	int ret=-EINVAL;
	CDBG("sz_cam_fac E file_names=%s\n",file_name);
	*kobj = kobject_create_and_add(file_name, NULL);
	if(*kobj)
	{
		ret = sysfs_create_file(*kobj, &dev_attr->attr);
		if (ret)
		{
			pr_err("sz_cam_fac create sys file for %s failed\n",file_name);
		}
	}
	return ret;
}
//common--
//vcm++
char g_vcm_proc_file_names[MAX_CAMERAS][50]=
{VCM_PROC_FILES_REAR,
#ifdef ZE553KL
VCM_PROC_FILES_REAR_2,VCM_PROC_FILES_FRONT,VCM_PROC_FILES_FRONT_2
#endif
#ifdef ZD552KL_PHOENIX
VCM_PROC_FILES_FRONT,VCM_PROC_FILES_FRONT_2,VCM_PROC_FILES_REAR_2
#endif
#ifdef ZS550KL
VCM_PROC_FILES_FRONT
#endif
};

static struct proc_dir_entry *vcm_pdes[MAX_CAMERAS];
static int vcm_show(struct seq_file *buf, void *v)
{
	int ret=0;
	struct msm_actuator_ctrl_t *vcm_ctrl=(struct msm_actuator_ctrl_t *)buf->private;
	CDBG("sz_cam_fac E vcm_ctrl=%p\n",vcm_ctrl);
	CDBG("sz_cam_fac E curr_step_pos=%d,total_steps=%d\n",vcm_ctrl->curr_step_pos,vcm_ctrl->total_steps);
	if(vcm_ctrl)
	{
		if(!vcm_ctrl->step_position_table)
		{
			seq_printf(buf, "0\n");
		}else{
			seq_printf(buf, "%d\n",vcm_ctrl->last_lens_pos);
		}
	}
	return ret;
}
static int vcm_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, vcm_show, PDE_DATA(inode));
}
static struct file_operations vcm_fops = {
	.owner = THIS_MODULE,
	.open = vcm_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
void vcm_create_proc_file(struct msm_actuator_ctrl_t *vcm_ctrl)
{
	char* target_file_name=g_vcm_proc_file_names[vcm_ctrl->cam_name];
	CDBG("sz_cam_fac E vcm_ctrl=%p\n",vcm_ctrl);
	if(strlen(target_file_name)>0)
		common_craete_proc_file(
		&vcm_pdes[vcm_ctrl->cam_name]
		,target_file_name
		,vcm_ctrl
		,&vcm_fops);
}
void vcm_remove_file(struct msm_actuator_ctrl_t *vcm_ctrl)
{
    extern struct proc_dir_entry proc_root;
	char* target_file_name=g_vcm_proc_file_names[vcm_ctrl->cam_name];
    CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
    remove_proc_entry(target_file_name, &proc_root);
}

//vcm--
//eeprom++
char g_eeprom_proc_file_names[MAX_CAMERAS][50]=
{EEPROM_PROC_FILE_REAR,EEPROM_PROC_FILE_FRONT,
#ifdef ZE553KL
EEPROM_PROC_FILE_REAR_2,
#endif
#ifdef ZD552KL_PHOENIX
EEPROM_PROC_FILE_FRONT_2,
#endif
"invalid_eeprom"};
struct msm_eeprom_ctrl_t * g_ectrls[MAX_CAMERAS];
static struct proc_dir_entry *eeprom_pdes[MAX_CAMERAS];
static int eeprom_proc_read(struct seq_file *buf, void *v)
{
	int i;
	struct msm_eeprom_ctrl_t * e_ctrl=(struct msm_eeprom_ctrl_t *)buf->private;
	struct msm_eeprom_memory_block_t *eeprom_data;
	CDBG("sz_cam_fac E e_ctrl=%p\n",e_ctrl);
	eeprom_data=&e_ctrl->cal_data;
	CDBG("sz_cam_fac g_eeprom_mapdata num_map:%d num_data:%d"
		,eeprom_data->num_map, eeprom_data->num_data);
	if (!eeprom_data->mapdata)
	{
		seq_printf(buf,"invalid paramters\n");
	}else{
		for( i = 0; i < eeprom_data->num_data; i++)
		{
			seq_printf(buf, "0x%02X ",eeprom_data->mapdata[i]);
			seq_printf(buf ,"\n");
		}
	}
	return 0;
}
static int eeprom_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, eeprom_proc_read, PDE_DATA(inode));
}
static struct file_operations eeprom_proc_fops = {
	.owner = THIS_MODULE,
	.open = eeprom_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
void eeprom_create_proc_file(struct msm_eeprom_ctrl_t * e_ctrl)
{
	char* target_file_name=g_eeprom_proc_file_names[e_ctrl->subdev_id];
	CDBG("sz_cam_fac E e_ctrl=%p\n",e_ctrl);
	if(strlen(target_file_name)>0)
	{
		g_ectrls[e_ctrl->subdev_id]=e_ctrl;
		common_craete_proc_file(
			&eeprom_pdes[e_ctrl->subdev_id]
			,target_file_name
			,e_ctrl
			,&eeprom_proc_fops);
	}
}
void eeprom_remove_file(uint32_t subdev_id)
{
    extern struct proc_dir_entry proc_root;
	char* target_file_name=g_vcm_proc_file_names[subdev_id];
    CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
		remove_proc_entry(target_file_name, &proc_root);
}
//eeprom--
//ASUS_BSP_Sheldon +++ "read ArcSoft Dual Calibration Data for DIT"
#define ARCSOFTDATA_START 0x0834
#define ARCSOFTDATA_END    0x1034
static int arcsoft_dualCali_read(struct seq_file *buf, void *v)
{
	//ArcSoft Dual Calibration Data
	int i=0;
	uint32_t camera_id=0;
	struct msm_sensor_ctrl_t *s_ctrl=(struct msm_sensor_ctrl_t *)buf->private;
	struct msm_eeprom_ctrl_t *e_ctrl;
	CDBG("sz_cam_fac E \n");
	camera_id=s_ctrl->sensordata->cam_slave_info->camera_id;
	CDBG("sz_cam_fac E camera_id=%d\n",camera_id);
	e_ctrl=g_ectrls[camera_id];
	if(!e_ctrl)
	{
		CDBG("sz_cam_fac e_ctrl is null\n");
		//seq_printf(buf, "e_ctrl is null\n");
	}
	if(e_ctrl  &&  e_ctrl->cal_data.mapdata  &&  e_ctrl->cal_data.num_data>=ARCSOFTDATA_END)
	{
		CDBG("sz_cam_fac e_ctrl mapdata=%p,num_data=%d\n",e_ctrl->cal_data.mapdata,e_ctrl->cal_data.num_data);
		 for ( i = ARCSOFTDATA_START; i < ARCSOFTDATA_END && i < e_ctrl->cal_data.num_data;  i++)
		 {
			seq_printf(buf, "%c",e_ctrl->cal_data.mapdata[i]);	
		 }
	}else{
	  	pr_err("sz_cam_%s: g_rear_eeprom_mapdata.mapdata is NULL!\n",__func__);
	}
	return 0;
}
int arcsoft_dualCali_open(struct inode *inode, struct  file *file)
{
	return single_open(file, arcsoft_dualCali_read, PDE_DATA(inode));
}
const struct file_operations arcsoft_dualCali_fops = {
	.owner = THIS_MODULE,
	.open = arcsoft_dualCali_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
void arcsoft_dualCali_create_proc_file(struct msm_camera_sensor_slave_info *slave_info)
{
	char *target_file_name="";
	CDBG("sz_cam_fac E slave_info=%p\n",slave_info);
	switch(asus_project_id)
	{
		case ASUS_ZE553KL:if(slave_info->camera_id==CAMERA_0)target_file_name=PROC_FILE_EEPEOM_ARCSOFT;break;
		case ASUS_ZD552KL_PHOENIX:if(slave_info->camera_id==CAMERA_0)target_file_name=PROC_FILE_EEPEOM_ARCSOFT;break;
	}
	CDBG("sz_cam_fac  camera_id=%d,target_file_name=%s\n",slave_info->camera_id,target_file_name);
	if(strlen(target_file_name)>0)
		if(!proc_create_data(target_file_name, 0666, NULL, &arcsoft_dualCali_fops,g_sensor_ctrl[slave_info->camera_id]))
		{
			pr_err("sz_cam_fac create proc file failed\n");
		}
}
void arcsoft_dualCali_remove_file(void)
{
	  extern struct proc_dir_entry proc_root;
	  CDBG("sz_cam_fac E\n");
	  remove_proc_entry(PROC_FILE_EEPEOM_ARCSOFT, &proc_root);
}
// Sheldon_Li add for  eeprom map to OTP--

//eeprom_thermal++
char g_eeprom_thermal_proc_file_names[MAX_CAMERAS][50]=
{PROC_FILE_EEPEOM_THERMAL_REAR,PROC_FILE_EEPEOM_THERMAL_FRONT,
#ifdef ZE553KL
PROC_FILE_EEPEOM_THERMAL_REAR_2,
#endif
#ifdef ZD552KL_PHOENIX
PROC_FILE_EEPEOM_THERMAL_REAR_2,
#endif
"invalid_eeprom_thermal"};

static struct proc_dir_entry *eeprom_thermal_pdes[MAX_CAMERAS];
static int eeprom_temp_hades_rear(struct msm_eeprom_memory_block_t eeprom_block,struct seq_file *buf, void *v)
{
	
	if(eeprom_block.mapdata  && eeprom_block.num_data>=0x1041+4){
		//AFC(Wide) :
		//Infinity temperature
		seq_printf(buf, "%d ",float32Toint24(eeprom_block,0x1039,4));
		//Middle temperature
		seq_printf(buf, "%d ",float32Toint24(eeprom_block,0x103D,4));
		//Macro temperature
		seq_printf(buf, "%d\n",float32Toint24(eeprom_block,0x1041,4));
	}
	return 0;
}
static int eeprom_temp_hades_rear2(struct msm_eeprom_memory_block_t eeprom_block,struct seq_file *buf, void *v)
{
	if(eeprom_block.mapdata  && eeprom_block.num_data>=0x104D+4){
		//AFC(Tele):
		//Infinity temperature
		seq_printf(buf, "%d ",float32Toint24(eeprom_block,0x1045,4));
		//Middle temperature
		seq_printf(buf, "%d ",float32Toint24(eeprom_block,0x1049,4));
		//Macro temperature
		seq_printf(buf, "%d\n",float32Toint24(eeprom_block,0x104D,4));
	}
	return 0;
}
int eeprom_thermal_proc_read(struct seq_file *buf, void *v)
{
	int ret=0;
	struct msm_sensor_ctrl_t *s_ctrl=(struct msm_sensor_ctrl_t *)buf->private;
	struct msm_eeprom_ctrl_t *e_ctrl;
	CDBG("sz_cam_fac E s_ctrl=%p\n",s_ctrl);
	if(!s_ctrl)
	{
		seq_printf(buf, "invalid s_ctrl param\n");
		pr_err("sz_cam_fac slave_info is NULL\n");
	}
	else
	{
		e_ctrl=g_ectrls[s_ctrl->sensordata->cam_slave_info->camera_id];
		if(!(e_ctrl  &&  e_ctrl->cal_data.mapdata  &&  e_ctrl->cal_data.num_data>0))
		{
			seq_printf(buf, "invalid e_ctrl param\n");
			pr_err("sz_cam_fac e_ctrl is e_ctrl invalid\n");
		}else
		{
			switch(asus_project_id)
			{
				case ASUS_ZE553KL:
					if(s_ctrl->sensordata->cam_slave_info->camera_id==CAMERA_0)
						eeprom_temp_hades_rear(e_ctrl->cal_data,buf,v);
					if(s_ctrl->sensordata->cam_slave_info->camera_id==CAMERA_2)
						eeprom_temp_hades_rear2(e_ctrl->cal_data,buf,v);
					break;
				case ASUS_ZD552KL_PHOENIX:
					if(s_ctrl->sensordata->cam_slave_info->camera_id==CAMERA_0)
						eeprom_temp_hades_rear(e_ctrl->cal_data,buf,v);
				break;
				default:pr_err("sz_cam_fac invalid projectId:%d\n",asus_project_id);break;
			}
		}
	}
	return ret;
}
int eeprom_thermal_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, eeprom_thermal_proc_read, PDE_DATA(inode));
}
struct file_operations eeprom_thermal_fops = {
	.owner = THIS_MODULE,
	.open = eeprom_thermal_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
void eeprom_thermal_remove_proc_files(struct msm_sensor_ctrl_t *s_ctrl)
{
	extern struct proc_dir_entry proc_root;
	char* target_file_name=g_eeprom_thermal_proc_file_names[s_ctrl->sensordata->cam_slave_info->camera_id];
	CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
		remove_proc_entry(target_file_name,&proc_root);
}
void eeprom_thermal_create_proc_files(struct msm_camera_sensor_slave_info *slave_info)
{
	char* target_file_name=g_eeprom_thermal_proc_file_names[slave_info->camera_id];
	CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
		common_craete_proc_file(
		&eeprom_thermal_pdes[slave_info->camera_id]
		,target_file_name
		,g_sensor_ctrl[slave_info->camera_id]
		,&eeprom_thermal_fops);
}
//eeprom_thermal--

//otp++
static struct proc_dir_entry *g_otp_pdes[MAX_CAMERAS]={0};
u8 g_otp_data_banks[MAX_CAMERAS][OTP_DATA_LEN_BYTE*3];
char *g_otp_proc_file_name[50]={PROC_FILE_OTP_REAR_1,PROC_FILE_OTP_FRONT,
#ifdef ZE553KL
PROC_FILE_OTP_REAR_2
#endif
#ifdef ZD552KL_PHOENIX
PROC_FILE_OTP_FRONT_2
#endif
"invalid_otp"};
int sensor_otp_initial_read(struct msm_sensor_ctrl_t *s_ctrl,u8* out_buffer)
{
	CDBG("sz_cam_fac E\n");
	CDBG("sz_cam_fac sensor_id =0x%x\n",s_ctrl->sensordata->slave_info->sensor_id);
	switch (s_ctrl->sensordata->slave_info->sensor_id) {
	case SENSOR_ID_T4K35 :
	case SENSOR_ID_T4K37 : t4k37_35_otp_read(s_ctrl,out_buffer);break;
	case SENSOR_ID_IMX214: imx214_otp_read(s_ctrl,out_buffer);break;
	//case SENSOR_ID_IMX298: t4k37_35_otp_read(s_ctrl,out_buffer);break;
	//case SENSOR_ID_IMX351: imx351_otp_read_from_eeprom(s_ctrl,out_buffer);break;
	case SENSOR_ID_IMX362: imx362_otp_read(s_ctrl,out_buffer);break;
	case SENSOR_ID_OV5670: ov5670_otp_read(s_ctrl,out_buffer);break;
	case SENSOR_ID_OV8856: ov8856_otp_read(s_ctrl,out_buffer);break;
	case SENSOR_ID_S5K3M3: s5k3m3_otp_read(s_ctrl,out_buffer);break;
	}
	return 0;
}
void otp_data_by_eeprom_hades(u8* out_buffer,u8* in_buffer,uint16_t camera_id)
{
	if(camera_id==CAMERA_0)
		memcpy(&out_buffer[0],&in_buffer[0],8);//AFC data
	if(camera_id==CAMERA_2)
		memcpy(&out_buffer[0],&in_buffer[8],8);//AFC data
	memcpy(&out_buffer[8],&in_buffer[0x10],24);//AFC data
}
void otp_data_by_eeprom_phoenix(u8* out_buffer,u8* in_buffer,uint16_t camera_id)
{
	if(camera_id==CAMERA_0)
		memcpy(&out_buffer[0],&in_buffer[0],OTP_DATA_LEN_WORD);
}

int otp_proc_read(struct seq_file *buf, void *v)
{
	int ret=0;
	int i=0;
	int bank=0;
	struct msm_sensor_ctrl_t *s_ctrl=(struct msm_sensor_ctrl_t *)buf->private;
	u8* eeprom_data;
	uint16_t camera_id=0;
	CDBG("sz_cam_fac E s_ctrl=%p\n",s_ctrl);
	camera_id=s_ctrl->sensordata->cam_slave_info->camera_id;
	eeprom_data=&g_otp_data_banks[camera_id][0];
	if(!eeprom_data)
	{
		ret=seq_printf(buf, "invalid param\n");
		pr_err("sz_cam_fac slave_info is NULL\n");
	}
	else
	{
		struct msm_eeprom_ctrl_t *e_ctrl=g_ectrls[camera_id];
		if(e_ctrl==NULL)
		{
			//seq_printf(buf, "e_ctrl is NULL\n");
			pr_err("sz_cam_fac e_ctrl is NULL\n");
		}else
		{
			if(e_ctrl->cal_data.mapdata &&  e_ctrl->cal_data.num_data>0)
			{
				CDBG("sz_cam_fac find eeprom data for sensor id=0x%x\n",s_ctrl->sensordata->slave_info->sensor_id);
				switch(asus_project_id)
				{
					case ASUS_ZE553KL:otp_data_by_eeprom_hades(eeprom_data,e_ctrl->cal_data.mapdata,camera_id);break;
					case ASUS_ZD552KL_PHOENIX:otp_data_by_eeprom_phoenix(eeprom_data,e_ctrl->cal_data.mapdata,camera_id);break;
					default:pr_err("sz_cam_fac invalid projectId:%d\n",asus_project_id);break;
				}
			}
		}
		for(bank=0; bank<3;bank++){
			eeprom_data=&g_otp_data_banks[camera_id][OTP_DATA_LEN_BYTE*bank];
			for( i = 0; i < OTP_DATA_LEN_WORD; i++)
			{
				ret+=seq_printf(buf, "0x%02X ",eeprom_data[i]);
				if((i&7)==7)ret+=seq_printf(buf ,"\n");
			}
			if(bank<2)
				seq_printf(buf ,"\n");
		}
	}
	return 0;
}
int otp_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, otp_proc_read, PDE_DATA(inode));
}
struct file_operations otp_fops = {
	.owner = THIS_MODULE,
	.open = otp_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void otp_remove_proc_files(struct msm_sensor_ctrl_t *s_ctrl)
{
	extern struct proc_dir_entry proc_root;
	char* target_file_name=g_otp_proc_file_name[s_ctrl->sensordata->cam_slave_info->camera_id];
	CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
	{
		remove_proc_entry(target_file_name, &proc_root);
	}
}
void otp_create_proc_files(struct msm_camera_sensor_slave_info *slave_info)
{
	char* target_file_name=g_otp_proc_file_name[slave_info->camera_id];
	CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
	{
		sensor_otp_initial_read(g_sensor_ctrl[slave_info->camera_id],&g_otp_data_banks[slave_info->camera_id][0]);
		common_craete_proc_file(
		&g_otp_pdes[slave_info->camera_id]
		,target_file_name
		,g_sensor_ctrl[slave_info->camera_id]
		,&otp_fops);
	}
}
//otp--

//thermal++
enum{
	THERMAL_ERROR_NOT_SUPPORT =1,
	THERMAL_ERROR_INVALID_PARAMTER,
	THERMAL_ERROR_INVALID_STATE,
}sensor_read_temperature_error_type;

int sensor_read_temperature_sony(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc=0;
 	static int reg_data = -1;
	uint16_t reg_val = -1;  
    struct msm_camera_i2c_client *sensor_i2c_client;

	struct timeval tv_now;
	static struct timeval tv_prv;
	unsigned long long val;
	CDBG("sz_cam_fac E\n");

	if(s_ctrl == NULL){
		pr_err("sz_cam_fac s_ctrl is NULL!\n");
		reg_data = -THERMAL_ERROR_INVALID_PARAMTER;
	}else
	{
		if(s_ctrl->sensor_state != MSM_SENSOR_POWER_UP)
		{
			pr_err("sz_cam_fac camera Not power up!\n");
			reg_data = -THERMAL_ERROR_INVALID_STATE;
		}else
		{
			sensor_i2c_client = s_ctrl->sensor_i2c_client;
			if(sensor_i2c_client == NULL)
			{
				pr_err("sz_cam_fac sensor_i2c_client  is NULL!\n");
				reg_data = -THERMAL_ERROR_INVALID_PARAMTER;
			}else
			{
				do_gettimeofday(&tv_now);
				val = (tv_now.tv_sec*1000000000LL+tv_now.tv_usec*1000)-(tv_prv.tv_sec*1000000000LL+tv_prv.tv_usec*1000);
				//[sheldon] : read circle 50ms	
				if(val <= 50)
				{
					CDBG( "sz_cam_fac camera_thermal : cached (%d) val=%lld\n", reg_data, val);
				}else
				{ 
					mutex_lock(s_ctrl->msm_sensor_mutex);
					do{
						//[1] : Temperature control enable
						rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
						sensor_i2c_client, 0x0138, &reg_val, MSM_CAMERA_I2C_BYTE_DATA);
						if(rc<0) {
							pr_err("[camera_thermal]read enable failed!\n");break;
						}  

						rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
						sensor_i2c_client, 0x0138, (reg_val | 0x01), MSM_CAMERA_I2C_BYTE_DATA);
						if(rc<0) {
							pr_err("[camera_thermal] set enable failed!\n");break;
						} 

						//[2] : Temperature data output
						rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
						sensor_i2c_client, 0x013a, &reg_val, MSM_CAMERA_I2C_BYTE_DATA);
						reg_data =  (int)(reg_val&0xff);
						//pr_err("[camera_thermal] temperature = 0x%02x\n",reg_data);
						if(rc<0) {
							pr_err("[camera_thermal] %s: read reg failed\n", __func__);break;
						}
						CDBG("reg_data=%d\n",reg_data);
						tv_prv.tv_sec = tv_now.tv_sec;
						tv_prv.tv_usec = tv_now.tv_usec;

						//[3] : Temperature control disable
						rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
						sensor_i2c_client, 0x0138, &reg_val, MSM_CAMERA_I2C_BYTE_DATA);
						if(rc<0) {
							pr_err("[camera_thermal]read disable failed!\n");break;
						} 

						rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
						sensor_i2c_client, 0x0138, (reg_val | 0x00), MSM_CAMERA_I2C_BYTE_DATA);
						if(rc<0) {
							pr_err("[camera_thermal] set disable failed!\n");break;
						} 
					}while(0);
					mutex_unlock(s_ctrl->msm_sensor_mutex);
				}
			}
		}		
	}
	return reg_data;
}

static struct proc_dir_entry *g_thermal_pdes[MAX_CAMERAS]={0};
char *g_thermal_proc_file_name[50]={PROC_FILE_THERMAL_REAR,PROC_FILE_THERMAL_FRONT,
#ifdef ZE553KL
PROC_FILE_THERMAL_REAR_2,
#endif
#ifdef ZD552KL_PHOENIX
PROC_FILE_THERMAL_FRONT_2,
#endif
"invalid_thermal"};

int sensor_read_temperature(struct msm_sensor_ctrl_t *s_ctrl)
{
	int temp=-THERMAL_ERROR_NOT_SUPPORT;
	CDBG("sz_cam_fac s_ctrl=%p\n",s_ctrl);
	switch(s_ctrl->sensordata->slave_info->sensor_id)
	{
		case SENSOR_ID_IMX214:temp=sensor_read_temperature_sony(s_ctrl);break;
		case SENSOR_ID_IMX362:temp=sensor_read_temperature_sony(s_ctrl);break;
		case SENSOR_ID_IMX351:temp=sensor_read_temperature_sony(s_ctrl);break;
	}
	return temp;
}
int thermal_proc_read(struct seq_file *buf, void *v)
{
	int ret=0;
	struct msm_sensor_ctrl_t *s_ctrl=(struct msm_sensor_ctrl_t *)buf->private;
	CDBG("sz_cam_fac s_ctrl=%p\n",s_ctrl);
	ret+=seq_printf(buf ,"%d\n",sensor_read_temperature(s_ctrl));
	return ret;
}
int thermal_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, thermal_proc_read, PDE_DATA(inode));
}
struct file_operations thermal_fops = {
	.owner = THIS_MODULE,
	.open = thermal_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void thermal_remove_proc_files(struct msm_sensor_ctrl_t *s_ctrl)
{
	extern struct proc_dir_entry proc_root;
	char* target_file_name=g_thermal_proc_file_name[s_ctrl->sensordata->cam_slave_info->camera_id];
	CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
	{
		remove_proc_entry(target_file_name, &proc_root);
	}
}
void thermal_create_proc_files(struct msm_camera_sensor_slave_info *slave_info)
{
	char* target_file_name=g_thermal_proc_file_name[slave_info->camera_id];
	int ret=0;
	CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
	{
		ret=common_craete_proc_file(
			&g_thermal_pdes[slave_info->camera_id]
			,target_file_name
			,g_sensor_ctrl[slave_info->camera_id]
			,&thermal_fops);
	}
}
//thermal--
#if 0
//pdaf++
static struct proc_dir_entry *g_pdaf_pdes[MAX_CAMERAS]={0};
char* g_pdaf_infos[MAX_CAMERAS];
char *g_pdaf_proc_file_name_hades[50]={REAR_PROC_FILE_PDAF_INFO,FRONT_PROC_FILE_PDAF_INFO,"pdaf_invalid0","pdaf_invalid1"};
int pdaf_proc_read(struct seq_file *buf, void *v)
{
	int ret=0;
	struct msm_sensor_ctrl_t *s_ctrl=(struct msm_sensor_ctrl_t *)buf->private;
	CDBG("sz_cam_fac s_ctrl=%p\n",s_ctrl);
	if(!g_pdaf_infos[s_ctrl->sensordata->cam_slave_info->camera_id]){
		pr_err("pdaf buffer is empty cameraid=%d\n",s_ctrl->sensordata->cam_slave_info->camera_id);
	}else{
		ret+=seq_printf(buf, "%s",g_pdaf_infos[s_ctrl->sensordata->cam_slave_info->camera_id]);
		ret+=seq_printf(buf ,"\n");
	}
	return ret;
}
ssize_t pdaf_proc_write (struct file *filp, const char __user *buff, size_t size, loff_t *ppos)
{
	int ret=0;
	struct msm_sensor_ctrl_t *s_ctrl=(struct msm_sensor_ctrl_t *)PDE_DATA(file_inode(filp));
	CDBG("sz_cam_fac s_ctrl=%p\n",s_ctrl);
	memset(g_pdaf_infos[s_ctrl->sensordata->cam_slave_info->camera_id], 0, PDAF_INFO_MAX_SIZE);
	if(copy_from_user(g_pdaf_infos[s_ctrl->sensordata->cam_slave_info->camera_id], buff, size)){
		ret =  - EFAULT;
		pr_err("sz_cam_fac %s copy from us failed!\n", __func__);
	}else{
		*ppos += size;
		ret = size;
	}
	return ret;
}
int pdaf_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, pdaf_proc_read, PDE_DATA(inode));
}
struct file_operations pdaf_fops = {
	.owner = THIS_MODULE,
	.open = pdaf_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = pdaf_proc_write,
	.release = single_release,
};

void remove_pdaf_proc_files(struct msm_sensor_ctrl_t *s_ctrl)
{
	extern struct proc_dir_entry proc_root;
	CDBG("sz_cam_fac E s_ctrl=%p\n",s_ctrl);
	remove_proc_entry(g_pdaf_proc_file_name_hades[s_ctrl->sensordata->cam_slave_info->camera_id], &proc_root);
	if(g_pdaf_infos[s_ctrl->sensordata->cam_slave_info->camera_id])
	{
		kfree(g_pdaf_infos[s_ctrl->sensordata->cam_slave_info->camera_id]);
		g_pdaf_infos[s_ctrl->sensordata->cam_slave_info->camera_id]=NULL;
	}
}
void create_pdaf_proc_files(struct msm_camera_sensor_slave_info *slave_info)
{
	char* target_file_name=NULL;
	int ret=0;
	CDBG("creating module procfs entry\n");
	switch(asus_project_id)
	{
		case ASUS_ZE553KL:break;
		case ASUS_ZD552KL_PHOENIX:break;
		default:pr_err("sz_cam_fac invalid projectId:%d\n",asus_project_id);break;
	}
	if(strlen(target_file_name)>0)
	{
		ret=common_craete_proc_file(
			&g_pdaf_pdes[slave_info->camera_id]
			,target_file_name
			,g_sensor_ctrl[slave_info->camera_id]
			,&pdaf_fops);
		if(ret)
		{
			pr_err("create proc file failed ret=%d\n",ret);
		}else
		{
			g_pdaf_infos[slave_info->camera_id]=kzalloc(PDAF_INFO_MAX_SIZE, GFP_KERNEL);
			if(!g_pdaf_infos[slave_info->camera_id])
			{
				pr_err("not enough memory\n");
			}
		}
	}
}
//pdaf--
#endif

int power_supply_read_rear_temp(struct thermal_zone_device *tzd, unsigned long *temp){

	  if(g_sensor_id[CAMERA_0]==SENSOR_ID_IMX298 || g_sensor_id[CAMERA_0]==SENSOR_ID_IMX362 || g_sensor_id[CAMERA_0]==SENSOR_ID_IMX351){
		struct msm_sensor_ctrl_t *s_ctrl = g_sensor_ctrl[CAMERA_0];
	  	if(g_sensor_ctrl[CAMERA_0]->sensor_state == MSM_SENSOR_POWER_UP){
			*temp = sensor_read_temperature(s_ctrl);  //temperature func
			  if(*temp < 0 || *temp > 0xec || *temp == 0x80){
				pr_err("%s fail!\n", __func__);
				*temp=0;
				return 0;
			  }
			  if(*temp < 0x50){
				*temp = *temp;
			  } else if(*temp >= 0x50 && *temp < 0x80){
				*temp = 80;
			  }else  if(*temp >= 0x81 && *temp <= 0xec){
			  	*temp = -20;
			  }else{
			        *temp -= 0xed;
			        *temp = *temp -19;
			  }
			}else{
				*temp=0;
				pr_debug("[camera_thermal]%s g_sensor_ctrl[CAMERA_0] not power up!\n", __func__);
				return 0;
			}
	  }else {
	  	*temp=-1;
	  	pr_err("%s Unknow  sensor id!\n", __func__);
	  }
	return 0;
  }

int power_supply_read_front_temp(struct thermal_zone_device *tzd, unsigned long *temp){

	if(g_sensor_id[CAMERA_1]==SENSOR_ID_IMX298 || g_sensor_id[CAMERA_1]==SENSOR_ID_IMX214 || g_sensor_id[CAMERA_1]==SENSOR_ID_IMX362){
		struct msm_sensor_ctrl_t *s_ctrl = g_sensor_ctrl[CAMERA_1];
	  	if(g_sensor_ctrl[CAMERA_1]->sensor_state == MSM_SENSOR_POWER_UP){
		  *temp = sensor_read_temperature(s_ctrl); //temperature func
		  if(*temp < 0 || *temp > 0xec || *temp == 0x80){
			pr_err("%s fail!\n", __func__);
			*temp=0;
			return 0;
		  }
		  if(*temp < 0x50){
			*temp = *temp;
		  } else if(*temp >= 0x50 && *temp < 0x80){
			*temp = 80;
		  }else  if(*temp >= 0x81 && *temp <= 0xec){
		  	*temp = -20;
		  }else{
		        *temp -= 0xed;
		        *temp = *temp -19;
		  }
	  	}else{
				*temp=0;
				pr_debug("[camera_thermal]%s g_sensor_ctrl[CAMERA_1] not power up!\n", __func__);
				return 0;
		}
	  }else {
	  	*temp=-1;
	  	pr_err("%s Unknow  sensor id!\n", __func__);
	  }
	  return 0;
  }

struct thermal_zone_device_ops psy_tzd_rear_ops ={
	.get_temp = power_supply_read_rear_temp,
};

struct thermal_zone_device_ops psy_tzd_front_ops ={
	.get_temp = power_supply_read_front_temp,
};

//module++
static struct proc_dir_entry *g_module_pdes[MAX_CAMERAS]={0};
char *g_module_proc_file_name[50]={PROC_FILE_REARMODULE_1,PROC_FILE_FRONTMODULE,
#ifdef ZE553KL
PROC_FILE_REARMODULE_2,
#endif
#ifdef ZD552KL_PHOENIX
PROC_FILE_FRONTMODULE_2,
#endif
"module_invalid"};

int sensor_module_read(struct seq_file *buf, uint16_t sensor_id)
{
	switch (sensor_id) {
	case SENSOR_ID_IMX214: return seq_printf(buf, "IMX214\n");
	case SENSOR_ID_IMX298: return seq_printf(buf, "IMX298\n");
	case SENSOR_ID_IMX351: return seq_printf(buf, "IMX351\n");
	case SENSOR_ID_IMX362: return seq_printf(buf, "IMX362\n");
	case SENSOR_ID_OV5670: return seq_printf(buf, "OV5670\n");
	case SENSOR_ID_OV8856: return seq_printf(buf, "OV8856\n");
	case SENSOR_ID_S5K3M3: return seq_printf(buf, "S5K3M3\n");
	default: return 0;
	}
}
int modules_proc_read(struct seq_file *buf, void *v)
{
	int ret=0;
	struct msm_sensor_ctrl_t *s_ctrl=(struct msm_sensor_ctrl_t *)buf->private;
	CDBG("sz_cam_fac s_ctrl=%p\n",s_ctrl);
	if(!s_ctrl)
	{
		ret=seq_printf(buf, "invalid param\n");
		pr_err("slave_info is NULL\n");
	}
	else
	{
		ret=sensor_module_read(buf, s_ctrl->sensordata->slave_info->sensor_id);
	}
	return ret;
}
int module_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, modules_proc_read, PDE_DATA(inode));
}
struct file_operations camera_module_fops = {
	.owner = THIS_MODULE,
	.open = module_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void module_remove_proc_files(struct msm_sensor_ctrl_t *s_ctrl)
{
	extern struct proc_dir_entry proc_root;
	char* target_file_name=g_module_proc_file_name[s_ctrl->sensordata->cam_slave_info->camera_id];
	CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
		remove_proc_entry(target_file_name, &proc_root);
}
void module_create_proc_files(struct msm_camera_sensor_slave_info *slave_info)
{
	char* target_file_name=g_module_proc_file_name[slave_info->camera_id];
	CDBG("creating module procfs entry\n");
	if(strlen(target_file_name)>0)
		common_craete_proc_file(
		&g_module_pdes[slave_info->camera_id]
		,target_file_name
		,g_sensor_ctrl[slave_info->camera_id]
		,&camera_module_fops);
}
//module--

//resolution++
struct kobject *g_kobj_camera_resolutions[MAX_CAMERAS];
ssize_t resolutions_read(struct device *dev, struct device_attribute *attr, char *buf);
char *g_resolution_sysfs_name[50]={SYSFS_FILE_RESOLUTION_REAR_0,SYSFS_FILE_RESOLUTION_FRONT,
#ifdef ZE553KL
SYSFS_FILE_RESOLUTION_REAR_2,
#endif
#ifdef ZD552KL_PHOENIX
SYSFS_FILE_RESOLUTION_FRONT_2,
#endif
"resolution_invalid"};

static struct device_attribute dev_attr_camera_resolutions[MAX_CAMERAS] ={
	__ATTR(camera_resolution, S_IRUGO | S_IWUSR | S_IWGRP, resolutions_read, NULL),
	__ATTR(vga_resolution, S_IRUGO | S_IWUSR | S_IWGRP, resolutions_read, NULL),
	__ATTR(camera_2_resolution, S_IRUGO | S_IWUSR | S_IWGRP, resolutions_read, NULL),
	__ATTR(invalid_resolution, S_IRUGO | S_IWUSR | S_IWGRP, resolutions_read, NULL),
};

//EXPORT_SYMBOL_GPL(camera_resolutions);
ssize_t resolution_read(char *buf, int camera_id)
{
	switch (g_sensor_id[camera_id]) {
	case SENSOR_ID_IMX214: return sprintf(buf, "13M\n");
	case SENSOR_ID_IMX298: return sprintf(buf, "16M\n");
	case SENSOR_ID_IMX351: return sprintf(buf, "16M\n");
	case SENSOR_ID_IMX362: return sprintf(buf, "12M\n");
	case SENSOR_ID_OV5670: return sprintf(buf, "5M\n");
	case SENSOR_ID_OV8856: return sprintf(buf, "8M\n");
	case SENSOR_ID_S5K3M3: return sprintf(buf, "13M\n");
	default: return sprintf(buf,"None\n");;
	}
}
ssize_t resolutions_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret=0;
	int i=0;
	CDBG("sz_cam_fac E\n");
	if(!attr){
		ret=sprintf(buf, "invalid parameter\n");
		pr_err("sz_cam_fac invalid parameter\n");
	}else
	{
		for(i=0;i<MAX_CAMERAS;i++){
			if(!strcmp(attr->attr.name,dev_attr_camera_resolutions[i].attr.name))
				break;
		}
		if(i==MAX_CAMERAS|| i==MAX_CAMERAS-1)
		{
			ret=sprintf(buf, "can't find resolution\n");
			pr_err("can't find resolution for %s failed\n",attr->attr.name);
		}else
		{
			ret=resolution_read(buf, i);
		}
	}
	return ret;
}
void resolution_create_sysfs_files(struct msm_camera_sensor_slave_info *slave_info)
{
	char*target_file_name = g_resolution_sysfs_name[slave_info->camera_id];
	CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
		common_create_sysfs_file(
		&g_kobj_camera_resolutions[slave_info->camera_id]
		,target_file_name
		,&dev_attr_camera_resolutions[slave_info->camera_id]);
}
void resolution_remove_sysfs_files(struct msm_sensor_ctrl_t *s_ctrl)
{
	char* target_file_name=g_resolution_sysfs_name[s_ctrl->sensordata->cam_slave_info->camera_id];
	CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
		common_remove_sysfs_file(
		g_kobj_camera_resolutions[s_ctrl->sensordata->cam_slave_info->camera_id]
		,dev_attr_camera_resolutions[s_ctrl->sensordata->cam_slave_info->camera_id]);
}
//resolution--
//status++
struct kobject *g_kobj_camera_status[MAX_CAMERAS];
ssize_t status_read(struct device *dev, struct device_attribute *attr, char *buf);
char *g_status_sysfs_name[50]={SYSFS_FILE_STATUS_REAR_1,SYSFS_FILE_STATUS_FRONT,
#ifdef ZE553KL
SYSFS_FILE_STATUS_REAR_2,
#endif
#ifdef ZD552KL_PHOENIX
SYSFS_FILE_STATUS_FRONT_2,
#endif
"status_invalid"};

struct device_attribute dev_attr_camera_status[MAX_CAMERAS] = 
{
	__ATTR(camera_status, S_IRUGO | S_IWUSR | S_IWGRP, status_read, NULL),
	__ATTR(vga_status, S_IRUGO | S_IWUSR | S_IWGRP, status_read, NULL),
	__ATTR(camera_status_2, S_IRUGO | S_IWUSR | S_IWGRP, status_read, NULL),
	__ATTR(invalid_status, S_IRUGO | S_IWUSR | S_IWGRP, status_read, NULL)
};
//EXPORT_SYMBOL_GPL(camera_resolutions);
ssize_t sensor_status_read(char *buf,int cameraID)
{
	struct msm_sensor_ctrl_t *s_ctrl = g_sensor_ctrl[cameraID];
	if(s_ctrl != NULL)
	{
		if((msm_sensor_testi2c(s_ctrl))<0)
			return sprintf(buf, "%s\n%s\n", "ERROR: i2c r/w test fail","Driver version:");
		else
			return sprintf(buf, "%s\n%s\n%s0x%x\n", "ACK:i2c r/w test ok","Driver version:","sensor_id:",g_sensor_id[cameraID]);   
	}
	else{
		return sprintf(buf, "%s\n%s\n", "ERROR: i2c r/w test fail","Driver version:");
	}
}

ssize_t status_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret=0;
	int i=0;
	CDBG("sz_cam_fac E\n");
	if(!attr){
		ret=sprintf(buf, "invalid parameter\n");
		pr_err("sz_cam_fac invalid parameter\n");
	}else
	{
		for(i=0;i<MAX_CAMERAS;i++){
			if(!strcmp(attr->attr.name,dev_attr_camera_status[i].attr.name))
				break;
		}
		if(i==MAX_CAMERAS|| i==MAX_CAMERAS-1)
		{
			ret=sprintf(buf, "can't find status\n");
			pr_err("can't find status for %s failed\n",attr->attr.name);
		}else
		{
			ret=sensor_status_read(buf, i);
		}
	}
	return ret;
}
void status_create_sysfs_files(struct msm_camera_sensor_slave_info *slave_info)
{
	char *target_file_name = g_status_sysfs_name[slave_info->camera_id];
	CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
		common_create_sysfs_file(
		&g_kobj_camera_status[slave_info->camera_id]
		,target_file_name
		,&dev_attr_camera_status[slave_info->camera_id]);
}
void status_remove_sysfs_files(struct msm_sensor_ctrl_t *s_ctrl)
{
	char *target_file_name = g_status_sysfs_name[s_ctrl->sensordata->cam_slave_info->camera_id];
	CDBG("sz_cam_fac E\n");
	if(strlen(target_file_name)>0)
		common_remove_sysfs_file(
		g_kobj_camera_status[s_ctrl->sensordata->cam_slave_info->camera_id],
		dev_attr_camera_status[s_ctrl->sensordata->cam_slave_info->camera_id]);
}
//status--

#ifdef ZD552KL_PHOENIX
static uint8_t g_camera_id;
static uint16_t g_camera_reg_addr;
static uint32_t g_camera_reg_val;
static enum msm_camera_i2c_data_type g_camera_data_type = MSM_CAMERA_I2C_BYTE_DATA;
static uint8_t g_camera_sensor_operation;
static int sensor_i2c_debug_read(struct seq_file *buf, void *v)
{
	uint16_t reg_val = 0xEEEE;
	int rc;

	seq_printf(buf,"=========================\n\
Camera %d, PowerState %d\n\
Camera %d, PowerState %d\n\
Camera %d, PowerState %d\n\
=========================\n",
	CAMERA_0,g_sensor_ctrl[CAMERA_0]->sensor_state,
	CAMERA_1,g_sensor_ctrl[CAMERA_1]->sensor_state,
	CAMERA_2,g_sensor_ctrl[CAMERA_2]->sensor_state
    );
	if(g_sensor_ctrl[g_camera_id]->sensor_state != MSM_SENSOR_POWER_UP)
	{
		pr_err("Please power up Camera Sensor %d first!\n",g_camera_id);
		seq_printf(buf,"Camera ID %d POWER DOWN!\n",g_camera_id);
	}
	else
	{
		rc = g_sensor_ctrl[g_camera_id]->sensor_i2c_client->i2c_func_tbl->i2c_read(
				g_sensor_ctrl[g_camera_id]->sensor_i2c_client,
				g_camera_reg_addr,
				&reg_val,
				g_camera_data_type
			);

		if(g_camera_sensor_operation == 1)//write
		{
			if(reg_val == g_camera_reg_val)
			{
				pr_info("read back the same value as write!\n");
			}
			else
			{
				pr_err("write value 0x%x and read back value 0x%x not same!\n",
						g_camera_reg_val,reg_val
				);
			}
		}
		seq_printf(buf,"Camera %d reg 0x%04x val 0x%x\n",g_camera_id,g_camera_reg_addr,reg_val);
	}
	return 0;
}

static int sensor_i2c_debug_open(struct inode *inode, struct  file *file)
{
	return single_open(file, sensor_i2c_debug_read, NULL);
}

static ssize_t sensor_i2c_debug_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	ssize_t ret_len;
	int n;
	char messages[32]="";
	uint32_t val[4];
	int rc;

	ret_len = len;
	if (len > 32) {
		len = 32;
	}
	if (copy_from_user(messages, buff, len)) {
		pr_err("%s command fail !!\n", __func__);
		return -EFAULT;
	}

	n = sscanf(messages,"%x %x %x %x",&val[0],&val[1],&val[2],&val[3]);

	if(n < 3 || n > 4 )
	{
		rc = -1;
		pr_err("Invalid Argument count %d!\n",n);
		goto RETURN;
	}
	else if( n == 3)
	{
		//camera id, reg addr, data type
		g_camera_id = val[0];
		g_camera_reg_addr = val[1];
		if(val[2] == 1)
			g_camera_data_type = MSM_CAMERA_I2C_BYTE_DATA;
		else if(val[2] == 2)
			g_camera_data_type = MSM_CAMERA_I2C_WORD_DATA;
		else
			g_camera_data_type = MSM_CAMERA_I2C_BYTE_DATA;
		g_camera_sensor_operation = 0;
	}
	else if( n == 4)
	{
		//camera id, reg addr, data type, reg val
		g_camera_id = val[0];
		g_camera_reg_addr = val[1];
		if(val[2] == 1)
			g_camera_data_type = MSM_CAMERA_I2C_BYTE_DATA;
		else if(val[2] == 2)
			g_camera_data_type = MSM_CAMERA_I2C_WORD_DATA;
		else
			g_camera_data_type = MSM_CAMERA_I2C_BYTE_DATA;
		g_camera_reg_val = val[3];
		g_camera_sensor_operation = 1;
	}

	if(g_camera_id > CAMERA_2)
	{
		pr_err("Invalid Sensor ID %d!\n",g_camera_id);
		g_camera_id = CAMERA_0;//reset to default ID
		goto RETURN;
	}
	pr_info("Camera %d power state %d\n",CAMERA_0,g_sensor_ctrl[CAMERA_0]->sensor_state);
	pr_info("Camera %d power state %d\n",CAMERA_1,g_sensor_ctrl[CAMERA_1]->sensor_state);
	pr_info("Camera %d power state %d\n",CAMERA_2,g_sensor_ctrl[CAMERA_2]->sensor_state);
	if(g_sensor_ctrl[g_camera_id]->sensor_state != MSM_SENSOR_POWER_UP)
	{
		pr_err("Please power up Camera Sensor %d first!\n",g_camera_id);
		goto RETURN;
	}

	pr_info("gona %s Camera ID %d reg 0x%04x, data type %s\n",
			g_camera_sensor_operation ? "WRITE":"READ",
			g_camera_id,
			g_camera_reg_addr,
			g_camera_data_type == MSM_CAMERA_I2C_BYTE_DATA?"BYTE":"WORD"
			);


	if(g_camera_sensor_operation == 1)
	{
		rc = g_sensor_ctrl[g_camera_id]->sensor_i2c_client->i2c_func_tbl->i2c_write(
			g_sensor_ctrl[g_camera_id]->sensor_i2c_client,
			g_camera_reg_addr,
			g_camera_reg_val,
			g_camera_data_type
		);

		if(rc < 0)
		{
			pr_err("write 0x%x to camera id %d addr 0x%04x FAIL\n",g_camera_reg_val,g_camera_id,g_camera_reg_addr);
		}
		else
		{
			pr_err("write 0x%x to camera id %d addr 0x%04x OK\n",g_camera_reg_val,g_camera_id,g_camera_reg_addr);
		}
	}
RETURN:
	return ret_len;
}
static const struct file_operations sensor_i2c_debug_fops = {
	.owner = THIS_MODULE,
	.open = sensor_i2c_debug_open,
	.read = seq_read,
	.write = sensor_i2c_debug_write,
	.llseek = seq_lseek,
	.release = single_release,
};


static uint8_t g_pdaf_read_count=192;//default read all

static int read_imx351_pdaf_data_via_i2c(uint8_t read_count, uint16_t* pd_data, uint16_t* confidence_data, uint8_t* window_num)
{
	int i,j;
	int rc;
	uint8_t data[4];
	uint16_t high_word,low_word;
	uint16_t hold_val=0xEEEE;
	uint16_t  area_mode;

	struct msm_camera_i2c_seq_reg_array *read_setting = NULL;

	if(read_count*4 > I2C_SEQ_REG_DATA_MAX)
	{
		pr_err("read bytes %d too large, max seq size is %d\n",read_count*4,I2C_SEQ_REG_DATA_MAX);
		return -1;
	}

	read_setting = kzalloc(sizeof(struct msm_camera_i2c_seq_reg_array),GFP_KERNEL);
	if (!read_setting)
		return -ENOMEM;

	read_setting->reg_addr = 0xC808;
	read_setting->reg_data_size = read_count*4;

	//add mutex to prevent sensor shutdown
	mutex_lock(g_sensor_ctrl[CAMERA_0]->msm_sensor_mutex);
	rc = g_sensor_ctrl[CAMERA_0]->sensor_i2c_client->i2c_func_tbl->i2c_read(
		g_sensor_ctrl[CAMERA_0]->sensor_i2c_client,
		0x38A3,
		&area_mode,
		MSM_CAMERA_I2C_BYTE_DATA
	);
	if(area_mode == 0)
	{
		*window_num = 192;//16x12
	}
	else if(area_mode == 1)
	{
		*window_num = 48;//8x6
	}
	else
	{
		*window_num = 8;//flexible, max window
	}
	//hold  on
	rc = g_sensor_ctrl[CAMERA_0]->sensor_i2c_client->i2c_func_tbl->i2c_write(
		g_sensor_ctrl[CAMERA_0]->sensor_i2c_client,
		0xE213,
		0x1,
		MSM_CAMERA_I2C_BYTE_DATA
	);
	rc = g_sensor_ctrl[CAMERA_0]->sensor_i2c_client->i2c_func_tbl->i2c_read(
			g_sensor_ctrl[CAMERA_0]->sensor_i2c_client,
			0xE213,
			&hold_val,
			MSM_CAMERA_I2C_BYTE_DATA
		);
	pr_info("Hold on read back value is %d\n",hold_val);
	if(hold_val != 0x1)
	{
		pr_err("Error, read pdaf in hold off mode\n");
		rc = -2;
		goto END;
	}

	rc = g_sensor_ctrl[CAMERA_0]->sensor_i2c_client->i2c_func_tbl->i2c_read_seq(
			g_sensor_ctrl[CAMERA_0]->sensor_i2c_client,
			read_setting->reg_addr,
			read_setting->reg_data,
			read_setting->reg_data_size);
	if(rc < 0)
	{
		pr_err("read reg 0x%04x size %d failed, rc = %d\n",read_setting->reg_addr,read_setting->reg_data_size,rc);
		rc = -3;
		goto END;
	}
	//process pd data and confidence

	for(i=0;i<read_count;i++)
	{
		for(j=0;j<4;j++)
			data[j]=read_setting->reg_data[i*4+j];
		//PD data
		high_word = ((data[1] & 0x1f)<<6);
		low_word = data[2]>>2;
		pd_data[i] = high_word|low_word;

		//Confidence
		high_word = data[0]<<3;
		low_word =  data[1]>>5;
		confidence_data[i] = high_word|low_word;

		pr_info("Index[%03d], Raw(0x%02x 0x%02x 0x%02x 0x%02x) parsed to PD[0x%04x], Conf[0x%04x]\n",
				i,
				data[0],data[1],data[2],data[3],
				pd_data[i],
				confidence_data[i]
			   );
	}
	rc = 0;
END:
	//hold off
	rc = g_sensor_ctrl[CAMERA_0]->sensor_i2c_client->i2c_func_tbl->i2c_write(
		g_sensor_ctrl[CAMERA_0]->sensor_i2c_client,
		0xE213,
		0x0,
		MSM_CAMERA_I2C_BYTE_DATA
	);

	mutex_unlock(g_sensor_ctrl[CAMERA_0]->msm_sensor_mutex);
	kfree(read_setting);
	read_setting = NULL;
	return rc;
}

static int pdaf_i2c_debug_read(struct seq_file *buf, void *v)
{
	int i;
	int rc;
	uint16_t* pd_data = NULL;
	uint16_t* conf_data = NULL;
	uint8_t window_number;
	uint8_t valid_window_number=0;
	seq_printf(buf,"=========================\n\
Camera %d, PowerState %d\n\
Camera %d, PowerState %d\n\
Camera %d, PowerState %d\n\
=========================\n",
	CAMERA_0,g_sensor_ctrl[CAMERA_0]->sensor_state,
	CAMERA_1,g_sensor_ctrl[CAMERA_1]->sensor_state,
	CAMERA_2,g_sensor_ctrl[CAMERA_2]->sensor_state
    );
	if(g_sensor_ctrl[CAMERA_0]->sensor_state != MSM_SENSOR_POWER_UP)
	{
		pr_err("Please power up Camera Sensor %d first!\n",CAMERA_0);
		seq_printf(buf,"Camera ID %d POWER DOWN!\n",g_camera_id);
	}
	else
	{
		pd_data = kzalloc(sizeof(uint16_t)*g_pdaf_read_count,GFP_KERNEL);
		conf_data = kzalloc(sizeof(uint16_t)*g_pdaf_read_count,GFP_KERNEL);
		if(!pd_data||!conf_data)
		{
			seq_printf(buf,"No memory left!\n");
		}
		else
		{
			rc = read_imx351_pdaf_data_via_i2c(g_pdaf_read_count,pd_data,conf_data,&window_number);
			if(!rc)
			{
				for(i=0;i<g_pdaf_read_count;i++)
				{
					if(pd_data[i]!=0x0555)
					{
						seq_printf(buf,"Index[%03d], Grid[%02d,%02d], PD[0x%04x], Conf[%d]\n",
						i,
						i/16,i%16,
						pd_data[i],
						conf_data[i]
						);
						valid_window_number++;
					}
				}
				seq_printf(buf,"================================================\n");
				seq_printf(buf,"window number configed %d, valid window %d\n",window_number,valid_window_number);
			}
			else
			{
				seq_printf(buf,"Read PD data failed, rc=%d\n",rc);
			}

			kfree(pd_data);
			kfree(conf_data);
		}
	}
	return 0;
}

static int pdaf_i2c_debug_open(struct inode *inode, struct  file *file)
{
	return single_open(file, pdaf_i2c_debug_read, NULL);
}

static ssize_t pdaf_i2c_debug_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	ssize_t ret_len;
	char messages[32]="";
	uint8_t val;

	ret_len = len;
	if (len > 32) {
		len = 32;
	}
	if (copy_from_user(messages, buff, len)) {
		pr_err("%s command fail !!\n", __func__);
		return -EFAULT;
	}

	sscanf(messages,"%hhd",&val);

	if(val <= 0 || val > 192)
	{
		pr_err("Invalid pdaf read count %d!\n",val);
		goto RETURN;
	}
	g_pdaf_read_count = val;

	pr_info("gona read %d count pdaf data from imx351\n",g_pdaf_read_count);

RETURN:
	return ret_len;
}
static const struct file_operations pdaf_i2c_debug_fops = {
	.owner = THIS_MODULE,
	.open = pdaf_i2c_debug_open,
	.read = seq_read,
	.write = pdaf_i2c_debug_write,
	.llseek = seq_lseek,
	.release = single_release,
};

//ASUS_BSP++, LLHDR
static uint32_t preisp_vhdr_mode;
static int preisp_vhdr_mode_read(struct seq_file *buf, void *v)
{
	seq_printf(buf,"%d\n",preisp_vhdr_mode);
	return 0;
}

static int preisp_vhdr_mode_open(struct inode *inode, struct  file *file)
{
	return single_open(file, preisp_vhdr_mode_read, NULL);
}

static ssize_t preisp_vhdr_mode_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	ssize_t ret_len;
	char messages[32]="";
	uint32_t val;

	ret_len = len;
	if (len > 32) {
		len = 32;
	}
	if (copy_from_user(messages, buff, len)) {
		pr_err("%s command fail !!\n", __func__);
		return -EFAULT;
	}

	sscanf(messages,"%d",&val);

	preisp_vhdr_mode = val;

	pr_info("preisp_vhdr_mode set to %d\n",preisp_vhdr_mode);

	return ret_len;
}
static const struct file_operations preisp_vhdr_mode_debug_fops = {
	.owner = THIS_MODULE,
	.open = preisp_vhdr_mode_open,
	.read = seq_read,
	.write = preisp_vhdr_mode_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static uint32_t preisp_cframe_id;
static int preisp_cframe_id_read(struct seq_file *buf, void *v)
{
	seq_printf(buf,"%d\n",preisp_cframe_id);
	return 0;
}

static int preisp_cframe_id_open(struct inode *inode, struct  file *file)
{
	return single_open(file, preisp_cframe_id_read, NULL);
}

static ssize_t preisp_cframe_id_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	ssize_t ret_len;
	char messages[32]="";
	uint32_t val;

	ret_len = len;
	if (len > 32) {
		len = 32;
	}
	if (copy_from_user(messages, buff, len)) {
		pr_err("%s command fail !!\n", __func__);
		return -EFAULT;
	}

	sscanf(messages,"%d",&val);

	preisp_cframe_id = val;

	pr_info("preisp_cframe_id set to %d\n",preisp_cframe_id);

	return ret_len;
}
static const struct file_operations preisp_cframe_id_debug_fops = {
	.owner = THIS_MODULE,
	.open = preisp_cframe_id_open,
	.read = seq_read,
	.write = preisp_cframe_id_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static uint32_t preisp_pframe_id;
static int preisp_pframe_id_read(struct seq_file *buf, void *v)
{
	seq_printf(buf,"%d\n",preisp_pframe_id);
	return 0;
}

static int preisp_pframe_id_open(struct inode *inode, struct  file *file)
{
	return single_open(file, preisp_pframe_id_read, NULL);
}

static ssize_t preisp_pframe_id_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	ssize_t ret_len;
	char messages[32]="";
	uint32_t val;

	ret_len = len;
	if (len > 32) {
		len = 32;
	}
	if (copy_from_user(messages, buff, len)) {
		pr_err("%s command fail !!\n", __func__);
		return -EFAULT;
	}

	sscanf(messages,"%d",&val);

	preisp_pframe_id = val;

	pr_info("preisp_pframe_id set to %d\n",preisp_pframe_id);

	return ret_len;
}
static const struct file_operations preisp_pframe_id_debug_fops = {
	.owner = THIS_MODULE,
	.open = preisp_pframe_id_open,
	.read = seq_read,
	.write = preisp_pframe_id_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static uint32_t preisp_sframe_id;
static int preisp_sframe_id_read(struct seq_file *buf, void *v)
{
	seq_printf(buf,"%d\n",preisp_sframe_id);
	return 0;
}

static int preisp_sframe_id_open(struct inode *inode, struct  file *file)
{
	return single_open(file, preisp_sframe_id_read, NULL);
}

static ssize_t preisp_sframe_id_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	ssize_t ret_len;
	char messages[32]="";
	uint32_t val;

	ret_len = len;
	if (len > 32) {
		len = 32;
	}
	if (copy_from_user(messages, buff, len)) {
		pr_err("%s command fail !!\n", __func__);
		return -EFAULT;
	}

	sscanf(messages,"%d",&val);

	preisp_sframe_id = val;

	pr_info("preisp_sframe_id set to %d\n",preisp_sframe_id);

	return ret_len;
}
static const struct file_operations preisp_sframe_id_debug_fops = {
	.owner = THIS_MODULE,
	.open = preisp_sframe_id_open,
	.read = seq_read,
	.write = preisp_sframe_id_write,
	.llseek = seq_lseek,
	.release = single_release,
};
//ASUS_BSP--, LLHDR

static uint32_t iface_process_frame_id;
static int iface_process_frame_id_read(struct seq_file *buf, void *v)
{
	seq_printf(buf,"%d\n",iface_process_frame_id);
	return 0;
}

static int iface_process_frame_id_open(struct inode *inode, struct  file *file)
{
	return single_open(file, iface_process_frame_id_read, NULL);
}

static ssize_t iface_process_frame_id_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	ssize_t ret_len;
	char messages[32]="";
	uint32_t val;

	ret_len = len;
	if (len > 32) {
		len = 32;
	}
	if (copy_from_user(messages, buff, len)) {
		pr_err("%s command fail !!\n", __func__);
		return -EFAULT;
	}

	sscanf(messages,"%d",&val);

	iface_process_frame_id = val;

	//pr_info("iface_process_frame_id set to %d\n",iface_process_frame_id);

	return ret_len;
}
static const struct file_operations iface_process_frame_id_debug_fops = {
	.owner = THIS_MODULE,
	.open = iface_process_frame_id_open,
	.read = seq_read,
	.write = iface_process_frame_id_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static void create_proc_file(const char *PATH,const struct file_operations* f_ops)
{
	struct proc_dir_entry *fd;

	fd = proc_create(PATH, 0666, NULL, f_ops);
	if(fd)
	{
		pr_info("%s(%s) succeeded!\n", __func__,PATH);
	}
	else
	{
		pr_err("%s(%s) failed!\n", __func__,PATH);
	}
}
static void sensor_i2c_rw_create_proc_file(void)
{
	static uint8_t has_created = 0;

	if(!has_created)
	{
		create_proc_file(PROC_SENSOR_I2C_RW,&sensor_i2c_debug_fops);
		has_created = 1;
	}
	else
		pr_err("proc file %s already created!\n",PROC_SENSOR_I2C_RW);
}
static void pdaf_i2c_create_proc_file(void)
{
	static uint8_t has_created = 0;

	if(!has_created)
	{
		create_proc_file(PROC_PDAF_I2C_R,&pdaf_i2c_debug_fops);
		has_created = 1;
	}
	else
		pr_err("proc file %s already created!\n",PROC_PDAF_I2C_R);
}

static void iface_process_frame_id_create_proc_file(void)
{
	static uint8_t has_created = 0;

	if(!has_created)
	{
		create_proc_file(PROC_IFACE_PROCESS_FRAME_ID,&iface_process_frame_id_debug_fops);
		has_created = 1;
	}
	else
		pr_info("proc file %s already created!\n",PROC_IFACE_PROCESS_FRAME_ID);
}

//ASUS_BSP++, LLHDR
static void preisp_vhdr_mode_create_proc_file(void)
{
	static uint8_t has_created = 0;

	if(!has_created)
	{
		create_proc_file(PROC_PREISP_VHDR_MODE,&preisp_vhdr_mode_debug_fops);
		has_created = 1;
	}
	else
		pr_info("proc file %s already created!\n",PROC_PREISP_VHDR_MODE);
}

static void preisp_cframe_id_create_proc_file(void)
{
	static uint8_t has_created = 0;

	if(!has_created)
	{
		create_proc_file(PROC_PREISP_CFRAME_ID,&preisp_cframe_id_debug_fops);
		has_created = 1;
	}
	else
		pr_info("proc file %s already created!\n",PROC_PREISP_CFRAME_ID);
}

static void preisp_pframe_id_create_proc_file(void)
{
	static uint8_t has_created = 0;

	if(!has_created)
	{
		create_proc_file(PROC_PREISP_PFRAME_ID,&preisp_pframe_id_debug_fops);
		has_created = 1;
	}
	else
		pr_info("proc file %s already created!\n",PROC_PREISP_PFRAME_ID);
}

static void preisp_sframe_id_create_proc_file(void)
{
	static uint8_t has_created = 0;

	if(!has_created)
	{
		create_proc_file(PROC_PREISP_SFRAME_ID,&preisp_sframe_id_debug_fops);
		has_created = 1;
	}
	else
		pr_info("proc file %s already created!\n",PROC_PREISP_SFRAME_ID);
}
//ASUS_BSP--, LLHDR
#endif

//camera_res +++
#ifdef ZE553KL
static void create_proc_file(const char *PATH,const struct file_operations* f_ops)
{
		struct proc_dir_entry *fd;
		fd = proc_create(PATH, 0666, NULL, f_ops);
		if(fd)
		{
			pr_info("%s(%s) succeeded!\n", __func__,PATH);
		}
		else
		{
			pr_err("%s(%s) failed!\n", __func__,PATH);
		}
}

void camera_res_remove_file(void)
{
	  extern struct proc_dir_entry proc_root;
	  CDBG("sz_cam_fac E\n");
	  remove_proc_entry(PROC_FILE_CAMERA_RES_INFO, &proc_root);
}

#define MAX_CAMERA_ID 2
typedef struct
{
	uint16_t id;
	char     name[8];
	uint8_t  resolution;//M
	uint32_t test_reg_addr;//reg to do i2c R/W test
}sensor_db_t;

static sensor_db_t g_sensor_info[] =
{
	{SENSOR_ID_IMX214, "IMX214", 13, 0x0340},
	//{SENSOR_ID_IMX298, "IMX298", 16, 0x0340},
	//{SENSOR_ID_IMX351, "IMX351", 16, 0x0340},
	{SENSOR_ID_IMX362, "IMX362", 12, 0x0340},
	//{SENSOR_ID_IMX363, "IMX363", 12, 0x0340},
	//{SENSOR_ID_OV5670, "OV5670", 5, 0x380e},
	//{SENSOR_ID_OV8856, "OV8856", 8, 0x380e},
	{SENSOR_ID_S5K3M3, "S5K3M3", 13, 0x0340},
};

typedef enum
{
	DIRECTION_REAR,
	DIRECTION_FRONT
}sensor_direct_t;

typedef struct
{
	uint16_t         sensor_id;
	sensor_direct_t  direction;
}sensor_define_t;


static void sort_cam_res(uint8_t* cam_res, int size)
{
	int i,j;
	uint8_t temp;
	for(i=0;i<size-1;i++)
	{
		for(j=0;j<size-1-i;j++)//little to tail
		{
			if(cam_res[j]<cam_res[j+1])
			{
				temp = cam_res[j];
				cam_res[j] = cam_res[j+1];
				cam_res[j+1] = temp;
			}
		}
	}
}

static uint8_t get_sensor_resolution(uint16_t sensor_id)
{
	int i;
	for(i=0;i<sizeof(g_sensor_info)/sizeof(sensor_db_t);i++)
	{
		if(g_sensor_info[i].id == sensor_id)
		{
			return g_sensor_info[i].resolution;
		}
	}
	return 0;
}

static sensor_define_t project_camera[MAX_CAMERA_ID+1] = {
	{SENSOR_ID_IMX362, DIRECTION_REAR},
	{SENSOR_ID_IMX214, DIRECTION_FRONT},
	{SENSOR_ID_S5K3M3, DIRECTION_REAR},
};
static char g_sensors_res_string[64];

static void fill_sensors_res_string(void)
{
	//asume each side has MAX_CAMERA_ID+1 cameras
	uint8_t front_cam_res[MAX_CAMERA_ID+1];
	uint8_t rear_cam_res[MAX_CAMERA_ID+1];

	int i;
	int front_count=0;
	int rear_count=0;

	char *p = g_sensors_res_string;
	int offset = 0;

	memset(front_cam_res,0,sizeof(front_cam_res));
	memset(rear_cam_res,0,sizeof(rear_cam_res));

	for(i=0;i<MAX_CAMERA_ID+1;i++)
	{
		if(project_camera[i].direction == DIRECTION_FRONT)
		{
			front_cam_res[front_count++] = get_sensor_resolution(project_camera[i].sensor_id);
		}
		else if(project_camera[i].direction == DIRECTION_REAR)
		{
			rear_cam_res[rear_count++] = get_sensor_resolution(project_camera[i].sensor_id);
		}
	}

	sort_cam_res(front_cam_res,front_count);
	sort_cam_res(rear_cam_res,rear_count);

	//format: front0+front1+...+frontN-rear0+rear2+...+rearN
	for(i=0;i<front_count;i++)
	{
		if(i==0)
			offset+=sprintf(p+offset, "%dM",front_cam_res[i]);
		else
			offset+=sprintf(p+offset, "+%dM",front_cam_res[i]);
	}

	for(i=0;i<rear_count;i++)
	{
		if(i==0)
			offset+=sprintf(p+offset, "-%dM",rear_cam_res[i]);
		else
			offset+=sprintf(p+offset, "+%dM",rear_cam_res[i]);
	}

	offset+=sprintf(p+offset,"\n");
}

static int sensors_res_proc_read(struct seq_file *buf, void *v)
{
	seq_printf(buf, "%s",g_sensors_res_string);
	return 0;
}

static int sensors_res_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, sensors_res_proc_read, NULL);
}

static struct file_operations sensors_res_fops = {
	.owner = THIS_MODULE,
	.open = sensors_res_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void sensors_res_create(void)
{
	static uint8_t has_created = 0;
	fill_sensors_res_string();
	if(!has_created)
	{
		create_proc_file(PROC_FILE_CAMERA_RES_INFO,&sensors_res_fops);
		has_created = 1;
	}
}

//camera_res ---
#endif

void create_sensor_proc_files(struct msm_sensor_ctrl_t *s_ctrl,struct msm_camera_sensor_slave_info *slave_info)
{
	static int first_init=1;
	if(first_init)
	{
		first_init=0;
		memset(g_otp_data_banks,0,sizeof(g_otp_data_banks));
	}
	if(!slave_info  ||  !s_ctrl)
	{
		pr_err("sz_cam_fac sensor info is NULL\n");return;
	}
	g_sensor_id[slave_info->camera_id]=slave_info->sensor_id_info.sensor_id;
	g_sensor_ctrl[slave_info->camera_id]=s_ctrl;
	CDBG("sz_cam_fac cameraid=%d,ctrl=%p\n",slave_info->camera_id,g_sensor_ctrl[slave_info->camera_id]);
	thermal_create_proc_files(slave_info);
	module_create_proc_files(slave_info);
	status_create_sysfs_files(slave_info);
	resolution_create_sysfs_files(slave_info);
	otp_create_proc_files(slave_info);
	eeprom_thermal_create_proc_files(slave_info);
	arcsoft_dualCali_create_proc_file(slave_info);
#ifdef ZE553KL
	sensors_res_create();
#endif
#ifdef ZD552KL_PHOENIX
	iface_process_frame_id_create_proc_file();
//ASUS_BSP++, LLHDR
	preisp_vhdr_mode_create_proc_file();
	preisp_cframe_id_create_proc_file();
	preisp_pframe_id_create_proc_file();
	preisp_sframe_id_create_proc_file();
//ASUS_BSP--, LLHDR
	if(g_sensor_ctrl[CAMERA_2])
	{
		sensor_i2c_rw_create_proc_file();
		pdaf_i2c_create_proc_file();
	}
#endif
}
void remove_sensor_proc_files(struct msm_sensor_ctrl_t *s_ctrl)
{
	arcsoft_dualCali_remove_file();
	eeprom_thermal_remove_proc_files(s_ctrl);
	otp_remove_proc_files(s_ctrl);
	resolution_remove_sysfs_files(s_ctrl);
	status_remove_sysfs_files(s_ctrl);
	module_remove_proc_files(s_ctrl);
	thermal_remove_proc_files(s_ctrl);
#ifdef ZE553KL
	camera_res_remove_file();
#endif
}
//asus bsp ralf:porting camera sensor related proc files<<


