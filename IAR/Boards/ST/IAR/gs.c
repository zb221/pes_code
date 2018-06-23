#include <bsp.h>
#include <string.h>
#include <time.h>
#define _FLASH_PROG
#include "stm32f10x_flash.h"
#include <gs.h>

int gps_enable;
int gprs_enable;
struct tm now;

int bat_volts;
int gnd_fft;
int gnd_wtr;
int env_lux;
int env_tmp;
int env_hum;

int gps_csq;
int gps_last_work_time,bkp_enable;

int system_time;
int gps_work_time;
int gprs_work_time;

char GPS_GPE[20];
char GPS_GPL[20];
char GPS_UTC[20];




u16 get_battery_volts(void)
{
  int adc=0;
  int count=10;
  
  while(count--){
	  ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5 );
	  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	  OSTimeDly(1);
	  while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC));
	  adc+=(u32)ADC_GetConversionValue(ADC1);
  	}
  return adc*330/4096;
}

 u16 get_fft_volts(void)
{
  int adc=0;
  int count=100;
  
  GPIO_SetBits(GPIOA,GPIO_Pin_5);
  OSTimeDly(100);
  while(count--){
	  ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_239Cycles5 );
	  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	  OSTimeDly(1);
   	  while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC));
	  adc+=(u32)ADC_GetConversionValue(ADC1);
  	}
  GPIO_ResetBits(GPIOA,GPIO_Pin_5);
  return adc*33/4096;
}

u16 get_hum_volts(void)
{
  int adc=0;
  int count=100;

  //TIM_Cmd(TIM3, ENABLE);
  GPIO_SetBits(GPIOA,GPIO_Pin_7);
  OSTimeDly(100);
  while(count--){
	  ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_239Cycles5 );
	  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	  OSTimeDly(1);
   	  while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC));
	  adc+=(u32)ADC_GetConversionValue(ADC1);
  }
  
  //TIM_Cmd(TIM3, DISABLE);
  GPIO_ResetBits(GPIOA,GPIO_Pin_7);
  return adc*33/4096;
}


struct tm get_calendar(void)
{
	time_t t_t;
	struct tm t_tm;

	t_t = (time_t)RTC_GetCounter();
	t_tm = *localtime(&t_t);
	return t_tm;
}


void set_calendar(struct tm t)
{
	RTC_WaitForLastTask();
	RTC_SetCounter((u32)mktime(&t));
	RTC_WaitForLastTask();
}

void check_bkp(void)
{
	gps_enable=0;
	gprs_enable=0;
   
	system_time=RTC_GetCounter(); 
	gps_work_time=((unsigned int)BKP_ReadBackupRegister(BKP_DR2)<<16)|(BKP_ReadBackupRegister(BKP_DR3));
	gprs_work_time=((unsigned int)BKP_ReadBackupRegister(BKP_DR4)<<16)|(BKP_ReadBackupRegister(BKP_DR5));

  
  
	if((system_time-gps_work_time)>GPS_SEARCH_CYCLE)
		gps_enable=1;
  
	if((system_time-gprs_work_time)>GPRS_COMMU_CYCLE)
		gprs_enable=1; 
  
  
	if(BKP_ReadBackupRegister(BKP_DR1)!=0xa5a5){
		gps_enable=1;
		gprs_enable=1;
    
		}

	now=get_calendar();
	if(now.tm_year<117){
		gps_enable=1;
		gprs_enable=1;
		}

}



//----------------------------------------------------------------------------------------


static void twi_nop(void)  
{  
	u32 i;  
	i = 25;  //min=6
	while(i--);  
}  

static u8 twi_start(void)  
{   
	GPIO_SetBits(GPIOB, GPIO_Pin_11);   
	twi_nop();  
	GPIO_SetBits(GPIOB, GPIO_Pin_10);   
	twi_nop();      
			  
	if(!GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11))  
		return 0; //bus busy

	GPIO_ResetBits(GPIOB, GPIO_Pin_11);  
	twi_nop();  
	GPIO_ResetBits(GPIOB, GPIO_Pin_10);    
	twi_nop();   
			  
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11))  
		return 0; //bus error

	return 1;  
}  



