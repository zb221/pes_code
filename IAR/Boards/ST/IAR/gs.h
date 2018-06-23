#ifndef  GS_PRESENT
#define  GS_PRESENT

#include "time.h"

#define 	GPS_SEARCH_CYCLE		60*60*0		//UNIT:S
#define 	GPRS_COMMU_CYCLE		60*60*2		//UNIT:S


#define TENGXUN_SERVER 	1
#define XIAOKE_SERVER 	2
#define MY_SERVER 		3

#define SEND_TO	XIAOKE_SERVER

//#define 	USE_SYS_FLASH 

#ifdef USE_SYS_FLASH
	#define	STORAGE_SPACE_QUALITY  32u
#else
	#define	STORAGE_SPACE_QUALITY  16
#endif


//#define 	LED1  				GPIO_Pin_0
#define 	LED_PIN 				GPIO_Pin_1

extern int 	gps_enable,gprs_enable;
extern int 	system_time;

extern int 	bat_volts;
extern int 	gnd_fft;
extern int 	gnd_wtr;
extern int 	env_lux;
extern int 	env_tmp;
extern int 	env_hum;
extern int 	gps_csq;

extern char 	GPS_GPE[20];
extern char 	GPS_GPL[20];
extern char 	GPS_UTC[20];

void 		save_data(unsigned int address,char *dat,unsigned int dat_lenth);
void 		erase_data(unsigned int address);
void 		read_data(unsigned int address,char *dat,unsigned int dat_lenth);

void  		set_calendar(struct tm t);
struct tm 	get_calendar(void);

void 		check_bkp(void);

u16 	       	get_battery_volts(void);
u16   		get_fft_volts(void);
INT16U 		soil_volts_to_fertility(INT16U volts);
u16 	       	get_hum_volts(void);

INT8U 		uart_write(char* buf,int len);
INT8U 		uart_write_string(char* buf);
INT8U 		uart_read(char* buf,int* len,int n_100ms);

u32    		read_opt3001(void);
void 		read_hdc1000(void);

void 		check_sum_16(char sndDataTmp[],int  stringLenth,unsigned short * chkSumTmp);

u32 	       	analysis_gps_info(char *nema_buf);


void 		led_off(int led_id);
void 		led_on(int led_id);

char * base64_encode( unsigned char * bindata, char * base64, int binlength );
#endif
