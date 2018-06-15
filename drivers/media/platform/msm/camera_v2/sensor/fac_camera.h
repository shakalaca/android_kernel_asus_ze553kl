#ifndef  FAC_CAMERA_H_INCLUDED
#define FAC_CAMERA_H_INCLUDED

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
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time.h>
#include <soc/qcom/camera2.h>
#include "msm_sensor.h"
#include "actuator/msm_actuator.h"
#include "eeprom/msm_eeprom.h"
#include <linux/thermal.h>

#undef CDBG
#undef pr_fmt
#define pr_fmt(fmt) "%s:%d " fmt, __func__, __LINE__

#define  HADES_OTP_MODE   //define for hades rear otp read

#define SENSOR_ID_IMX298  0x0298
#define SENSOR_ID_T4K37   0x1c21
#define SENSOR_ID_T4K35   0x1481
#define SENSOR_ID_OV5670  0x5670
#define SENSOR_ID_OV8856  0x885a
#define SENSOR_ID_IMX362  0x0362
#define SENSOR_ID_S5K3M3  0x30D3
#define SENSOR_ID_IMX214  0x0214
#define SENSOR_ID_IMX351  0x0351

#define OTP_DATA_LEN_WORD (32)
#define OTP_DATA_LEN_BYTE (OTP_DATA_LEN_WORD*2)

#define	PROC_FILE_OTP_REAR_1  "driver/rear_otp"
#define	PROC_FILE_OTP_REAR_2  "driver/rear2_otp"
#define	PROC_FILE_OTP_FRONT	"driver/front_otp"
#define	PROC_FILE_OTP_FRONT_2	"driver/front2_otp"

#define	PROC_FILE_STATUS_BANK1_REAR_1	"driver/rear_otp_bank1"
#define	PROC_FILE_STATUS_BANK1_REAR_2	"driver/rear2_otp_bank1"
#define	PROC_FILE_STATUS_BANK1_FRONT	"driver/front_otp_bank1"

#define	PROC_FILE_STATUS_BANK2_REAR_1	"driver/rear_otp_bank2"
#define	PROC_FILE_STATUS_BANK2_REAR_2	"driver/rear2_otp_bank2"
#define	PROC_FILE_STATUS_BANK2_FRONT	"driver/front_otp_bank2"

#define	PROC_FILE_STATUS_BANK3_REAR_1	"driver/rear_otp_bank3"
#define	PROC_FILE_STATUS_BANK3_REAR_2	"driver/rear2_otp_bank3"
#define	PROC_FILE_STATUS_BANK3_FRONT	"driver/front_otp_bank3"

#define	PROC_FILE_REARMODULE_1	"driver/RearModule"
#define	PROC_FILE_REARMODULE_2	"driver/RearModule2"
#define	PROC_FILE_FRONTMODULE	"driver/FrontModule"
#define	PROC_FILE_FRONTMODULE_2	"driver/FrontModule2"


#define REAR_PROC_FILE_PDAF_INFO	"driver/PDAF_value_more_info"
#define FRONT_PROC_FILE_PDAF_INFO	"driver/PDAF_value_more_info_1"

#define PROC_FILE_CAMERA_RES_INFO	"driver/camera_res"

//for thermal team

//for DIT
#define   PROC_FILE_THERMAL_REAR  "driver/rear_temp"
#define   PROC_FILE_THERMAL_FRONT  "driver/front_temp"
#define   PROC_FILE_THERMAL_REAR_2  "driver/rear2_temp"
#define   PROC_FILE_THERMAL_FRONT_2  "driver/front2_temp"

#define   PROC_FILE_EEPEOM_THERMAL_REAR  "driver/rear_eeprom_temp"
#define   PROC_FILE_EEPEOM_THERMAL_FRONT  "driver/front_eeprom_temp"
#define   PROC_FILE_EEPEOM_THERMAL_REAR_2  "driver/rear2_eeprom_temp"

#define   PROC_FILE_EEPEOM_ARCSOFT  "driver/dualcam_cali"
#define SYSFS_FILE_RESOLUTION_REAR_0 "camera_resolution_camera"
#define SYSFS_FILE_RESOLUTION_REAR_2 "camera_resolution_camera_2"
#define SYSFS_FILE_RESOLUTION_FRONT "camera_resolution_vga"
#define SYSFS_FILE_RESOLUTION_FRONT_2 "camera_resolution_vga_2"


//asus bsp ralf:porting camera sensor related proc files>>
#define VCM_PROC_FILES_REAR "driver/vcm"
#define VCM_PROC_FILES_FRONT "driver/front_vcm"
#define VCM_PROC_FILES_REAR_2 "driver/vcm2"
#define VCM_PROC_FILES_FRONT_2 "driver/front_vcm2"


#define	EEPROM_PROC_FILE_REAR		"driver/rear_eeprom"
#define	EEPROM_PROC_FILE_FRONT		"driver/front_eeprom"
#define	EEPROM_PROC_FILE_REAR_2		"driver/rear2_eeprom"
#define	EEPROM_PROC_FILE_FRONT_2	"driver/front2_eeprom"

#define SYSFS_FILE_STATUS_REAR_1  "camera_status_camera"
#define SYSFS_FILE_STATUS_REAR_2  "camera_status_camera_2"
#define SYSFS_FILE_STATUS_FRONT   "camera_status_vga"
#define SYSFS_FILE_STATUS_FRONT_2   "camera_status_vga_2"

#ifdef ZD552KL_PHOENIX
#define PROC_SENSOR_I2C_RW "driver/sensor_i2c_rw"
#define PROC_PDAF_I2C_R "driver/pdaf_i2c_r"
//ASUS_BSP++, LLHDR
#define PROC_PREISP_VHDR_MODE "driver/preisp_vhdr_mode"
#define PROC_PREISP_CFRAME_ID "driver/preisp_cframe_id"
#define PROC_PREISP_PFRAME_ID "driver/preisp_pframe_id"
#define PROC_PREISP_SFRAME_ID "driver/preisp_sframe_id"
//ASUS_BSP--, LLHDR
#define PROC_IFACE_PROCESS_FRAME_ID "driver/iface_process_frame_id"
#endif

void remove_sensor_proc_files(struct msm_sensor_ctrl_t *s_ctrl);
void create_sensor_proc_files(struct msm_sensor_ctrl_t *s_ctrl,struct msm_camera_sensor_slave_info *slave_info);
void eeprom_create_proc_file(struct msm_eeprom_ctrl_t * e_ctrl);
void eeprom_remove_file(uint32_t subdev_id);
void vcm_create_proc_file(struct msm_actuator_ctrl_t *vcm_ctrl);
//asus bsp ralf:porting camera sensor related proc files<<

#endif
