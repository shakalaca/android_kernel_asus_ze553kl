#include "utils.h"
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

#undef  pr_fmt
#define pr_fmt(fmt) " [sz_cam_ois] UTILS %s() " fmt, __func__

void swap_word_data(uint16_t* register_data){
	*register_data = ((*register_data >> 8) | ((*register_data & 0xff) << 8)) ;
}

int format_hex_string(char *output_buf,int len, uint8_t* data,int count)
{
	int i;
	int offset;

	if(len<count*3+1+1)
		return -1;

	offset=0;

	for(i=0;i<count;i++)
	{
		offset+=sprintf(output_buf+offset,"0x%02x ",data[i]);
	}
	offset+=sprintf(output_buf+offset,"\n");

	return offset;
}

void delay_ms(uint32_t time)
{
	usleep_range(time*1000,time*1000+time*10);
}

void delay_us(uint32_t time)
{
	usleep_range(time,time+time/100);
}

int64_t diff_time_us(struct timeval *t1, struct timeval *t2 )
{
	return (((t1->tv_sec*1000000)+t1->tv_usec)-((t2->tv_sec*1000000)+t2->tv_usec));
}

int32_t get_file_size(const char *filename, uint64_t* size)
{
    struct kstat stat;
    mm_segment_t fs;
    int rc = 0;

	stat.size = 0;

    fs = get_fs();
    set_fs(KERNEL_DS);

	rc = vfs_stat(filename,&stat);
	if(rc < 0)
	{
		pr_err("vfs_stat(%s) failed, rc = %d\n",filename,rc);
		rc = -1;
		goto END;
	}

    *size = stat.size;
END:
	set_fs(fs);
    return rc;
}

int read_file_into_buffer(const char *filename, uint8_t* data, uint32_t size)
{
    struct file *fp;
    mm_segment_t fs;
    loff_t pos;
	int rc;

    fp = filp_open(filename,O_RDONLY,S_IRWXU | S_IRWXG | S_IRWXO);
    if (IS_ERR(fp)){
		pr_err("open(%s) failed, rc = %d\n",filename,rc);
        return -1;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);

    pos = 0;
	rc = vfs_read(fp,data, size, &pos);

    set_fs(fs);
    filp_close(fp,NULL);

    return rc;
}

int sysfs_write_byte_seq(char *filename, uint8_t *value, uint32_t size)
{
	struct file *fp = NULL;
	int i = 0;
	mm_segment_t old_fs;
	loff_t pos_lsts = 0;
	char buf[size][3];

	for(i=0; i<size; i++){
		sprintf(buf[i], "%02x", value[i]);
		buf[i][2] = '\n';
	}

	/* Open file */
	fp = filp_open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR_OR_NULL(fp)) {
		pr_err("%s: open %s fail, line = %d\n", __func__, filename, __LINE__);
		return -1;
	}

	/*For purpose that can use read/write system call*/

	/* Save addr_limit of the current process */
	old_fs = get_fs();
	/* Set addr_limit of the current process to that of kernel */
	set_fs(KERNEL_DS);

	if (fp->f_op != NULL && fp->f_op->write != NULL) {
		pos_lsts = 0;
		for(i = 0; i < size; i++){
			fp->f_op->write(fp, buf[i], 3, &fp->f_pos);
		}
	} else {
		/* Set addr_limit of the current process back to its own */
		set_fs(old_fs);

		/* Close file */
		filp_close(fp, NULL);
		pr_err("%s: f_op = null or write = null, fail line = %d\n", __func__, __LINE__);

		return -1;
	}
	/* Set addr_limit of the current process back to its own */
	set_fs(old_fs);

	/* Close file */
	filp_close(fp, NULL);
	return 0;
}

