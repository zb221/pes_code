/*
*********************************************************************************************************
*
*                                        BOARD SUPPORT PACKAGE
*
*                                     ST Microelectronics STM32
*                                              on the 
*                                 IAR STM32F103ZE-SK Evaluation Board
*
* Filename      : bsp.c
* Version       : V1.00
* Programmer(s) : FT
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#define  BSP_MODULE
#include <bsp.h>

#define  BSP_RCC_TO_VAL                  0x00000FFF             /* Max Timeout for RCC register                             */

/*
*********************************************************************************************************
*                                       BSP_CPU_ClkFreq()
*
* Description : This function reads CPU registers to determine the CPU clock frequency of the chip in KHz.
*
* Argument(s) : none.
*
* Return(s)   : The CPU clock frequency, in Hz.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT32U  BSP_CPU_ClkFreq (void)
{
    static  RCC_ClocksTypeDef  rcc_clocks;

    RCC_GetClocksFreq(&rcc_clocks);
	
    return ((CPU_INT32U)rcc_clocks.HCLK_Frequency);
}

/*
*********************************************************************************************************
*                                         OS_CPU_SysTickClkFreq()
*
* Description : Get system tick clock frequency.
*
* Argument(s) : none.
*
* Return(s)   : Clock frequency (of system tick).
*
* Caller(s)   : BSP_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

INT32U  OS_CPU_SysTickClkFreq (void)
{
    INT32U  freq;

    freq = BSP_CPU_ClkFreq();
    return (freq);
}

/*
*********************************************************************************************************
*                                         BSP_RCC_Init()
*
* Description : Initializes the RCC module. Set the FLASH memmory timing and the system clock dividers
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : BSP_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  BSP_RCC_Init (void) 
{    
    CPU_INT32U  rcc_to;                                          /* RCC registers timeout                                    */
        
                    
    RCC_DeInit();                                                /*  Reset the RCC clock config to the default reset state   */

    RCC_HSEConfig(RCC_HSE_ON);                                   /*  HSE Oscillator ON                                       */        
    
    
    rcc_to = BSP_RCC_TO_VAL;
    
    while ((rcc_to > 0) &&
           (RCC_WaitForHSEStartUp() != SUCCESS)) {               /* Wait until the oscilator is stable                       */
        rcc_to--;
    }
      
    FLASH_SetLatency(FLASH_Latency_2);
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
    
    RCC_PLLConfig(RCC_PLLSource_HSE_Div2, RCC_PLLMul_9);         /* Fcpu = (PLL_src * PLL_MUL) = (8 Mhz / 2) * (9) = 36Mhz   */ 
    RCC_PLLCmd(ENABLE);

    rcc_to = BSP_RCC_TO_VAL;

    while ((rcc_to > 0) &&
           (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)) {
        rcc_to--;
    }

    RCC_HCLKConfig(RCC_SYSCLK_Div1);                             /* Set system clock dividers                                */                            
    RCC_PCLK2Config(RCC_HCLK_Div1);    
    RCC_PCLK1Config(RCC_HCLK_Div2);    
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    FLASH_SetLatency(FLASH_Latency_2);                           /* Embedded Flash Configuration                             */
    FLASH_HalfCycleAccessCmd(FLASH_HalfCycleAccess_Disable);
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
  
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

}



/*
*********************************************************************************************************
*                                         BSP_GPIO_Init()
*
* Description : Initializes the board's GPIO.
*
* Argument(s) : none
*
* Return(s)   : none.
*
* Caller(s)   : BSP_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  BSP_GPIO_Init (void)
{
    GPIO_InitTypeDef  gpio_init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    gpio_init.GPIO_Pin   = (GPIO_Pin_1|GPIO_Pin_5|GPIO_Pin_7);
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &gpio_init);
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    gpio_init.GPIO_Pin   = (GPIO_Pin_1);
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &gpio_init);

    RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOC, ENABLE);
    gpio_init.GPIO_Pin   = (GPIO_Pin_13);
    GPIO_Init(GPIOC, &gpio_init);
	
    GPIO_SetBits(GPIOB,GPIO_Pin_0|GPIO_Pin_1);
    GPIO_ResetBits(GPIOC,GPIO_Pin_13);
}

/*
*********************************************************************************************************
*                                         BSP_TM3_Init()
*
* Description : Initializes the board's TM3.
*
* Argument(s) : none
*
* Return(s)   : none.
*
* Caller(s)   : BSP_Init().
*
* Note(s)     : none.
*********************************************************************************************************

static  void  BSP_TM3_Init (void)
{
    GPIO_InitTypeDef  gpio_init;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);
    
    gpio_init.GPIO_Pin   = GPIO_Pin_7;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio_init);
    
    TIM_TimeBaseStructure.TIM_Period = 3;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 2;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    TIM_OC2Init(TIM3, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
}
*/

