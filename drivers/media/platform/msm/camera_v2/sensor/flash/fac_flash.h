#ifndef FAC_FLASH_H
#define FAC_FLASH_H


#define	PROC_FILE_FLASH_BRIGHTNESS	"driver/asus_flash_brightness"
#define	PROC_FILE_FLASH_STATUS	"driver/flash_status"
#define PROC_FILE_FLASH_SELECT  "driver/Flash_Select"
#define PROC_FILE_ASUS_FLASH_1 "driver/asus_flash"
#define PROC_FILE_ASUS_FLASH_2 "driver/asus_flash2"
#define PROC_FILE_ASUS_FLASH_3 "driver/asus_flash3"
#define PROC_FILE_ASUS_FLASH_4 "driver/asus_flash4"
///////////////////////////////////////////////////
#define PROC_FILE_ASUS_FLASH_TIMEOUT_1 "driver/asus_flash_timeout"
#define PROC_FILE_ASUS_FLASH_TIMEOUT_2 "driver/asus_flash_timeout2"
#define PROC_FILE_ASUS_FLASH_TIMEOUT_3 "driver/asus_flash_timeout3"
#define PROC_FILE_ASUS_FLASH_TIMEOUT_4 "driver/asus_flash_timeout4"
////////////////////////////
#define SKY81296_HW_SPEC_DATA
#ifdef SKY81296_HW_SPEC_DATA
#define MAX_FLASH_CURRENT_SKY81296 1500
#define MAX_TORCH_CURRENT_SKY81296 250
#define FLASH_MODE_STEP_SKY81296 50
#define TORCH_MODE_STEP_SKY81296 25
#define MAX_FLASH_TIMEOUT_SKY81296 1425
//registers
#define SKY81296_FLASH1_CURRENT		0x00
#define SKY81296_FLASH2_CURRENT		0x01
#define SKY81296_FLASH_TIMER		0x02
#define SKY81296_MOVIE_MODE_CURRENT	0x03
#define SKY81296_CONTROL1		0x04
#define SKY81296_CONTROL2		0x05
#define SKY81296_CONTROL3		0x06

enum sky81296_flash_timeout{
	SKY81296_FLASHTIMEOUT_OFF, //0
	SKY81296_FLASHTIMEOUT_95MS,
	SKY81296_FLASHTIMEOUT_190MS,
	SKY81296_FLASHTIMEOUT_285MS,
	SKY81296_FLASHTIMEOUT_380MS,
	SKY81296_FLASHTIMEOUT_475MS,
	SKY81296_FLASHTIMEOUT_570MS,
	SKY81296_FLASHTIMEOUT_665MS,
	SKY81296_FLASHTIMEOUT_760MS,
	SKY81296_FLASHTIMEOUT_855MS,
	SKY81296_FLASHTIMEOUT_950MS,
	SKY81296_FLASHTIMEOUT_1045MS,
	SKY81296_FLASHTIMEOUT_1140MS,
	SKY81296_FLASHTIMEOUT_1235MS,
	SKY81296_FLASHTIMEOUT_1330MS,
	SKY81296_FLASHTIMEOUT_1425MS, //15
	SKY81296_FLASHTIMEOUT_NUM,
};

enum sky81296_flash_current{
	SKY81296_FLASH_CURRENT_250MA, //0
	SKY81296_FLASH_CURRENT_300MA,
	SKY81296_FLASH_CURRENT_350MA,
	SKY81296_FLASH_CURRENT_400MA,
	SKY81296_FLASH_CURRENT_450MA,
	SKY81296_FLASH_CURRENT_500MA,
	SKY81296_FLASH_CURRENT_550MA,
	SKY81296_FLASH_CURRENT_600MA,
	SKY81296_FLASH_CURRENT_650MA,
	SKY81296_FLASH_CURRENT_700MA,
	SKY81296_FLASH_CURRENT_750MA,
	SKY81296_FLASH_CURRENT_800MA,
	SKY81296_FLASH_CURRENT_850MA,
	SKY81296_FLASH_CURRENT_900MA,
	SKY81296_FLASH_CURRENT_950MA,
	SKY81296_FLASH_CURRENT_1000MA,
	SKY81296_FLASH_CURRENT_1100MA,
	SKY81296_FLASH_CURRENT_1200MA,
	SKY81296_FLASH_CURRENT_1300MA,
	SKY81296_FLASH_CURRENT_1400MA,
	SKY81296_FLASH_CURRENT_1500MA, //20
};

