/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#include <includes.h>
#include <time.h>

/*
*********************************************************************************************************
*                                       DEFINES
*********************************************************************************************************
*/

 
/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/
static  OS_STK         App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static  OS_STK         App_TaskIOStk[APP_CFG_TASK_IO_STK_SIZE];

char gps_buf[200];

char gps_buf2[300];
int gps_buf_count;
int retry;
int cpuid[3];
char tmp_str[20];
unsigned short chksum;
unsigned int i,temp;
int count;
int gsm_err;
extern  struct tm now;
int max_value,max_index;

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/
static  void            App_TaskStart              (void);
static  void            App_TaskIO                 (void       *p_arg);

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This the main standard entry point.
*
* Note(s)     : none.
*********************************************************************************************************
*/
int  main (void)
{
#if (OS_TASK_NAME_SIZE > 5)    
	CPU_INT08U  err;
#endif

	BSP_IntDisAll();									/* Disable all interrupts until we are ready to accept them */

	OSInit();											/* Initialize "uC/OS-II, The Real-Time Kernel"              */

	OSTaskCreateExt((void (*)(void *)) App_TaskStart,	/* Create the start task                                    */
                    (void           *) 0,
                    (OS_STK         *)&App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE - 1],
                    (INT8U           ) APP_CFG_TASK_START_PRIO,
                    (INT16U          ) APP_CFG_TASK_START_PRIO,
                    (OS_STK         *)&App_TaskStartStk[0],
                    (INT32U          ) APP_CFG_TASK_START_STK_SIZE,
                    (void           *) 0,
                    (INT16U          )(OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));

#if (OS_TASK_NAME_SIZE >5 )
	OSTaskNameSet(APP_CFG_TASK_START_PRIO, "Start", &err);
#endif

	OSStart();											/* Start multitasking (i.e. give control to uC/OS-II)       */
}

/*
*********************************************************************************************************
*                                          App_TaskStart()
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
*
* Return(s)   : none.
*
* Caller(s)   : This is a task.
*
* Notes       : (1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                   used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/
static  void  App_TaskStart (void)
{
	BSP_Init();											/* Initialize BSP functions                          */
	OS_CPU_SysTickInit();								/*Initialize the SysTick.                            */

#if (OS_TASK_STAT_EN > 0)
	OSStatInit();										/* Determine CPU capacity                            */
#endif

#if (OS_TASK_NAME_SIZE > 2)        
	CPU_INT08U      err;
#endif
    

//task1
	OSTaskCreateExt((void (*)(void *)) App_TaskIO,
		(void           *) 0,
		(OS_STK         *)&App_TaskIOStk[APP_CFG_TASK_IO_STK_SIZE - 1],
		(INT8U           ) APP_CFG_TASK_IO_PRIO,
		(INT16U          ) APP_CFG_TASK_IO_PRIO,
		(OS_STK         *)&App_TaskIOStk[0],
		(INT32U          ) APP_CFG_TASK_IO_STK_SIZE,
		(void           *) 0,
		(INT16U          )(OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));

#if (OS_TASK_NAME_SIZE > 2)
	OSTaskNameSet(APP_CFG_TASK_IO_PRIO, "IO", &err);
#endif
        

//--------------------------READ DEVICE ID-----------------------------        
        cpuid[0]=*((volatile unsigned int *)(0x1FFFF7E8));
	cpuid[1]=*((volatile unsigned int *)(0x1ffff7EC));
	cpuid[2]=*((volatile unsigned int *)(0x1FFFF7F0));
	cpuid[0]&=0xFFFF;
        
//-------------------------SENSOR START--------------------------------
	if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET){
		RCC_ClearFlag();
		uart_write_string("AT\r\n");
		uart_read(gps_buf,&gps_buf_count,300);
		gps_buf[0]=0x1A;
		uart_write(gps_buf,1);
		uart_read(gps_buf,&gps_buf_count,300);
		uart_write_string("AT+CPOF\r\n");
		uart_read(gps_buf,&gps_buf_count,1000);
		}
	
	bat_volts=get_battery_volts();
	bat_volts=31*bat_volts/18;
	if(bat_volts<3700){
		gps_enable=0;
		gprs_enable=0;
		}
        
	if(bat_volts<3600)
		goto system_off_section;
        
	check_bkp();
	
	env_lux=read_opt3001();
	read_hdc1000();
        
	for(i=0;i<STORAGE_SPACE_QUALITY;i++){
		read_data(i,gps_buf,4);
		if(gps_buf[0]==0xff)
			break;
		}
	if(i==STORAGE_SPACE_QUALITY)
		gprs_enable=1;

	i=get_fft_volts();
	gnd_fft=soil_volts_to_fertility(i);
	i=get_hum_volts(); 
	if(i<220)
		gnd_wtr=0;
	else
		gnd_wtr=(i-220)/4;
	
	gps_enable=1;//for debug        
        
