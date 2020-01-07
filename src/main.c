/**

******************************************************************************
  * @file    GPIO/GPIO_EXTI/Src/main.c
  * @author  MCD Application Team
  * @version V1.8.0
  * @date    21-April-2017
  * @brief   This example describes how to configure and use GPIOs through
  *          the STM32L4xx HAL API.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************


*/




/* Includes ------------------------------------------------------------------*/
#include "main.h"

#define PWM_TIMER                          TIM1

#define POLLING_TIMER                      TIM3

#define TEMP_POLLING_PERIOD                1000

#define ADC_TO_MV_CONVERSION_FACTOR        0.02442  // conversion factor of 3??(2^12)
#define MV_TO_TEMP_CONVERSION_FACTOR       333       // conversion factor of 1??(3??0.01)



/** @addtogroup STM32L4xx_HAL_Examples
  * @{
  */

/** @addtogroup GPIO_EXTI
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
typedef enum
{
	S_TEMP=0,
	S_SETPT
}states;
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
states curr_s;

__IO HAL_StatusTypeDef Hal_status;  //HAL_ERROR, HAL_TIMEOUT, HAL_OK, of HAL_BUSY 

ADC_HandleTypeDef    Adc_Handle;

TIM_HandleTypeDef    Tim3_Handle, Tim4_Handle;
TIM_OC_InitTypeDef Tim3_OCInitStructure, Tim4_OCInitStructure;

uint16_t ADC_raw_val;
__IO uint16_t ADC1ConvertedValue;   //if declare it as 16t, it will not work.


volatile double  setPoint=23.5;
int ii=0;

double measuredTemp; 
double setpt;

uint8_t currentSpeed;

int sel_pressed;
int up_pressed;
int down_pressed;

int setting;

uint16_t TIM4Prescaler;    // will set it to make the timer's counter run at 10Khz. ---50 ticks is 1ms.
uint16_t TIM4Period;       //will make a period to be 20ms. ----1000 ticks
__IO uint16_t TIM4_CCR1_Val=0;  // for pulse width!!! can vary it from 1 to 1000 to vary the pulse width.


char lcd_buffer[6];    // LCD display buffer


/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);

static void   PollingTimer_Config        (void);
static void   PWMTimer_Config            (void);
static void TIM3_OC_Config(void);

static void   ADC_Config                 (void);
static void readADCValMV(void);
static void showSet(void);
static void showTemp(void);
static void setFanSpeed(void);


//static void EXTILine14_Config(void); // configure the exti line4, for exterrnal button, WHICH BUTTON?


/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* STM32L4xx HAL library initialization:
       - Configure the Flash prefetch
       - Systick timer is configured by default as source of time base, but user 
         can eventually implement his proper time base source (a general purpose 
         timer for example or other time source), keeping in mind that Time base 
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4 
       - Low Level Initialization
     */
	sel_pressed=0;
	down_pressed=0;
	up_pressed=0;
	curr_s=S_TEMP;
	
	HAL_Init();

	SystemClock_Config();   

	HAL_InitTick(0x0000); // set systick's priority to the highest.

	
	BSP_LED_Init(LED4);
	BSP_LED_Init(LED5);


	BSP_LCD_GLASS_Init();
	
	BSP_JOY_Init(JOY_MODE_EXTI);  
	
	ADC_Config();
	PWMTimer_Config();
	
	PollingTimer_Config();
	TIM3_OC_Config();
	readADCValMV();
	setpt=measuredTemp+1;
	currentSpeed=0;


 	
  while (1)
  {
	switch (curr_s)
	{
		case S_TEMP:
			if(ii==1){
				readADCValMV();
				showTemp();
			}
			if ((measuredTemp-setpt)>0)
			{
				TIM4_CCR1_Val=250+(measuredTemp-setpt)*250;	 
				//  when CCR2_Val is less than 250, (with period as 1000), the motor can not run.
				//if use finger touch the sensor, the temperature sensor can vary ~3 C, so (1000-250)/3 ~=255				
				__HAL_TIM_SET_COMPARE(&Tim4_Handle, TIM_CHANNEL_1,TIM4_CCR1_Val);	
				// can not use :TIM_OCInitStructure.TIM_Pulse=TIM4_CCR1_Val;, 
				
				//Tim4_OCInitStructure.Pulse = 1000;
				//HAL_TIM_PWM_ConfigChannel(&Tim4_Handle, &Tim4_OCInitStructure, TIM_CHANNEL_1);
				//HAL_TIM_PWM_Start(&Tim4_Handle, TIM_CHANNEL_1);
				//one group of student use this function, in this place and cause the output pin always high.
				currentSpeed=100;
				setting=2;
				BSP_LED_Toggle(LED5);
				BSP_LED_Toggle(LED4);
				if (measuredTemp-setpt>5){
					setting=1;
					BSP_LED_On(LED5);
					BSP_LED_On(LED4);
				}
			}
			else 
			{
				TIM4_CCR1_Val=0;
				__HAL_TIM_SET_COMPARE(&Tim4_Handle, TIM_CHANNEL_1, TIM4_CCR1_Val);	 
				currentSpeed=0;
				setting=0;
				BSP_LED_Off(LED4);
				BSP_LED_Off(LED5);
			}
			
			
			if(sel_pressed==1){
				sel_pressed=0;
				curr_s=S_SETPT;
				break;
			}
			setFanSpeed();
			break;
				
		case S_SETPT:

			if (down_pressed==1)
			{
				down_pressed=0;
				setpt=setpt-0.5;
			}
			if (up_pressed==1)
			{
				up_pressed=0;
				setpt=setpt+0.5;
			}
			if (setting==1){
				BSP_LED_On(LED5);
				BSP_LED_On(LED4);
			}
			else if(setting==2)
			{
				BSP_LED_Toggle(LED5);
				BSP_LED_Toggle(LED4);
			}
			else
			{
				BSP_LED_Off(LED5);
				BSP_LED_Off(LED4);
			}
			setFanSpeed();
			showSet();
			if(sel_pressed==1){
				sel_pressed=0;
				curr_s=S_TEMP;
				//break;
			}
			break;
		
			
	}
	
			
	} //end of while 1

}