/** @brief read many word(two bytes) from file
*
*	@param filename the file to write
*	@param value the word which will store the calibration data from read file
*	@param size the size of write data
*
*/
int sysfs_read_word_seq(char *filename, int *value, uint32_t size)
{
	int i = 0;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos_lsts = 0;
	char buf[size][5];
	ssize_t buf_size = 0;
	/* open file */
	fp = filp_open(filename, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR_OR_NULL(fp)) {
		pr_err("%s: open %s fail, line = %d\n", __func__, filename, __LINE__);
		return -ENOENT;	/*No such file or directory*/
	}

	/*For purpose that can use read/write system call*/

	/* Save addr_limit of the current process */
	old_fs = get_fs();
	/* Set addr_limit of the current process to that of kernel */
	set_fs(KERNEL_DS);

	if (fp->f_op != NULL && fp->f_op->read != NULL) {
		pos_lsts = 0;
		for(i = 0; i < size; i++){
			buf_size = fp->f_op->read(fp, buf[i], 5, &pos_lsts);
			if(buf_size < 5) {
				/* Set addr_limit of the current process back to its own */
				set_fs(old_fs);
				/* close file */
				filp_close(fp, NULL);
				return -1;
			}
			buf[i][4]='\0';
			sscanf(buf[i], "%x", &value[i]);
		}
	} else {
		/* Set addr_limit of the current process back to its own */
		set_fs(old_fs);

		/* close file */
		filp_close(fp, NULL);
		pr_err("%s: f_op = null or write = null, fail line = %d\n", __func__, __LINE__);

		return -ENXIO;	/*No such device or address*/
	}
	/* Set addr_limit of the current process back to its own */
	set_fs(old_fs);

	/* close file */
	filp_close(fp, NULL);

	return 0;
}
/** @brief read many dword(four bytes) from file
*
*	@param filename the file to write
*	@param value the word which will store the calibration data from read file
*	@param size the size of write data
*
*/
int sysfs_read_dword_seq(char *filename, int *value, uint32_t size)
{
	int i = 0;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos_lsts = 0;
	char buf[size][9];
	ssize_t buf_size = 0;
	/* open file */
	fp = filp_open(filename, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR_OR_NULL(fp)) {
		pr_err("%s: open %s fail, line = %d\n", __func__, filename, __LINE__);
		return -ENOENT;	/*No such file or directory*/
	}

	/*For purpose that can use read/write system call*/

	/* Save addr_limit of the current process */
	old_fs = get_fs();
	/* Set addr_limit of the current process to that of kernel */
	set_fs(KERNEL_DS);

	if (fp->f_op != NULL && fp->f_op->read != NULL) {
		pos_lsts = 0;
		for(i = 0; i < size; i++){
			buf_size = fp->f_op->read(fp, buf[i], 9, &pos_lsts);
			if(buf_size < 9) {
				/* Set addr_limit of the current process back to its own */
				set_fs(old_fs);
				/* close file */
				filp_close(fp, NULL);
				return -1;
			}
			buf[i][8]='\0';
			sscanf(buf[i], "%x", &value[i]);
		}
	} else {
		/* Set addr_limit of the current process back to its own */
		set_fs(old_fs);

		/* close file */
		filp_close(fp, NULL);
		pr_err("%s: f_op = null or write = null, fail line = %d\n", __func__, __LINE__);

		return -ENXIO;	/*No such device or address*/
	}
	/* Set addr_limit of the current process back to its own */
	set_fs(old_fs);

	/* close file */
	filp_close(fp, NULL);

	return 0;
}
/** @brief read many byte from file
*
*	@param filename the file to write
*	@param value the byte which will store the calibration data from read file
*	@param size the size of write data
*
*/
int sysfs_read_byte_seq(char *filename, uint8_t *value, uint32_t size)
{
	int i = 0;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos_lsts = 0;
	char buf[3];
	ssize_t read_size = 0;

	/* open file */
	fp = filp_open(filename, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR_OR_NULL(fp)) {
		pr_err("%s: open %s fail, line = %d\n", __func__, filename, __LINE__);
		return -ENOENT;	/*No such file or directory*/
	}

	/*For purpose that can use read/write system call*/

	/* Save addr_limit of the current process */
	old_fs = get_fs();
	/* Set addr_limit of the current process to that of kernel */
	set_fs(KERNEL_DS);

	if (fp->f_op != NULL && fp->f_op->read != NULL) {
		pos_lsts = 0;
		for(i = 0; i < size; i++){
			read_size = fp->f_op->read(fp, buf, 3, &pos_lsts);
			buf[2]='\0';
			if(read_size == 0) {
				break;
			}
			sscanf(buf, "%02hhx", &value[i]);
		}
	} else {
		/* Set addr_limit of the current process back to its own */
		set_fs(old_fs);

		/* close file */
		filp_close(fp, NULL);
		pr_err("%s: f_op = null or write = null, fail line = %d\n", __func__, __LINE__);

		return -ENXIO;	/*No such device or address*/
	}
	/* Set addr_limit of the current process back to its own */
	set_fs(old_fs);

	/* close file */
	filp_close(fp, NULL);

	return 0;
}