static void twi_stop(void)  
{  
	GPIO_ResetBits(GPIOB, GPIO_Pin_11);   
	twi_nop();  
     
	GPIO_SetBits(GPIOB, GPIO_Pin_10);   
	twi_nop();      
  
	GPIO_SetBits(GPIOB, GPIO_Pin_11);  
	twi_nop();  
}  

static void twi_send_ack(void)  
{  
	GPIO_ResetBits(GPIOB, GPIO_Pin_11);  
	twi_nop();  
	GPIO_SetBits(GPIOB, GPIO_Pin_10);  
	twi_nop();  
	GPIO_ResetBits(GPIOB, GPIO_Pin_10);   
	twi_nop();   
	GPIO_SetBits(GPIOB, GPIO_Pin_11);  
}  
  
static void twi_send_nack(void)  
{  
	GPIO_SetBits(GPIOB, GPIO_Pin_11);  
	twi_nop();  
	GPIO_SetBits(GPIOB, GPIO_Pin_10);  
	twi_nop();  
	GPIO_ResetBits(GPIOB, GPIO_Pin_10);   
	twi_nop();  
}  


static u8 twi_send_byte(u8 Data)  
{  
	u32 i;  
	  
	GPIO_ResetBits(GPIOB, GPIO_Pin_10);  
	for(i=0;i<8;i++){    
		if(Data&0x80)
			GPIO_SetBits(GPIOB, GPIO_Pin_11);  
		else  
			GPIO_ResetBits(GPIOB, GPIO_Pin_11);  
		Data<<=1;  
	  
		twi_nop();  
		GPIO_SetBits(GPIOB, GPIO_Pin_10);  
		twi_nop();  
		GPIO_ResetBits(GPIOB, GPIO_Pin_10);  
		twi_nop();
		}  
	GPIO_SetBits(GPIOB, GPIO_Pin_11);   
	twi_nop();  
	GPIO_SetBits(GPIOB, GPIO_Pin_10);  
	twi_nop();     
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11))  {  
		GPIO_ResetBits(GPIOB, GPIO_Pin_10);  
		GPIO_SetBits(GPIOB, GPIO_Pin_11);  
		return 0;  //NACK
		}
	      
	else  {  
		GPIO_ResetBits(GPIOB, GPIO_Pin_10);  
		GPIO_SetBits(GPIOB, GPIO_Pin_11);  
		return 1; //ACK  
		}
	   
}  
 
static u8 twi_receive_byte(void)  
{  
	u32 i;
	u8 Dat; 

	GPIO_SetBits(GPIOB, GPIO_Pin_11);  
	GPIO_ResetBits(GPIOB, GPIO_Pin_10);   
	Dat=0;  
	for(i=0;i<8;i++)  {  
		GPIO_SetBits(GPIOB, GPIO_Pin_10);
		twi_nop();   
		Dat<<=1;  
		if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11))
			Dat|=0x01;   
		GPIO_ResetBits(GPIOB, GPIO_Pin_10);
		twi_nop();
		}  
	return Dat;  
}  

void twi_write (unsigned char addr, unsigned char* buf, int len)  
{  
	u32 i;  
	u8 t;  
  
	twi_start();                       
	t = (addr << 1) | 0;                   
	twi_send_byte(t);  
	for (i=0; i<len; i++)  
		twi_send_byte(buf[i]);  
	twi_stop();                   
}


void twi_read(unsigned char addr, unsigned char* buf, int len)  
{  
	u32 i;  
	u8 t;  
  
	twi_start();                      
	t = (addr << 1) | 1;                   
	twi_send_byte(t);  
	for (i=0; i<len-1; i++){
		*buf++= twi_receive_byte();  
		twi_send_ack();
		}
	*buf= twi_receive_byte();  
	twi_send_nack();
	twi_stop();                   
}  


