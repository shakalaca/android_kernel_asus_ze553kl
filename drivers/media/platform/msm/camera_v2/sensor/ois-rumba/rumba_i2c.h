#ifndef ASUS_OIS_RUMBA_I2C_H
#define ASUS_OIS_RUMBA_I2C_H
#include <linux/types.h>
#include "msm_ois.h"

int rumba_read_byte(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t* reg_data);
int rumba_read_word(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t* reg_data);
int rumba_read_dword(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint32_t* reg_data);
int rumba_read_seq_bytes(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint8_t* reg_data, uint32_t size);

int rumba_poll_byte(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t reg_data, uint32_t delay_ms);
int rumba_poll_word(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t reg_data, uint32_t delay_ms);

int rumba_write_byte(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t reg_data);
int rumba_write_word(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t reg_data);
int rumba_write_dword(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint32_t reg_data);
int rumba_write_seq_bytes(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint8_t* reg_data,uint32_t size);

#endif