/** @brief write many words(two bytes)  to file
*
*	@param filename the file to write
*	@param value the word which will be written to file
*	@param size the size of write data
*
*/
int sysfs_write_word_seq(char *filename, uint16_t *value, uint32_t size)
{
	struct file *fp = NULL;
	int i = 0;
	mm_segment_t old_fs;
	loff_t pos_lsts = 0;
	char buf[size][5];

	for(i=0; i<size; i++){
		sprintf(buf[i], "%04x", value[i]);
		buf[i][4] = ' ';
	}

	/* Open file */
	fp = filp_open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR_OR_NULL(fp)) {
		pr_err("%s: open %s fail, line = %d\n", __func__, filename, __LINE__);
		return -1;
	}

	/*For purpose that can use read/write system call*/

	/* Save addr_limit of the current process */
	old_fs = get_fs();
	/* Set addr_limit of the current process to that of kernel */
	set_fs(KERNEL_DS);

	if (fp->f_op != NULL && fp->f_op->write != NULL) {
		pos_lsts = 0;
		for(i = 0; i < size; i++){
			fp->f_op->write(fp, buf[i], 5, &fp->f_pos);
		}
	} else {
		/* Set addr_limit of the current process back to its own */
		set_fs(old_fs);

		/* Close file */
		filp_close(fp, NULL);
		pr_err("%s: f_op = null or write = null, fail line = %d\n", __func__, __LINE__);

		return -1;
	}
	/* Set addr_limit of the current process back to its own */
	set_fs(old_fs);

	/* Close file */
	filp_close(fp, NULL);
	return 0;
}

/** @brief write many dwords(four bytes)  to file
*
*	@param filename the file to write
*	@param value the word which will be written to file
*	@param size the size of write data
*
*/
int sysfs_write_dword_seq(char *filename, uint32_t *value, uint32_t size)
{
	struct file *fp = NULL;
	int i = 0;
	mm_segment_t old_fs;
	loff_t pos_lsts = 0;
	char buf[size][9];

	for(i=0; i<size; i++){
		sprintf(buf[i], "%08x", value[i]);
		buf[i][8] = ' ';
	}

	/* Open file */
	fp = filp_open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR_OR_NULL(fp)) {
		pr_err("%s: open %s fail, line = %d\n", __func__, filename, __LINE__);
		return -1;
	}

	/*For purpose that can use read/write system call*/

	/* Save addr_limit of the current process */
	old_fs = get_fs();
	/* Set addr_limit of the current process to that of kernel */
	set_fs(KERNEL_DS);

	if (fp->f_op != NULL && fp->f_op->write != NULL) {
		pos_lsts = 0;
		for(i = 0; i < size; i++){
			fp->f_op->write(fp, buf[i], 9, &fp->f_pos);
		}
	} else {
		/* Set addr_limit of the current process back to its own */
		set_fs(old_fs);

		/* Close file */
		filp_close(fp, NULL);
		pr_err("%s: f_op = null or write = null, fail line = %d\n", __func__, __LINE__);

		return -1;
	}
	/* Set addr_limit of the current process back to its own */
	set_fs(old_fs);

	/* Close file */
	filp_close(fp, NULL);
	return 0;
}