//------------------------------PWR ON GSM	------------------------------------	
	if((gps_enable==0)&&(gprs_enable==0))
		goto	save_data_section;
        
	GPIO_SetBits(GPIOA, GPIO_Pin_1);
	if((gnd_fft>=0)&&(gnd_wtr>=0)&&(env_lux>=0)&&(env_hum>=0)&&(env_tmp>=0)){
		led_on(LED_PIN);
		OSTimeDly(1000);
		led_off(LED_PIN);
		OSTimeDly(1000);
		}
	else{
		led_on(LED_PIN);
		OSTimeDly(200);
		led_off(LED_PIN);
		OSTimeDly(1800);
		}
	GPIO_ResetBits(GPIOA, GPIO_Pin_1);

	retry=30;
	while(retry--){
		uart_write_string("AT\r\n");
		uart_read(gps_buf,&gps_buf_count,1000);
		if((gps_buf_count>0)&&(strstr(gps_buf,"OK")!=0))
			break;
		gsm_err=1;
		}
	
	if(retry==-1)
		goto save_data_section;
        
	uart_write_string("ATE0\r\n");
	uart_read(gps_buf,&gps_buf_count,300);
        
   



//------------------------------GPS------------------------------------	
	if(gps_enable==0)
		goto save_data_section;
	
	gsm_err=2;//SearchingGPS ing
	uart_write_string("AT+GPSRD=0\r\n");
	uart_read(gps_buf,&gps_buf_count,300);
        
	uart_write_string("AT+GPS=1\r\n");
	uart_read(gps_buf,&gps_buf_count,500);

	uart_write_string("AT+GPSRD=2\r\n");
	uart_read(gps_buf,&gps_buf_count,300);
        
	memset(GPS_GPE,0,20);
	memset(GPS_GPL,0,20);
	memset(GPS_UTC,0,20);

	retry=150;
	count=0;
	while(retry--){
		uart_read(gps_buf,&gps_buf_count,3000);
		if(gps_buf_count>0){
			if(analysis_gps_info(gps_buf)==1)
				count++;
			if(count>10){
				gsm_err=3;
				break;
				}
			}
		}

	uart_write_string("AT+GPSRD=0\r\n");
	uart_read(gps_buf,&gps_buf_count,300);
	
	uart_write_string("AT+GPS=0\r\n");
	uart_read(gps_buf,&gps_buf_count,300);
        
	//strcpy(gps_buf,"\r\n+GPSRD:$GPGGA,004012.000,3903.25846,N,12147.03641,E,1,04,1.5,33.1,M,,M,,0000*71\r\n$GPRMC,004012.000,A,3903.25846,N,12147.03641,E,0.00,0.00,040717,,,A*69\r\n$GPVTG,0.00,T,,M,0.00,N,0.00,K,A*3D\r\n");
	//gsm_err=3;
	//analysis_gps_info(gps_buf);


	if((gsm_err==3)&&(GPS_UTC[0]!=0)){
		now.tm_mday=(GPS_UTC[0]-'0')*10+(GPS_UTC[1]-'0');
		now.tm_mon=(GPS_UTC[2]-'0')*10+(GPS_UTC[3]-'0')-1;
		now.tm_year=(GPS_UTC[4]-'0')*10+(GPS_UTC[5]-'0')+2000-1900;
		now.tm_hour=(GPS_UTC[6]-'0')*10+(GPS_UTC[7]-'0');
		now.tm_min=(GPS_UTC[8]-'0')*10+(GPS_UTC[9]-'0');
		now.tm_sec=(GPS_UTC[10]-'0')*10+(GPS_UTC[11]-'0');
		if((now.tm_sec<0)||(now.tm_sec>59))
			goto save_data_section;
		if((now.tm_min<0)||(now.tm_min>59))
			goto save_data_section;
		if((now.tm_hour<0)||(now.tm_hour>23))
			goto save_data_section;
		if((now.tm_mday<1)||(now.tm_mday>31))
			goto save_data_section;
		if((now.tm_mon<0)||(now.tm_mon>11))
			goto save_data_section;
		if(now.tm_year<117)
			goto save_data_section;

		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
		set_calendar(now);
		i=RTC_GetCounter();
		BKP_WriteBackupRegister(BKP_DR2, (unsigned short)((i&0xffff0000)>>16));
		BKP_WriteBackupRegister(BKP_DR3, (unsigned short)(i&0xffff));  
		}

