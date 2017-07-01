#ifndef ASUS_OIS_RUMBA_INTERFACE_H
#define ASUS_OIS_RUMBA_INTERFACE_H

#include "msm_ois.h"

typedef enum
{
  OIS_DATA,
  PID_PARAM,
  USER_DATA
}flash_write_data_type;

typedef enum
{
	INIT_STATE=0,
	IDLE_STATE,
	RUN_STATE,
	HALL_CALI_STATE,
	HALL_POLA_STATE,
	GYRO_CALI_STATE,
	RESERVED1_STATE,
	RESERVED2_STATE,
	PWM_DUTY_FIX_STATE,
	DFLS_UPDATE_STATE,//user data area update state
	STANDBY_STATE,//low power consumption state
	GYRO_COMM_CHECK_STATE,
	RESERVED3_STATE,
	RESERVED4_STATE,
	LOOP_GAIN_CTRL_STATE,
	RESERVED5_STATE,
	GYRO_SELF_TEST_STATE,
	OIS_DATA_WRITE_STATE,
	PID_PARAM_INIT_STATE,
	GYRO_WAKEUP_WAIT_STATE
}rumba_ois_state;

rumba_ois_state rumba_get_state(struct msm_ois_ctrl_t * ctrl);
const char * rumba_get_state_str(rumba_ois_state state);

int rumba_idle2standy_state(struct msm_ois_ctrl_t * ctrl);
int rumba_standy2idle_state(struct msm_ois_ctrl_t * ctrl);

int rumba_servo_go_on(struct msm_ois_ctrl_t * ctrl);
int rumba_servo_go_off(struct msm_ois_ctrl_t * ctrl);

int rumba_get_servo_state(struct msm_ois_ctrl_t * ctrl, int *state);
int rumba_restore_servo_state(struct msm_ois_ctrl_t * ctrl, int state);

int rumba_fw_info_enable(struct msm_ois_ctrl_t * ctrl);
int rumba_fw_info_disable(struct msm_ois_ctrl_t * ctrl);

int rumba_switch_mode(struct msm_ois_ctrl_t *ctrl, uint8_t mode);

int rumba_set_i2c_mode(struct msm_ois_ctrl_t *ctrl,enum i2c_freq_mode_t mode);

int rumba_gyro_read_data(struct msm_ois_ctrl_t * ctrl,uint16_t* gyro_data, uint32_t size);
int rumba_gyro_read_K_related_data(struct msm_ois_ctrl_t * ctrl,uint16_t* gyro_data, uint32_t size);
int rumba_gyro_read_xy(struct msm_ois_ctrl_t * ctrl,uint16_t *x_value, uint16_t *y_value);

int rumba_read_pair_sensor_data(struct msm_ois_ctrl_t * ctrl,
								 uint16_t reg_addr_x,uint16_t reg_addr_y,
								 uint16_t *value_x,uint16_t *value_y);

int rumba_read_acc_position(struct msm_ois_ctrl_t * ctrl,uint16_t *x_value, uint16_t *y_value);


int rumba_gyro_calibration(struct msm_ois_ctrl_t * ctrl);

int rumba_gyro_communication_check(struct msm_ois_ctrl_t * ctrl);

int rumba_gyro_adjust_gain(struct msm_ois_ctrl_t * ctrl);
int rumba_hall_calibration(struct msm_ois_ctrl_t * ctrl);
int rumba_hall_check_polarity(struct msm_ois_ctrl_t * ctrl);
int rumba_measure_loop_gain(struct msm_ois_ctrl_t * ctrl);
int rumba_adjust_loop_gain(struct msm_ois_ctrl_t * ctrl);

int rumba_check_flash_write_result(struct msm_ois_ctrl_t * ctrl,flash_write_data_type type);


int rumba_init_PID_params(struct msm_ois_ctrl_t * ctrl);
int rumba_init_all_params(struct msm_ois_ctrl_t * ctrl);

int rumba_backup_ois_data_to_file(struct msm_ois_ctrl_t * ctrl, char * file);
int rumba_restore_ois_data_from_file(struct msm_ois_ctrl_t * ctrl, char * file);

int rumba_backup_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_addr, uint32_t size, uint8_t *buffer);
int rumba_compare_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_addr, uint32_t size, uint8_t *buffer);

int rumba_write_user_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_offset,uint8_t* data, uint32_t size);
int rumba_read_user_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_offset,uint8_t* data, uint32_t size);

int rumba_get_bin_fw_revision(const char * file_path, uint32_t* read_fw_rev);
int rumba_can_update_fw(uint32_t dev_fw_rev, uint32_t bin_fw_rev, int force_update);
int rumba_update_fw(struct msm_ois_ctrl_t *ctrl, const char *file_path);

int rumba_update_parameter_for_updating_fw(struct msm_ois_ctrl_t *ctrl,uint32_t src_fw_rev, uint32_t target_fw_rev);
int rumba_init_params_for_updating_fw(struct msm_ois_ctrl_t *ctrl, uint32_t fw_revision);

#endif
