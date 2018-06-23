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
* Description    : 转换UNIX时间戳为日历时间
* Input    : u32 t  当前时间的UNIX时间戳
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
* Description    : 写入RTC时钟当前时间
* Input    : struct tm t
* Output   : None
* Return   : time_t
*******************************************************************************/
time_t Time_ConvCalendarToUnix(struct tm t)
{
 t.tm_year -= 1900;  //外部tm结构体存储的年份为2008格式
      //而time.h中定义的年份格式为1900年开始的年份
      //所以，在日期转换时要考虑到这个因素。
 return mktime(&t);
}

/*******************************************************************************
* Function Name  : Time_GetUnixTime()
* Description    : 从RTC取当前时间的Unix时间戳值
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
* Description    : 从RTC取当前时间的日历时间（struct tm）
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
* Description    : 将给定的Unix时间戳写入RTC
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
* Description    : 将给定的Calendar格式时间转换成UNIX时间戳写入RTC
* Input    : struct tm t
* Output   : None
* Return   : None
*******************************************************************************/
void Time_SetCalendarTime(struct tm t)
{
	Time_SetUnixTime(Time_ConvCalendarToUnix(t));
	return;
}