void setFanSpeed(void)
{
  //PWMTimer_Config(currentSpeed);
}


void showSet(void)
{
	BSP_LCD_GLASS_Clear();
	snprintf(lcd_buffer, 7, "%5.2fC", setpt);

	BSP_LCD_GLASS_DisplayString((uint8_t*)lcd_buffer);
	
}

void showTemp(void)
{
	BSP_LCD_GLASS_Clear();
	snprintf(lcd_buffer, 7, "%5.2fC", measuredTemp);

	BSP_LCD_GLASS_DisplayString((uint8_t*)lcd_buffer);
	
}

void readADCValMV(void)
{
  if (HAL_ADC_PollForConversion(&Adc_Handle, 10) != HAL_OK)
    {
      Error_Handler();
    }
    uint32_t output = HAL_ADC_GetValue(&Adc_Handle);
		double blab = output *ADC_TO_MV_CONVERSION_FACTOR;
    output *= ADC_TO_MV_CONVERSION_FACTOR;
    measuredTemp = blab;
}

static double getTemp(void)
{
 // double ADCVal = (double) readADCValMV();
  /* Divide ADC value in mv by 3 to remove gain, and divide by 10 to convert to celcius */
  // (ADCVal * MV_TO_TEMP_CONVERSION_FACTOR);
}