//------------------------------FLASH------------------------------------	
save_data_section:
 
	max_value=0;
	max_index=0;
	for(i=0;i<STORAGE_SPACE_QUALITY;i++){
		read_data(i,tmp_str,8);
		if(tmp_str[0]==0xff)
			continue;
		tmp_str[7]=0;

		sscanf(tmp_str, "%d", &temp);
		if(temp>max_value){
			max_value=temp;
			max_index=i;
			}
		}
	
	memset(gps_buf,0,200);
	max_value=max_value%1000000;
	sprintf(gps_buf,"%07d$",max_value+1);
	//cpuid[0]=*((volatile unsigned int *)(0x1FFFF7E8));
	//cpuid[1]=*((volatile unsigned int *)(0x1ffff7EC));
	//cpuid[2]=*((volatile unsigned int *)(0x1FFFF7F0));
	//cpuid[0]&=0xFFFF;
	sprintf(tmp_str,"CID=%04X%08X%08X ",cpuid[0],cpuid[1],cpuid[2]);
	strcat(gps_buf,tmp_str);  
	
	if(env_tmp!=0xffffffff){
		sprintf(tmp_str,"TMP=%.2f ",env_tmp*0.1);
		strcat(gps_buf,tmp_str);
		}
	
	if(env_hum!=0xffffffff){
		sprintf(tmp_str,"HUM=%d ",env_hum);
		strcat(gps_buf,tmp_str);  
		}
		      
	if(env_lux!=0xffffffff){
		sprintf(tmp_str,"LUX=%d ",env_lux);
		strcat(gps_buf,tmp_str);  
		}
		      
	sprintf(tmp_str,"FFT=%d ",gnd_fft);
	strcat(gps_buf,tmp_str);  
	              
	sprintf(tmp_str,"WTR=%d ",gnd_wtr);
	strcat(gps_buf,tmp_str);  

	sprintf(tmp_str,"BAT=%d ",bat_volts);
	strcat(gps_buf,tmp_str); 

	if(GPS_GPE[0]!=0){
		sprintf(tmp_str,"GPE=%s GPL=%s ",GPS_GPE,GPS_GPL);
		strcat(gps_buf,tmp_str);     
		}

	now=get_calendar();
	sprintf(tmp_str,"TIM=%02d%02d%04d%02d%02d%02d ",now.tm_mday,
		now.tm_mon+1,now.tm_year+1900,now.tm_hour,now.tm_min,now.tm_sec);
	strcat(gps_buf,tmp_str);  

	max_index++;
	if(max_index>=STORAGE_SPACE_QUALITY)
		max_index=0;
	save_data(max_index,gps_buf,200);

//------------------------------GSM------------------------------------	
	if(gprs_enable==0)
		goto system_off_section;
                  
	gsm_err=4;
	retry=60;
	while(retry--){
		uart_write_string("AT+CREG?\r\n");
		uart_read(gps_buf,&gps_buf_count,300);
		if(gps_buf_count>0){
			if(strstr(gps_buf,"1,1\r")!=0){
				gsm_err=5;
				break;
				}
			if(strstr(gps_buf,"1,5\r")!=0){
				gsm_err=6;
				break;
				}
			if(strstr(gps_buf,"1,13\r")!=0){//unknown err: reset dtu
				uart_write_string("AT+CPOF\r\n");
				uart_read(gps_buf,&gps_buf_count,5000);
				OSTimeDly(1000);
				GPIO_SetBits(GPIOA, GPIO_Pin_1);
				OSTimeDly(2000);			
				GPIO_ResetBits(GPIOA, GPIO_Pin_1);
				OSTimeDly(5000);	
				uart_write_string("ATE0\r\n");
				uart_read(gps_buf,&gps_buf_count,300);
				gsm_err=7;
				}
			uart_write_string("AT+CREG=1\r\n");
			uart_read(gps_buf,&gps_buf_count,300);
			OSTimeDly(1000);
			}
		}
        
	if(retry==-1)
		goto system_off_section;
        

	uart_write_string("AT+CSQ\r\n");
	uart_read(gps_buf,&gps_buf_count,10000);
	if(gps_buf_count>0)
		gps_csq=(gps_buf[8]-'0')*10+gps_buf[9]-'0';
        OSTimeDly(1000);
        

        	
//------------------------------GPRS------------------------------------
 	gsm_err=8;
	retry=10;
	while(retry--){
#if 	SEND_TO == TENGXUN_SERVER
		uart_write_string("AT+CIPSTART=\"TCP\",\"123.207.144.115\",8086\r\n");
#elif 	SEND_TO == XIAOKE_SERVER
		uart_write_string("AT+CIPSTART=\"TCP\",\"jkis.jkconsulting.cn\",10068\r\n");
#else
		uart_write_string("AT+CIPSTART=\"TCP\",\"hamatec.gicp.net\",8086\r\n");
#endif
		uart_read(gps_buf,&gps_buf_count,10000);
		if((gps_buf_count==0)||(strstr(gps_buf,"OK")==0)){
			uart_write_string( "AT+CIPCLOSE\r\n");
			uart_read(gps_buf,&gps_buf_count,2000);
			gsm_err=9;
			continue;
			}
		else
			break;
		}
	if(retry==-1)
		goto system_off_section;

	