int sysfs_read_char_seq(char *filename, int *value, uint32_t size)
{
	int i = 0;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos_lsts = 0;
	char buf[2];
	ssize_t read_size = 0;

	/* open file */
	fp = filp_open(filename, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR_OR_NULL(fp)) {
		pr_err("%s: open %s fail, line = %d\n", __func__, filename, __LINE__);
		return -ENOENT;	/*No such file or directory*/
	}

	/*For purpose that can use read/write system call*/

	/* Save addr_limit of the current process */
	old_fs = get_fs();
	/* Set addr_limit of the current process to that of kernel */
	set_fs(KERNEL_DS);

	if (fp->f_op != NULL && fp->f_op->read != NULL) {
		pos_lsts = 0;
		for(i = 0; i < size; i++){
			read_size = fp->f_op->read(fp, buf, 1, &pos_lsts);

			buf[1]='\0';
			if(read_size == 0) {
				break;
			}
			sscanf(buf, "%d", &value[i]);
		}
	} else {
		/* Set addr_limit of the current process back to its own */
		set_fs(old_fs);

		/* Close file */
		filp_close(fp, NULL);
		pr_err("%s: f_op = null or write = null, fail line = %d\n", __func__, __LINE__);

		return -ENXIO;	/*No such device or address*/
	}
	/* Set addr_limit of the current process back to its own */
	set_fs(old_fs);

	/* close file */
	filp_close(fp, NULL);

	return 0;
}

int sysfs_write_word_seq_change_line(char *filename, uint16_t *value, uint32_t size,uint32_t number)
{
	struct file *fp = NULL;
	int i = 0;
	mm_segment_t old_fs;
	loff_t pos_lsts = 0;
	char buf[5];

	/* Open file */
	fp = filp_open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR_OR_NULL(fp)) {
		pr_err("%s: open %s fail, line = %d\n", __func__, filename, __LINE__);
		return -1;
	}

	/*For purpose that can use read/write system call*/

	/* Save addr_limit of the current process */
	old_fs = get_fs();
	/* Set addr_limit of the current process to that of kernel */
	set_fs(KERNEL_DS);

	if (fp->f_op != NULL && fp->f_op->write != NULL) {
		pos_lsts = 0;
		for(i = 0; i < size; i++){
			sprintf(buf, "%04x", value[i]);
			if((i + 1) % number == 0)
				buf[4] = '\n';
			else
				buf[4] = ',';
			fp->f_op->write(fp, buf, 5, &fp->f_pos);
		}
	} else {
		/* Set addr_limit of the current process back to its own */
		set_fs(old_fs);

		/* Close file */
		filp_close(fp, NULL);
		pr_err("%s: f_op = null or write = null, fail line = %d\n", __func__, __LINE__);

		return -1;
	}
	/* Set addr_limit of the current process back to its own */
	set_fs(old_fs);

	/* Close file */
	filp_close(fp, NULL);
	return 0;
}

/*ASUS_BSP --- bill_chen "Implement ois"*/

void fix_gpio_value_of_power_down_setting(struct msm_camera_power_ctrl_t *power_info)
{
	int i;
	int size;

	struct msm_sensor_power_setting *ps;
	size = power_info->power_down_setting_size;

	ps = power_info->power_down_setting;
	for(i=0;i<size;i++)
	{
		if(ps[i].seq_type == SENSOR_GPIO)
		{
			//GPIO Reset and Standy can not be reverted automatically
			//also revert Regulator related GPIO, not use regulator in power sequence
			if(ps[i].seq_val == SENSOR_GPIO_RESET ||
			   ps[i].seq_val == SENSOR_GPIO_STANDBY ||
			   ps[i].seq_val == SENSOR_GPIO_VDIG ||
			   ps[i].seq_val == SENSOR_GPIO_VANA ||
			   ps[i].seq_val == SENSOR_GPIO_VIO ||
			   ps[i].seq_val == SENSOR_GPIO_VAF)
			{
				if(ps[i].config_val == GPIO_OUT_HIGH)
				{
					ps[i].config_val = GPIO_OUT_LOW;
					pr_info("power down setting[%d] GPIO %d value reverted, %d -> %d\n",
							i,
							ps[i].seq_val,
							GPIO_OUT_HIGH,
							GPIO_OUT_LOW
					);
				}
				/*
				else if(ps[i].config_val == GPIO_OUT_LOW)
				{
					ps[i].config_val = GPIO_OUT_HIGH;
					pr_info("power down setting[%d] GPIO %d value reverted, %d -> %d\n",
							i,
							ps[i].seq_val,
							GPIO_OUT_LOW,
							GPIO_OUT_HIGH
					);
				}
				*/
			}
		}
	}
}