static void ADC_Config(void)
{
  ADC_ChannelConfTypeDef   adcConfig;
  ADC_AnalogWDGConfTypeDef AnalogWDGConfig;

  /* Configuration of ADCx init structure: ADC parameters and regular group */
  Adc_Handle.Instance = ADC1;

   if (HAL_ADC_DeInit(&Adc_Handle) != HAL_OK)
  {
    /* ADC de-initialization Error */
    Error_Handler();
  }


  Adc_Handle.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;          /* Asynchronous clock mode, input ADC clock not divided */
  Adc_Handle.Init.Resolution            = ADC_RESOLUTION_12B;             /* 12-bit resolution for converted data */
  Adc_Handle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;           /* Right-alignment for converted data */
  Adc_Handle.Init.ScanConvMode          = DISABLE;                       /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
  Adc_Handle.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;           /* EOC flag picked-up to indicate conversion end */
  Adc_Handle.Init.LowPowerAutoWait      = DISABLE;                       /* Auto-delayed conversion feature disabled */
  Adc_Handle.Init.ContinuousConvMode    = ENABLE;                        /* Continuous mode enabled (automatic conversion restart after each conversion) */
  Adc_Handle.Init.NbrOfConversion       = 1;                             /* Parameter discarded because sequencer is disabled */
  Adc_Handle.Init.DiscontinuousConvMode = DISABLE;                       /* Parameter discarded because sequencer is disabled */
  Adc_Handle.Init.NbrOfDiscConversion   = 1;                             /* Parameter discarded because sequencer is disabled */
  Adc_Handle.Init.ExternalTrigConv      = ADC_SOFTWARE_START;            /* Software start to trig the 1st conversion manually, without external event */
  Adc_Handle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE; /* Parameter discarded because software trigger chosen */
  Adc_Handle.Init.DMAContinuousRequests = DISABLE;                       /* DMA one-shot mode selected (not applied to this example) */
  Adc_Handle.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;      /* DR register is overwritten with the last conversion result in case of overrun */
  Adc_Handle.Init.OversamplingMode      = DISABLE;

  if (HAL_ADC_Init(&Adc_Handle) != HAL_OK)
  {
    /* ADC initialization error */
    Error_Handler();
  }

  /* ### - 2 - Start calibration ############################################ */
  if (HAL_ADCEx_Calibration_Start(&Adc_Handle, ADC_SINGLE_ENDED) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configuration of channel on ADCx regular group on sequencer rank 1 */
  /* Note: Considering IT occurring after each ADC conversion if ADC          */
  /*       conversion is out of the analog watchdog window selected (ADC IT   */
  /*       enabled), select sampling time and ADC clock with sufficient       */
  /*       duration to not create an overhead situation in IRQHandler.        */
  adcConfig.Channel      = ADC_CHANNEL_6;                /* Sampled channel number */
  adcConfig.Rank         = ADC_REGULAR_RANK_1;          /* Rank of sampled channel number ADCx_CHANNEL */
  adcConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;    /* Sampling time (number of clock cycles unit) */
  adcConfig.SingleDiff   = ADC_SINGLE_ENDED;            /* Single-ended input channel */
  adcConfig.OffsetNumber = ADC_OFFSET_NONE;             /* No offset subtraction */
  adcConfig.Offset = 0;                                 /* Parameter discarded because offset correction is disabled */

  if (HAL_ADC_ConfigChannel(&Adc_Handle, &adcConfig) != HAL_OK)
  {
    /* Channel Configuration Error */
    Error_Handler();
  }

  /* ### - 4 - Start conversion ############################################# */
  if (HAL_ADC_Start(&Adc_Handle) != HAL_OK)
  {
    Error_Handler();
  }

}

void PollingTimer_Config()
{
  /* Compute the prescaler value to have PollingTimer counter clock equal to 1 KHz */
  uint16_t PrescalerValue = (uint16_t)(SystemCoreClock / 1000) - 1;

  /* Set PollingTimer instance */
  Tim3_Handle.Instance = POLLING_TIMER;

  Tim3_Handle.Init.Period = 40000 - 1;
  Tim3_Handle.Init.Prescaler = PrescalerValue;
  Tim3_Handle.Init.ClockDivision = 0;
  Tim3_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;

  if (HAL_TIM_Base_Init(&Tim3_Handle) != HAL_OK) // this line need to call the callback function _MspInit() in stm32f4xx_hal_msp.c to set up peripheral clock and NVIC..
  {
    /* Initialization Error */
    Error_Handler();
  }
  /*##-2- Start the TIM Base generation in interrupt mode ####################*/
  /* Start Channel1 */
  if (HAL_TIM_Base_Start_IT(&Tim3_Handle) != HAL_OK) //the TIM_XXX_Start_IT function enable IT, and also enable Timer
  //so do not need HAL_TIM_BASE_Start() any more.
  {
    /* Starting Error */
    Error_Handler();
  }
}

void TIM3_OC_Config(void)
{
	Tim3_OCInitStructure.OCMode = TIM_OCMODE_TIMING;
	Tim3_OCInitStructure.Pulse = 5000;		//10000/10000 = 1s
	Tim3_OCInitStructure.OCPolarity = TIM_OCPOLARITY_HIGH;
	
	HAL_TIM_OC_Init(&Tim3_Handle);
	
	HAL_TIM_OC_ConfigChannel(&Tim3_Handle,&Tim3_OCInitStructure,TIM_CHANNEL_1);
	
	HAL_TIM_OC_Start_IT(&Tim3_Handle, TIM_CHANNEL_1);
					
}

void PWMTimer_Config(void)
{
  //TIM3 TIM4 are 16 bits, TIM2 and TIM5 are 32 bits	
//with the settings in the sysclock_config(), the sysclk is 80Mhz, so is the AHB, APB1 and APB2. 
//since the the prescaler of the APB is 1, the timeer's frequncy is 80Mhz. 

  TIM4Prescaler=(uint16_t) (SystemCoreClock/ 50000) - 1;    //set the frequency as 50 Khz (50 ticks is 1ms). the prescaler is less than 65535, is OK.
	TIM4Period=1000;   //this makes the period as 20ms 
	
	Tim4_Handle.Instance = TIM4; //TIM3 is defined in stm32f429xx.h
   
  Tim4_Handle.Init.Period = TIM4Period; //pwm frequency? 
  Tim4_Handle.Init.Prescaler = TIM4Prescaler;
  Tim4_Handle.Init.ClockDivision = 0;
  Tim4_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
 
 
	if(HAL_TIM_PWM_Init(&Tim4_Handle) != HAL_OK)
  {
    Error_Handler();
  }
  
	//configure the PWM channel
	Tim4_OCInitStructure.OCMode=  TIM_OCMODE_PWM1; //TIM_OCMODE_TIMING;
	Tim4_OCInitStructure.OCFastMode=TIM_OCFAST_DISABLE;
	Tim4_OCInitStructure.OCPolarity=TIM_OCPOLARITY_HIGH;
	Tim4_OCInitStructure.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
	Tim4_OCInitStructure.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	 Tim4_OCInitStructure.OCIdleState  = TIM_OCIDLESTATE_RESET;
	Tim4_OCInitStructure.Pulse=TIM4_CCR1_Val;
	
	if(HAL_TIM_PWM_ConfigChannel(&Tim4_Handle, &Tim4_OCInitStructure, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
	
	if(HAL_TIM_PWM_Start(&Tim4_Handle, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }  
	
}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follows :
  *            System Clock source            = MSI
  *            SYSCLK(Hz)                     = 4000000
  *            HCLK(Hz)                       = 4000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            MSI Frequency(Hz)              = 4000000
  *            Flash Latency(WS)              = 0
  * @param  None
  * @retval None
  */


void SystemClock_Config(void)
{ 
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  // RTC requires to use HSE (or LSE or LSI, suspect these two are not available)
  //reading from RTC requires the APB clock is 7 times faster than HSE clock,
  //so turn PLL on and use PLL as clock source to sysclk (so to APB)
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;     //RTC need either HSE, LSE or LSI

  RCC_OscInitStruct.LSEState = RCC_LSE_ON;

  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6; // RCC_MSIRANGE_6 is for 4Mhz. _7 is for 8 Mhz, _9 is for 16..., _10 is for 24 Mhz, _11 for 48Hhz
  RCC_OscInitStruct.MSICalibrationValue= RCC_MSICALIBRATION_DEFAULT;

    //RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;//RCC_PLL_NONE;

  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;   //PLL source: either MSI, or HSI or HSE, but can not make HSE work.
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLR = 2;  //2,4,6 or 8
  RCC_OscInitStruct.PLL.PLLP = 7;   // or 17.
  RCC_OscInitStruct.PLL.PLLQ = 4;   //2, 4,6, 0r 8
    //the PLL will be MSI (4Mhz)*N /M/R =

  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    // Initialization Error
    while(1);
  }

  // configure the HCLK, PCLK1 and PCLK2 clocks dividers
  // Set 0 Wait State flash latency for 4Mhz
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; //the freq of pllclk is MSI (4Mhz)*N /M/R = 80Mhz
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;


  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)   //???
  {
    // Initialization Error
    while(1);
  }

  // The voltage scaling allows optimizing the power consumption when the device is
  //   clocked below the maximum system frequency, to update the voltage scaling value
  //   regarding system frequency refer to product datasheet.

  // Enable Power Control clock
  __HAL_RCC_PWR_CLK_ENABLE();

  if(HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2) != HAL_OK)
  {
    // Initialization Error
    while(1);
  }

  // Disable Power Control clock   //why disable it?
  __HAL_RCC_PWR_CLK_DISABLE();
}




/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin) {
			case GPIO_PIN_0: 		               //SELECT button					
						sel_pressed=1;
						break;	
			case GPIO_PIN_1:     //left button						
							
							break;
			case GPIO_PIN_2:    //right button						  to play again.
						
							break;
			case GPIO_PIN_3:    //up button							
				up_pressed=1;
							break;
			
			case GPIO_PIN_5:    //down button						
					down_pressed=1;
							break;
			
			default://
						//default
						break;
	  } 
}


void HAL_TIM_PeriodElapsedCallback ( TIM_HandleTypeDef * htim )
{
  /* If general timer set corresponding flag */
  if (htim->Instance == POLLING_TIMER)
  {
	ii=1;
  }
}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef * htim) //see  stm32XXX_hal_tim.c for different callback function names. 
{																																//for timer4 
		if ((*htim).Instance==TIM3){
//					sprintf(lcd_buffer,"b%5d",ADC1ConvertedValue);
//					BSP_LCD_GLASS_DisplayString((uint8_t *)lcd_buffer);
					//HAL_Delay(300);   //seems that with this set of Hal library, the HAL_Delay can not be used in any ISR.
				//clear the timer counter!  in stm32f4xx_hal_tim.c, the counter is not cleared after  OC interrupt
				__HAL_TIM_SET_COUNTER(htim, 0x0000);   //this maro is defined in stm32f4xx_hal_tim.h
		}
}
 
 
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef * htim){  //this is for TIM4_pwm
	
	__HAL_TIM_SET_COUNTER(htim, 0x0000);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
{

}



static void Error_Handler(void)
{
  /* Turn LED4 on */
  BSP_LED_On(LED4);
  while(1)
  {
  }
}





#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
