// balmer@inbox.ru RLC Meter 303
// 2013-2014

#include "hw_config.h"
#include "quadrature_encoder.h"
#include "lcd_interface.h"

//-------------------------------------------#DEFINE------------------------------------------
static bool g_lastButtonState = false;

#ifdef DEF_INTERFACE_BUTTONS
	static bool g_lastButtonStateA = false;
	static bool g_lastButtonStateB = false;
#else
	static uint16_t g_lastEncValue = 0;
#endif

void QuadInit(void);
void QuadTimerButton(void);

void QuadEncInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	QuadInit();

	GPIO_StructInit(&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = BUTTON_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(BUTTON_GPIO, &GPIO_InitStructure);

	QuadTimerButton();
}

void QuadInit()
{
	GPIO_InitTypeDef GPIO_InitStructure;

#ifdef DEF_INTERFACE_BUTTONS
	
	// 2 Inputs for A and B Encoder Channels
	GPIO_InitStructure.GPIO_Pin = Codeur_A | Codeur_B;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(Codeur_GPIO, &GPIO_InitStructure);

#else //!DEF_INTERFACE_BUTTONS

	RCC_AHBPeriphClockCmd(Codeur_RCC, ENABLE);
	RCC_APB1PeriphClockCmd(Codeur_TIMER_RCC, ENABLE);

 	// 2 Inputs for A and B Encoder Channels
	GPIO_InitStructure.GPIO_Pin = Codeur_A | Codeur_B;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(Codeur_GPIO, &GPIO_InitStructure);
 
	// Timer AF Pins Configuration
	GPIO_PinAFConfig(Codeur_GPIO, Codeur_A_SOURCE, Codeur_AF);
	GPIO_PinAFConfig(Codeur_GPIO, Codeur_B_SOURCE, Codeur_AF);
 
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
 
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_Period = 65535; // Maximal
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
 
	TIM_TimeBaseInit(Codeur_TIMER, &TIM_TimeBaseStructure);
 
	// TIM_EncoderMode_TI1: Counter counts on TI1FP1 edge depending on TI2FP2 level.
	TIM_EncoderInterfaceConfig(Codeur_TIMER, TIM_EncoderMode_TI1, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);

	TIM_Cmd(Codeur_TIMER, ENABLE);
#endif
}

uint16_t QuadEncValue()
{
	return TIM_GetCounter(Codeur_TIMER);
}

bool QuadEncButton()
{
	return (GPIO_ReadInputDataBit(BUTTON_GPIO, BUTTON_PIN) == 0);
}

void QuadTimerButton()
{
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);

	//TIM_TimeBaseStructure.TIM_Period = 2000;			// 1 sec
	TIM_TimeBaseStructure.TIM_Period = 40;				// 20 msec
	TIM_TimeBaseStructure.TIM_Prescaler = 72 * 500 - 1;	// 2000 tick per sec
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  

	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure);
	TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);
	TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
	TIM_Cmd(TIM7, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
	NVIC_Init(&NVIC_InitStructure);
}

void Timer_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)
	{
		bool buttonState = QuadEncButton();
		if(buttonState && !g_lastButtonState)
			OnButtonPressed();   
		g_lastButtonState = buttonState;

#ifdef DEF_INTERFACE_BUTTONS
		bool buttonStateA = (GPIO_ReadInputDataBit(Codeur_GPIO, Codeur_A)==0);
		if(buttonStateA && !g_lastButtonStateA)
			OnWeel(+1);   
		g_lastButtonStateA = buttonStateA;

		bool buttonStateB = (GPIO_ReadInputDataBit(Codeur_GPIO, Codeur_B)==0);
		if(buttonStateB && !g_lastButtonStateB)
			OnWeel(-1);   
		g_lastButtonStateB = buttonStateB;
#else
		uint16_t encValue = QuadEncValue();
		if(encValue!=g_lastEncValue)
		{
			OnWeel(encValue-g_lastEncValue);
		}

		g_lastEncValue = encValue;
#endif

		OnTimer();

		TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
	}
}