bool is_i2c_seq_setting_address_valid(struct msm_camera_i2c_seq_reg_setting* i2c_seq_setting, uint16_t max_reg_addr)
{
	int i;
	uint16_t start_addr;
	uint16_t data_size;
	bool rc = true;
	//pr_info("address type %d\n",i2c_seq_setting->addr_type);

	for(i=0;i<i2c_seq_setting->size;i++)
	{
		start_addr = i2c_seq_setting->reg_setting[i].reg_addr;
		data_size = i2c_seq_setting->reg_setting[i].reg_data_size;
		pr_debug("setting[%d], reg 0x%04x, data_size %d\n",
				i,
				start_addr,
				data_size);

		if(start_addr + data_size - 1 > max_reg_addr )
		{
			pr_err("Invalid address range detected! settings[%d], start addr 0x%04x, offset %d, max 0x%04x, while valid max is 0x%04x\n",
				i,
				start_addr,
				data_size,
				start_addr + data_size - 1,
				max_reg_addr
			);
			rc = false;
			break;
		}
	}
	return rc;
}
bool i2c_seq_setting_contain_address(struct msm_camera_i2c_seq_reg_setting* i2c_seq_setting, uint16_t reg_addr, uint8_t *reg_val)
{
	int i;
	uint16_t start_addr;
	uint16_t data_size;
	bool rc = false;

	for(i=0;i<i2c_seq_setting->size;i++)
	{
		start_addr = i2c_seq_setting->reg_setting[i].reg_addr;
		data_size = i2c_seq_setting->reg_setting[i].reg_data_size;

		if(reg_addr >= start_addr  && reg_addr <= start_addr + data_size - 1 )
		{
			*reg_val = i2c_seq_setting->reg_setting[i].reg_data[reg_addr-start_addr];
			pr_debug("settings[%d], start addr 0x%04x, offset %d, max 0x%04x, contains target reg 0x%04x, val 0x%02x\n",
				i,
				start_addr,
				data_size,
				start_addr + data_size - 1,
				reg_addr,
				*reg_val
			);
			rc = true;
			break;
		}
	}
	return rc;
}

void fix_i2c_seq_setting(struct msm_camera_i2c_seq_reg_setting* i2c_seq_setting, uint16_t reg_addr, uint8_t set_val)
{
	int i;
	uint16_t start_addr;
	uint16_t data_size;
	uint8_t  original_value;

	for(i=0;i<i2c_seq_setting->size;i++)
	{
		start_addr = i2c_seq_setting->reg_setting[i].reg_addr;
		data_size = i2c_seq_setting->reg_setting[i].reg_data_size;

		if(reg_addr >= start_addr  && reg_addr <= start_addr + data_size - 1 )
		{
			original_value = i2c_seq_setting->reg_setting[i].reg_data[reg_addr-start_addr];
			i2c_seq_setting->reg_setting[i].reg_data[reg_addr-start_addr] = set_val;
			pr_info("settings[%d], start addr 0x%04x, offset %d, max 0x%04x, contains target reg 0x%04x, val 0x%02x --> 0x%02x\n",
				i,
				start_addr,
				data_size,
				start_addr + data_size - 1,
				reg_addr,
				original_value,
				set_val
			);
			break;
		}
	}
}