#define OPT3001_ADDRESS			0x44	//I2C address for OPT3001

#define OPT_RESULT_ADDR			0x00	//Result register
#define OPT_CONFIG_ADDR			0x01	//Configuration register
#define OPT_LOW_LIMIT_ADDR		0x02	//Low limit register
#define OPT_HIGH_LIMIT_ADDR		0x03	//High limit register
#define OPT_MAN_ID_ADDR			0x7E	//Manufacturer ID register
#define OPT_DEV_ID_ADDR			0x7F	//Device ID register
#define OPT_CONFIG_MSB 			0xC2	//MSB of configuration
#define OPT_CONFIG_LSB         		0x11	//LSB of configuration


u32 read_opt3001(void)
{
	u8 Buf[3];
	u32 lux_e,lux_r;

	Buf[0]=OPT_DEV_ID_ADDR;
	twi_write(OPT3001_ADDRESS,Buf,1);
	twi_read(OPT3001_ADDRESS,Buf,2);
	if((Buf[0]==0x30)&&(Buf[1]==0x01)){
		Buf[0]=OPT_CONFIG_ADDR;
		Buf[1]=OPT_CONFIG_MSB;
		Buf[2]=OPT_CONFIG_LSB;
		twi_write(OPT3001_ADDRESS,Buf,3);
		OSTimeDly(1000);
		Buf[0]=OPT_RESULT_ADDR;
		twi_write(OPT3001_ADDRESS,Buf,1);
		twi_read(OPT3001_ADDRESS,Buf,2);
		lux_e= 1<<((Buf[0]&0xf0)>>4);
		lux_r= ((Buf[0]&0x0f)<<8)|Buf[1];
		return lux_e*lux_r/100;
		}
        return -1;
}

  
#define HDC1000_ADDRESS			0x40	//I2C address for HDC1000
#define HDC1000_TEMP_ADDR		0x00	//Temperature measurement output register
#define	HDC1000_HUMIDITY_ADDR	        0x01	//Relative humidity output register
#define HDC1000_CONFIG_ADDR		0x02	//HDC1000 configuration and status
#define HDC1000_MAN_ID_ADDR		0xFE	//Manufacturer ID register (default: 'TI')
#define HDC1000_DEV_ID_ADDR		0xFF	//Device ID register
#define HDC1000_TEMP_RH_11BIT_MSB	0x15	//MSB of configuration
#define HDC1000_TEMP_RF_11BIT_LSB	0x00	//LSB of configuration

void read_hdc1000(void)
{
	u8 Buf[4];
	u32 temp;
  
	env_tmp=-1;
	env_hum=-1;
  
	Buf[0]=HDC1000_DEV_ID_ADDR;
	twi_write(HDC1000_ADDRESS,Buf,1);
	twi_read(HDC1000_ADDRESS,Buf,2);
	if((Buf[0]==0x10)&&(Buf[1]==0x50)){
		Buf[0]=HDC1000_CONFIG_ADDR;
		Buf[1]=HDC1000_TEMP_RH_11BIT_MSB;
		Buf[2]=HDC1000_TEMP_RF_11BIT_LSB;
		twi_write(HDC1000_ADDRESS,Buf,3);
		Buf[0]=HDC1000_TEMP_ADDR;
		twi_write(HDC1000_ADDRESS,Buf,1);
		OSTimeDly(100);
		twi_read(HDC1000_ADDRESS,Buf,4);
		temp=((Buf[0]<<8)|Buf[1]);
		env_tmp=(temp*1650/65535-400);
		temp=((Buf[2]<<8)|Buf[3]);
		env_hum=(temp*100/65535);  
		}
}



char *uart_tx_buf;
char uart_tx_len;
char uart_rx_len;
char *uart_rx_buf;


OS_EVENT *uart_tx_done;
OS_EVENT *uart_rx_done;

