#ifndef ASUS_OIS_H
#define ASUS_OIS_H

//#include "msm_sd.h" //should be in msm_ois.h!
#include "msm_ois.h"
//#include "msm_cci.h" //should be in msm_ois.h!
#include "msm_sensor.h"
#include "msm_eeprom.h"
extern unsigned char g_ois_status;
extern uint8_t g_ois_i2c_block_other;
extern uint8_t g_ois_mode;
extern uint8_t g_ois_power_state;
extern uint32_t g_ois_reg_fw_version;

void asus_ois_init(struct msm_ois_ctrl_t * ctrl);
extern struct msm_sensor_ctrl_t ** get_msm_sensor_ctrls(void);
extern struct msm_eeprom_ctrl_t * get_eeprom_ctrl(void);
extern void Laser_set_enforce_value(uint32_t value);
extern void Laser_switch_measure_mode(int val);

#endif