/*
*********************************************************************************************************
*                                         BSP_USART2_UART_Init()
*
* Description : Initializes the board's USART2.
*
* Argument(s) : none
*
* Return(s)   : none.
*
* Caller(s)   : BSP_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  BSP_USART2_UART_Init (void)
{
    USART_InitTypeDef USART_InitStructure;
    USART_ClockInitTypeDef  USART_ClockInitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); 

    USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
    USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
    USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
    USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;
    USART_ClockInit(USART2, &USART_ClockInitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;

    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    USART_Cmd(USART2, ENABLE);
    
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQChannel;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

}
 



/*
*********************************************************************************************************
*                                         BSP_ADC1_Init()
*
* Description : Initializes the board's ADC1.
*
* Argument(s) : none
*
* Return(s)   : none.
*
* Caller(s)   : BSP_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/
static  void  BSP_ADC1_Init (void)
{
  
  GPIO_InitTypeDef GPIO_InitStructure;
  ADC_InitTypeDef ADC_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_4|GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 , ENABLE);  
  ADC_DeInit(ADC1);
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, &ADC_InitStructure);
  ADC_Cmd(ADC1, ENABLE);
  
  ADC_ResetCalibration(ADC1);
  while(ADC_GetResetCalibrationStatus(ADC1));
  ADC_StartCalibration(ADC1);
  while(ADC_GetCalibrationStatus(ADC1));
}




/*
*********************************************************************************************************
*                                         BSP_I2C2_Software_Simu_Init()
*
* Description : Initializes the board's I2C2.
*
* Argument(s) : none
*
* Return(s)   : none.
*
* Caller(s)   : BSP_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  BSP_I2C2_Software_Simu_Init (void)
{
      GPIO_InitTypeDef GPIO_InitStructure;  
      GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;  
      GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_OD;  
      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11|GPIO_Pin_10;  
      GPIO_Init(GPIOB, &GPIO_InitStructure);  
}


/*
*********************************************************************************************************
*                                         RTC_Configuration()
*
* Description : Initializes the board's RTC.
*
* Argument(s) : none
*
* Return(s)   : none.
*
* Caller(s)   : BSP_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static void RTC_Configuration(void)
{

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
  PWR_BackupAccessCmd(ENABLE);
  
  if(BKP_ReadBackupRegister(BKP_DR6)!=0xA5A5){
    BKP_DeInit();
    RCC_LSEConfig(RCC_LSE_ON);
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);
  
    RTC_WaitForSynchro();
    RTC_WaitForLastTask();
    RTC_SetPrescaler(32767);
    RTC_WaitForLastTask();
    BKP_WriteBackupRegister(BKP_DR6, 0xA5A5);
  }

  
}


/*
*********************************************************************************************************
*                                         IWDG_Configuration()
*
* Description : Initializes the board's RTC.
*
* Argument(s) : none
*
* Return(s)   : none.
*
* Caller(s)   : BSP_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void IWDG_Configuration(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	IWDG_SetPrescaler(IWDG_Prescaler_64);
	IWDG_SetReload(0x0fff);
	IWDG_ReloadCounter();
	IWDG_Enable();
}



/*
*********************************************************************************************************
*                                            BSP_Init()
*
* Description : This function should be called by your application code before you make use of any of the
*               functions found in this module.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_Init (void)
{    
    BSP_RCC_Init(); 
    RTC_Configuration();
    BSP_GPIO_Init();
    //BSP_TM3_Init();
    BSP_ADC1_Init();
    BSP_I2C2_Software_Simu_Init();
    BSP_USART2_UART_Init();
    IWDG_Configuration();
}
