OS_TMR *uart_rx_tmr_ovf;
OS_TMR *uart_rx_tmr_process;

INT8U uart_write_string(char* buf)
{
	INT8U err;
	uart_tx_buf=buf;
	uart_tx_len=strlen(buf);
	uart_tx_done=OSSemCreate(0);
	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	OSSemPend(uart_tx_done,0,&err);
	OSSemDel(uart_tx_done,OS_DEL_NO_PEND, &err);
	return err;
}

INT8U uart_write(char* buf,int len)
{
	INT8U err;
	uart_tx_buf=buf;
	uart_tx_len=len;
	uart_tx_done=OSSemCreate(0);
	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	OSSemPend(uart_tx_done,0,&err);
	OSSemDel(uart_tx_done,OS_DEL_NO_PEND, &err);
	return err;
}


void uart_rx_continue(void *ptmr, void *parg)
{
	OSSemPost(uart_rx_done);
}


void uart_rx_ovf(void *ptmr, void *parg)
{
	OSSemPost(uart_rx_done);
}

INT8U uart_read(char* buf,int* len,int timeout)
{
	INT8U err;
	uart_rx_buf=buf;
	uart_rx_len=0;
	*uart_rx_buf=0;
	uart_rx_done=OSSemCreate(0);
	USART_ReceiveData(USART2);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	uart_rx_tmr_process= OSTmrCreate (0, 3,OS_TMR_OPT_PERIODIC,  uart_rx_continue, (void  *)NULL,"process",&err);
	uart_rx_tmr_ovf= OSTmrCreate (0, timeout/100,OS_TMR_OPT_PERIODIC,  uart_rx_ovf, (void  *)NULL,"ovf",&err);
	OSTmrStart(uart_rx_tmr_ovf, &err);
        
	OSSemPend(uart_rx_done,0,&err);

	OSTmrDel(uart_rx_tmr_process, &err);
	OSTmrDel(uart_rx_tmr_ovf, &err);
	OSSemDel(uart_rx_done,OS_DEL_NO_PEND, &err);
	
	*len=uart_rx_len;
	
	return err;
}

u8 uart_read_wait(char* buf,int* len,int timeout)
{
	INT8U err;
	uart_rx_buf=buf;
	uart_rx_len=0;
	*uart_rx_buf=0;
	uart_rx_done=OSSemCreate(0);
	USART_ReceiveData(USART2);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	uart_rx_tmr_process= OSTmrCreate (0, timeout/50,OS_TMR_OPT_PERIODIC,  uart_rx_continue, (void  *)NULL,"tmr_process",&err);
	uart_rx_tmr_ovf= OSTmrCreate (0, timeout/100,OS_TMR_OPT_PERIODIC,  uart_rx_ovf, (void  *)NULL,"tmr_ovf",&err);
	OSTmrStart(uart_rx_tmr_ovf, &err);
        
	OSSemPend(uart_rx_done,0,&err);

	OSTmrDel(uart_rx_tmr_process, &err);
	OSTmrDel(uart_rx_tmr_ovf, &err);
	OSSemDel(uart_rx_done,OS_DEL_NO_PEND, &err);
	
	*len=uart_rx_len;
	
	return err;
}

void USART2_IRQHandler(void)
{
	INT8U err;
	if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET){
		if(uart_tx_len>0){
			USART_SendData(USART2, *uart_tx_buf);
			uart_tx_buf++;
			uart_tx_len--;
			}
		else{
			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
			OSSemPost(uart_tx_done);
			}
		}
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET){
		OSTmrStart(uart_rx_tmr_process, &err);
                *uart_rx_buf=(unsigned char)USART_ReceiveData(USART2);
		if(uart_rx_len<199){
			uart_rx_buf++;
			uart_rx_len++;
			*uart_rx_buf=0;
		}

             
	}	
}