//------------------------------SEND------------------------------------	
	
	retry=12;

send_dat_section:

	if(retry==-1)
		goto system_off_section;
	
	max_value=0;
	max_index=0;
	for(i=0;i<STORAGE_SPACE_QUALITY;i++){
		read_data(i,tmp_str,8);
		if(tmp_str[0]==0xff)
			continue;
		tmp_str[7]=0;
		sscanf(tmp_str, "%d", &temp);
		if(temp>max_value){
			max_value=temp;
			max_index=i;
			}
		}
	if(max_value==0){
		uart_write_string("AT+CIPCLOSE\r\n");		//CLOSE
		OSTimeDly(500);
		gsm_err=16;
		goto system_off_section;
		}

	uart_write_string("AT+CIPSEND\r\n");			//SEND
	uart_read(gps_buf,&gps_buf_count,10000);
	if((gps_buf_count==0)||(strstr(gps_buf,">")==0)){
		gps_buf[0]=0x1a;
		uart_write(gps_buf,1);				
		OSTimeDly(300);
		gsm_err=13;
		retry--;
		goto send_dat_section;
		}

	read_data(max_index,gps_buf,200);
	for(i=0;i<192;i++)
		gps_buf[i]=gps_buf[i+8];
	for(i=192;i<200;i++)
		gps_buf[i]=0;	

#ifndef USE_SYS_FLASH
	for(i=0;i<200;i++){
	  if(gps_buf[i]==0xff){
		gps_buf[i]=0;
	  }
	}
#endif
	if(gps_csq>0){
		sprintf(tmp_str,"CSQ=%d ",gps_csq);
		strcat(gps_buf,tmp_str); 
		gps_csq=0;
		}

	check_sum_16(gps_buf,strlen(gps_buf),&chksum);
	sprintf(tmp_str,"CHK=%4.X\r\n",chksum);
	strcat(gps_buf,tmp_str);
        
	count=strlen(gps_buf);
	for(i=0;i<count;i++){
		if((i%2)==0)
			gps_buf[i]--;
		else
			gps_buf[i]++;
		}
	memset(gps_buf2,0,sizeof(gps_buf2));
	base64_encode((unsigned char *)gps_buf,gps_buf2, count);
	i=strlen(gps_buf2);
	gps_buf2[i++]=0x0A;
	gps_buf2[i++]=0x1A;
	uart_write(gps_buf2, i);
	uart_read(gps_buf,&gps_buf_count,10000);
	if((gps_buf_count==0)||(strstr(gps_buf,"OK")==0)){
		gsm_err=14;
		retry--;
		}
	else
		erase_data(max_index);
	
	goto send_dat_section;



system_off_section:	
	
	if(gsm_err==16){
		i=RTC_GetCounter();
		BKP_WriteBackupRegister(BKP_DR4, (unsigned short)((i&0xffff0000)>>16));
		BKP_WriteBackupRegister(BKP_DR5, (unsigned short)(i&0xffff));   
		}
                
	while (1) { 
	  	gsm_err=20;
	  	retry=3;
		while(retry--){
			if((gps_enable==0)&&(gprs_enable==0))
				break;
			uart_write_string("AT+CPOF\r\n");
			uart_read(gps_buf,&gps_buf_count,5000);
			if((gps_buf_count>0)&&(strstr(gps_buf,"OK")!=0))
				break;
			}
		
		GPIO_SetBits(GPIOC,GPIO_Pin_13);
		i=600;
		while(i--)
			OSTimeDly(1000);
		} 
}

/*
*********************************************************************************************************
*                                      App_TaskIO()
*
* Description :  This function creates the application tasks.
*
* Argument(s) :  none.
*
* Return(s)   :  none.
*
* Caller(s)   :  App_TaskStart().
*
* Note(s)     :  none.
*********************************************************************************************************
*/
static  void  App_TaskIO (void *p_arg)
{
	int count2;
	(void)p_arg;

	while (1) { 
		OSTimeDly(1000);
		IWDG_ReloadCounter();

		if (gsm_err==0)
			continue;
        
		count2=gsm_err/4;
		while(count2--){
			IWDG_ReloadCounter();
			OSTimeDly(400);
			led_on(LED_PIN);
			OSTimeDly(100);
			led_off(LED_PIN);
			}
		count2=gsm_err%4;
		while(count2--){
			IWDG_ReloadCounter();
			OSTimeDly(100);
			led_on(LED_PIN);
			OSTimeDly(150);
			led_off(LED_PIN);
			}
		}
}




 