enum sky81296_torch_current{
	SKY81296_TORCH_CURRENT_25MA, //0
	SKY81296_TORCH_CURRENT_50MA,
	SKY81296_TORCH_CURRENT_75MA,
	SKY81296_TORCH_CURRENT_100MA,
	SKY81296_TORCH_CURRENT_125MA,
	SKY81296_TORCH_CURRENT_150MA,
	SKY81296_TORCH_CURRENT_175MA,
	SKY81296_TORCH_CURRENT_200MA,
	SKY81296_TORCH_CURRENT_225MA,
	SKY81296_TORCH_CURRENT_250MA, // 9
	SKY81296_TORCH_CURRENT_NUM,
};
#endif

#define PMI8952_HW_SPEC_DATA
#ifdef PMI8952_HW_SPEC_DATA
//pmi8952
#define MAX_FLASH_CURRENT_PMI8952  1000
#define MAX_TORCH_CURRENT_PMI8952  200
#define MAX_FLASH_TIMEOUT_PMI8952  1000
#endif

#define MAX_FLASH_CURRENT_SPEC_PISCES
#ifdef MAX_FLASH_CURRENT_SPEC_PISCES
//otg mode on pisces
#define MAX_FLASH_CURRENT_PISCES_REAR_OTG_1 900 
#define MAX_FLASH_CURRENT_PISCES_FRONT_OTG_1 100
#define MAX_TORCH_CURRENT_PISCES_REAR_OTG_1 200
#define MAX_TORCH_CURRENT_PISCES_FRONT_OTG_1 100
//otg mode off pisces
#define MAX_FLASH_CURRENT_PISCES_REAR_OTG_0 900 
#define MAX_FLASH_CURRENT_PISCES_FRONT_OTG_0 100
#define MAX_TORCH_CURRENT_PISCES_REAR_OTG_0 200
#define MAX_TORCH_CURRENT_PISCES_FRONT_OTG_0 100
//otg mode on hades
#define MAX_FLASH_CURRENT_HADES_REAR_OTG_1 900
#define MAX_TORCH_CURRENT_HADES_REAR_OTG_1 50
//otg mode off hades
#define MAX_FLASH_CURRENT_HADES_REAR_OTG_0 900
#define MAX_TORCH_CURRENT_HADES_REAR_OTG_0 200

//otg mode on phoenix
#define MAX_FLASH_CURRENT_PHOENIX_REAR_OTG_1 1000
#define MAX_FLASH_CURRENT_PHOENIX_FRONT_OTG_1 200
#define MAX_TORCH_CURRENT_PHOENIX_REAR_OTG_1 100
#define MAX_TORCH_CURRENT_PHOENIX_FRONT_OTG_1 200
//otg mode off phoenix
#define MAX_FLASH_CURRENT_PHOENIX_REAR_OTG_0 1000
#define MAX_FLASH_CURRENT_PHOENIX_FRONT_OTG_0 200
#define MAX_TORCH_CURRENT_PHOENIX_REAR_OTG_0 200
#define MAX_TORCH_CURRENT_PHOENIX_FRONT_OTG_0 200

//timeout
#define MAX_FLASH_MAX_TIMEOUT_REAR ( SKY81296_FLASHTIMEOUT_1425MS*95 )
#endif

#define MAX_FLASH_CURRENT_SPEC_AQUARIUS
#ifdef MAX_FLASH_CURRENT_SPEC_AQUARIUS
//otg mode on
#define MAX_FLASH_CURRENT_AQUARIUS_REAR_OTG_1 900 
//#define MAX_FLASH_CURRENT_AQUARIUS_FRONT_OTG_1 200
#define MAX_TORCH_CURRENT_AQUARIUS_REAR_OTG_1 100
//#define MAX_TORCH_CURRENT_AQUARIUS_FRONT_OTG_1 100
//otg mode off
#define MAX_FLASH_CURRENT_AQUARIUS_REAR_OTG_0 900 
//#define MAX_FLASH_CURRENT_AQUARIUS_FRONT_OTG_0 200   //no front flash
#define MAX_TORCH_CURRENT_AQUARIUS_REAR_OTG_0 200
//#define MAX_TORCH_CURRENT_AQUARIUS_FRONT_OTG_0 100   //no front flash
#endif
////////////////////////////
#define MAX_LED_FLASH 2
//#define ENABLE_FLASH_SELECT_PROC 1
#define FAC_FLASH_MODE_TORCH 0
#define FAC_FLASH_MODE_FLASH 1
#define FAC_FLASH_MODE_RELEASE 5


enum led_direction_t {
	REAR_LED = 0,
	FRONT_LED = 1,
};
void create_flash_proc_file(void * ctrl);

#endif