void check_sum_16(char sndDataTmp[],int  stringLenth,unsigned short * chkSumTmp)
{
	u16  i=0;
	*chkSumTmp=0;
	for (i=0;i<stringLenth;i++)
		*chkSumTmp+=sndDataTmp[i];
	*chkSumTmp=~(*chkSumTmp)+1;
}




u16 const soil_volts_to_fertility_table[][2]={
  {0,0},
  {320,100},
  {640,200},
  {940,300},
  {1160,400},
  {1320,500},
  {1820,1000},
  {2070,1500},
  {2270,2000},
  {2420,3000},
  {2544,4000},
  {3000,4838},
};


u16 soil_volts_to_fertility(u16 volts)
{
	int i;
	int input_min,input_max;
	int output_min,output_max;
	float val=0;
  
	volts=(u16)((float)volts/1.1);
	if(volts>3000)
		return 5000;
  
	for(i=0;i<11;i++){
		input_min=soil_volts_to_fertility_table[i][0];
		input_max=soil_volts_to_fertility_table[i+1][0];
		output_min=soil_volts_to_fertility_table[i][1];
		output_max=soil_volts_to_fertility_table[i+1][1];
    
		if((volts>=input_min)&&(volts<input_max)){
			val=volts-input_min;
			val=val/(input_max-input_min);
			val=val*(output_max-output_min);
			val=val+output_min;
			return (u16)val;
			}
		}
	return (-1);
}




#define MAX_STR_LENTH  20
u32 analysis_gps_info(char *nema_buf)
{
	int i=0;
	int count;
	char *p_source;
	char *p_dest;
  
	if(nema_buf[2]!='+')
		return 0;
      
	if(nema_buf[16]==',')
		return 0;
  
	i=0;
	for(count=0;count<200;count++){
	if(nema_buf[count]==',')
		i++;
	if(i==6){
		if(nema_buf[count+1]=='1')
			break;
		else
			return 0;
		}
	}
        
	if(strncmp(&nema_buf[10],"GPGGA",5)==0){
		p_source=nema_buf+16;
		p_dest=GPS_UTC;
                p_dest+=6;
     
		for(i=0;i<6;i++)
			*p_dest++=*p_source++;

		p_source+=5;
		p_dest=GPS_GPE;
		for(i=0;i<MAX_STR_LENTH;i++){
			if(((*p_source>='0')&&(*p_source<='9'))||(*p_source=='.'))
				*p_dest++=*p_source++;
			else{
				*p_dest=0;
				break;
				}
			}
    
		p_source+=3;
		p_dest=GPS_GPL;
		for(i=0;i<MAX_STR_LENTH;i++){
			if(((*p_source>='0')&&(*p_source<='9'))||(*p_source=='.'))
				*p_dest++=*p_source++;
			else{
				*p_dest=0;
				break;
				}
			} 
		
		p_source=strstr(p_source,"GPRMC");
                if(p_source==0)
                  return 0;
		p_source+=6;
		count=8;
		while(count--){
			p_source=strstr(p_source,",");
			p_source++;
                }
		p_dest=GPS_UTC;
		for(i=0;i<6;i++)			
			*p_dest++=*p_source++;
		}
	return 1;
}



void led_off(int led_id)
{
	GPIO_SetBits(GPIOB, led_id);
}

void led_on(int led_id)
{

	GPIO_ResetBits(GPIOB, led_id);
}

#define FLASH_PAGE_SIZE    0x400
#define USR_FLASH_PAGES    32

u8 erase_flash(unsigned char usr_flash_page)
{
	volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;
	int flash_addr;		

	if(usr_flash_page>=USR_FLASH_PAGES)
		return 0;
       
	flash_addr=0x08008000+(FLASH_PAGE_SIZE * usr_flash_page);
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	
	FLASHStatus = FLASH_ErasePage(flash_addr);
	FLASH_Lock();
	return 1;
}

 

