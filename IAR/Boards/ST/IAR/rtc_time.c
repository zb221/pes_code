/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_lib.h"
#include "RTC_Time.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void Time_Set(u32 t);
/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
* Function Name  : Time_ConvUnixToCalendar(time_t t)
* Description    : ת��UNIXʱ���Ϊ����ʱ��
* Input    : u32 t  ��ǰʱ���UNIXʱ���
* Output   : None
* Return   : struct tm
*******************************************************************************/
struct tm Time_ConvUnixToCalendar(time_t t)
{
 struct tm *t_tm;
 t_tm = localtime(&t);
 t_tm->tm_year += 1900; 
 return *t_tm;
}

/*******************************************************************************
* Function Name  : Time_ConvCalendarToUnix(struct tm t)
* Description    : д��RTCʱ�ӵ�ǰʱ��
* Input    : struct tm t
* Output   : None
* Return   : time_t
*******************************************************************************/
time_t Time_ConvCalendarToUnix(struct tm t)
{
 t.tm_year -= 1900;  //�ⲿtm�ṹ��洢�����Ϊ2008��ʽ
      //��time.h�ж������ݸ�ʽΪ1900�꿪ʼ�����
      //���ԣ�������ת��ʱҪ���ǵ�������ء�
 return mktime(&t);
}

/*******************************************************************************
* Function Name  : Time_GetUnixTime()
* Description    : ��RTCȡ��ǰʱ���Unixʱ���ֵ
* Input    : None
* Output   : None
* Return   : time_t t
*******************************************************************************/
time_t Time_GetUnixTime(void)
{
 return (time_t)RTC_GetCounter();
}

/*******************************************************************************
* Function Name  : Time_GetCalendarTime()
* Description    : ��RTCȡ��ǰʱ�������ʱ�䣨struct tm��
* Input    : None
* Output   : None
* Return   : time_t t
*******************************************************************************/
struct tm Time_GetCalendarTime(void)
{
 time_t t_t;
 struct tm t_tm;

 t_t = (time_t)RTC_GetCounter();
 t_tm = Time_ConvUnixToCalendar(t_t);
 return t_tm;
}

/*******************************************************************************
* Function Name  : Time_SetUnixTime()
* Description    : ��������Unixʱ���д��RTC
* Input    : time_t t
* Output   : None
* Return   : None
*******************************************************************************/
void Time_SetUnixTime(time_t t)
{
	PWR_BackupAccessCmd(ENABLE);
	RTC_WaitForLastTask();
	RTC_SetCounter((u32)t);
	RTC_WaitForLastTask();
	PWR_BackupAccessCmd(DISABLE);
	return;
}

/*******************************************************************************
* Function Name  : Time_SetCalendarTime()
* Description    : ��������Calendar��ʽʱ��ת����UNIXʱ���д��RTC
* Input    : struct tm t
* Output   : None
* Return   : None
*******************************************************************************/
void Time_SetCalendarTime(struct tm t)
{
	Time_SetUnixTime(Time_ConvCalendarToUnix(t));
	return;
}