u8 read_flash(unsigned char usr_flash_page,char *dat,unsigned int dat_lenth)
{
	u32 flash_addr;
	unsigned int xdat;
	unsigned char * pointer;

	if(usr_flash_page>=USR_FLASH_PAGES)
		return 0;

	if(dat_lenth>FLASH_PAGE_SIZE)
		return 0;

	if((dat_lenth%4)==0)
		dat_lenth=dat_lenth/4;
	else
		dat_lenth=dat_lenth/4+1;

	flash_addr=0x08008000+(FLASH_PAGE_SIZE * usr_flash_page);
	
	while(dat_lenth--){
		xdat=*(volatile unsigned int *)flash_addr;
		pointer=(unsigned char *)&xdat;
		dat[0]=pointer[0];
		dat[1]=pointer[1];
		dat[2]=pointer[2];
		dat[3]=pointer[3];
		dat+=4;
		flash_addr+=4;
		}
	return 1;
}


u8 write_flash(unsigned char usr_flash_page,char *dat,unsigned int dat_lenth)
{
       volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;
	u32 flash_addr;

	if(usr_flash_page>=USR_FLASH_PAGES)
		return 0;

	if(dat_lenth>FLASH_PAGE_SIZE)
		return 0;
	
	if((dat_lenth%4)==0)
		dat_lenth=dat_lenth/4;
	else
		dat_lenth=dat_lenth/4+1;
		
       
	flash_addr=0x08008000+(FLASH_PAGE_SIZE * usr_flash_page);
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	
        
	if(*(unsigned int *)flash_addr!=0xffffffff)
		FLASHStatus = FLASH_ErasePage(flash_addr);

	while((dat_lenth--)&&(FLASHStatus == FLASH_COMPLETE)){
		FLASHStatus = FLASH_ProgramWord(flash_addr, *(u32 *)dat);
		flash_addr+=4;
		dat+=4;
		}
 
	FLASH_Lock();
	
	return 1;

}


/*
#define EEPROM_BLOCK_SIZE        	256
#define EEPROM_BLOCKS        		256
u8 write_eeprom(unsigned int usr_eeprom_block,char *dat,unsigned int dat_lenth)
{
	int i;
	u32 eeprom_addr;

	if(usr_eeprom_block>=EEPROM_BLOCKS)
		return 0;

	if(dat_lenth>EEPROM_BLOCK_SIZE)
		return 0;

	eeprom_addr=usr_eeprom_block*EEPROM_BLOCK_SIZE;

	if(dat_lenth>128){
		twi_start();
		twi_send_byte(0xa0);
		twi_send_byte(eeprom_addr>>8);
		twi_send_byte(eeprom_addr&0xff);

		for(i=0;i<128;i++)
			twi_send_byte(*dat++);
		twi_stop();
		dat_lenth-=128;
		eeprom_addr+=128;
		}
	
	OSTimeDly(20);

	twi_start();
	twi_send_byte(0xa0);
	twi_send_byte(eeprom_addr>>8);
	twi_send_byte(eeprom_addr&0xff);
	while(dat_lenth--)
		twi_send_byte(*dat++);
	twi_stop();
	OSTimeDly(20);
        
	return 1;
}

u8 erase_eeprom(unsigned char usr_eeprom_block)
{
	char blank[8]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	write_eeprom(usr_eeprom_block,blank,8);
	return 1;
}

u8 read_eeprom(unsigned int usr_eeprom_block,char *dat,unsigned int dat_lenth)
{
	int i;
	u32 eeprom_addr;

	if(usr_eeprom_block>=EEPROM_BLOCKS)
		return 0;

	if(dat_lenth>EEPROM_BLOCK_SIZE)
		return 0;
	eeprom_addr=usr_eeprom_block*EEPROM_BLOCK_SIZE;
	
	twi_start();
	twi_send_byte(0xa0);
	twi_send_byte(eeprom_addr>>8);
	twi_send_byte(eeprom_addr&0xff);

	twi_start();
	twi_send_byte(0xa1);

	for(i=0;i<dat_lenth-1;i++){
		dat[i]=twi_receive_byte();
		twi_send_ack();
		}
	dat[i]=twi_receive_byte();
	twi_send_nack();
	twi_stop();
	OSTimeDly(2);
        
	return 1;
}

*/


///-------------------------------AT24C32E-------------------------------

#define EEPROM_BLOCK_SIZE        	32
#define EEPROM_BLOCKS        		16

 
  

u8 write_eeprom(unsigned int usr_eeprom_block,char *dat,unsigned int dat_lenth)
{
	int i;
	u32 eeprom_addr;
        int count;
        int j;
        
     
        count=dat_lenth/32;
        count++;
        if((count%32)==0)
          count--;
        

	eeprom_addr=usr_eeprom_block*256;

	if(count>1){
          
          for(j=0;j<(count-1);j++){
		twi_start();
		twi_send_byte(0xa0);
		twi_send_byte(eeprom_addr>>8);
		twi_send_byte(eeprom_addr&0xff);

		for(i=0;i<32;i++)
			twi_send_byte(*dat++);
		twi_stop();
		dat_lenth-=32;
		eeprom_addr+=32;
                  OSTimeDly(20);
		}
          
        }
	
	

	twi_start();
	twi_send_byte(0xa0);
	twi_send_byte(eeprom_addr>>8);
	twi_send_byte(eeprom_addr&0xff);
	while(dat_lenth--)
		twi_send_byte(*dat++);
	twi_stop();
	OSTimeDly(20);
        
	return 1;
}

u8 erase_eeprom(unsigned char usr_eeprom_block)
{
	char blank[8]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	write_eeprom(usr_eeprom_block,blank,8);
	return 1;
}

u8 read_eeprom(unsigned int usr_eeprom_block,char *dat,unsigned int dat_lenth)
{
	int i;
	u32 eeprom_addr;


	eeprom_addr=usr_eeprom_block*256;
	
	twi_start();
	twi_send_byte(0xa0);
	twi_send_byte(eeprom_addr>>8);
	twi_send_byte(eeprom_addr&0xff);

	twi_start();
	twi_send_byte(0xa1);

	for(i=0;i<dat_lenth-1;i++){
		dat[i]=twi_receive_byte();
		twi_send_ack();
		}
	dat[i]=twi_receive_byte();
	twi_send_nack();
	twi_stop();
	OSTimeDly(2);
        
	return 1;
}


///-------------------------------AT24C32E-------------------------------


















void save_data(unsigned int address,char *dat,unsigned int dat_lenth)
{
#ifdef USE_SYS_FLASH
	write_flash(address,dat,dat_lenth);
#else
	write_eeprom(address,dat,dat_lenth);
#endif
}
void erase_data(unsigned int address)
{
#ifdef USE_SYS_FLASH
	erase_flash(address);
#else
	erase_eeprom(address);
#endif
}
void read_data(unsigned int address,char *dat,unsigned int dat_lenth)
{
#ifdef USE_SYS_FLASH
	read_flash(address,dat,dat_lenth);
#else
	read_eeprom(address,dat,dat_lenth);
#endif
}

const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char * base64_encode( unsigned char * bindata, char * base64, int binlength )
{
    int i, j;
    unsigned char current;

    for ( i = 0, j = 0 ; i < binlength ; i += 3 )
    {
        current = (bindata[i] >> 2) ;
        current &= (unsigned char)0x3F;
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)(bindata[i] << 4 ) ) & ( (unsigned char)0x30 ) ;
        if ( i + 1 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+1] >> 4) ) & ( (unsigned char) 0x0F );
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)(bindata[i+1] << 2) ) & ( (unsigned char)0x3C ) ;
        if ( i + 2 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+2] >> 6) ) & ( (unsigned char) 0x03 );
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)bindata[i+2] ) & ( (unsigned char)0x3F ) ;
        base64[j++] = base64char[(int)current];
    }
    base64[j] = '\0';
    return base64;
}








