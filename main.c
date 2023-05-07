
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

uint32_t systick_cnt = 0;
void SysTick_Handler()
{
	systick_cnt++;
}
void system_tick_init()
{
	uint32_t* CSR = (uint32_t* )0xe000e010;
	uint32_t* RVR = (uint32_t* )0xe000e014;

	*RVR = 15999;
	*CSR |= (1<<1)|(1<<0)|(1<<2);
}

void SystemInit()
{
	
}

void Custom_delay(uint32_t mSec)
{
	systick_cnt = 0;
	while(systick_cnt < mSec);
}
#define GPIOD_BASE_ADDRESS 0x40020C00
#define GPIOA_BASE_ADDRESS 0x40020000
void led_init()
{
	uint32_t* RCC_AHB1ENR = (uint32_t*)(0x40023800 + 0x30);
	*RCC_AHB1ENR |= (1<<3);
	uint32_t* GPIOD_MODER  = (uint32_t*)(GPIOD_BASE_ADDRESS + 0x00);
	uint32_t* GPIOD_OTYPER = (uint32_t*)(GPIOD_BASE_ADDRESS + 0x04);
	*GPIOD_MODER &= ~(0xff << 24);
	*GPIOD_MODER |= (0b01 << 24) | (0b01 << 26) | (0b01 << 28) | (0b01 << 30);
	*GPIOD_OTYPER &= ~(0xf << 12);
}
typedef enum
{
	LED_1 = 12, LED_2, LED_3, LED_4
}led_num_t;
typedef enum
{
	LED_OFF, LED_ON
} led_state_t;
void led_ctrl(led_num_t led_num, led_state_t state)
{
	uint32_t* GPIOD_ODR  = (uint32_t*)(GPIOD_BASE_ADDRESS + 0x14);
	if(state == LED_ON)
		*GPIOD_ODR |= (1<<led_num);
	else
		*GPIOD_ODR &= ~(1<<led_num);
}

void UART1_Init()
{
	uint32_t* RCC_AHB1 = (uint32_t*)(0x40023800 + 0x30);
	*RCC_AHB1 |= (1<< 1);
	uint32_t* RCC_APB2ENR = (uint32_t*)(0x40023800 + 0x44);
	*RCC_APB2ENR |= (1<<4);

	uint32_t* MODER = (uint32_t*)(0x40020400);
	*MODER |= (0b10 << 12) | (0b10 << 14);		//set PB6 (U1Tx), PB7(U1Rx)

	uint32_t* AFRL = (uint32_t*)(0x40020420);
	*AFRL  |= (0b0111 << 24) | (0b0111 << 28);

	uint32_t* BRR = (uint32_t*)(0x40011008);
	*BRR = (104<<4) | 3;

	uint32_t* CR3 = (uint32_t*)(0x40011014);
	*CR3 |= 1 << 6;							//enable DMA for receiver

	uint32_t* CR1 = (uint32_t*)(0x4001100c);
	*CR1 |= (1<< 3)|(1<<2)|(1<<13);
}
void UART1_Send(char data)
{
	uint32_t* SR = (uint32_t*)(0x40011000);
	uint32_t* DR = (uint32_t*)(0x40011004);
	while(((*SR >> 7) & 1) != 1);
	*DR	= data;
	while(((*SR >> 6) & 1) != 1);
	*SR &= ~(1 << 6);
}

void print_log(char* format, ...)
{
	char msg[128] = { 0 };
	va_list _ArgList ;
	va_start(_ArgList, format);
	vsprintf(msg, format, _ArgList);
	va_end(_ArgList);
	int msg_len = strlen(msg);
	for(int i = 0; i < msg_len; i++)
	{
		UART1_Send(msg[i]);
	}
}


void ADC_Init()
{
	uint32_t* RCC_APB2ENR = (uint32_t*)(0x40023800 + 0x44);
	*RCC_APB2ENR |= (1<<8);			//enable clock for ADC1
	uint32_t* CR2 = (uint32_t*)0x40012008;
	uint32_t* JSQR = (uint32_t*)0x40012038;
	*JSQR |= (16<<15);		//set channel 16 (temp sensor) for JSQ4
	uint32_t* CCR = (uint32_t*)0x40012304;
	*CCR |= (1<<23);
	uint32_t* SMPR1 = (uint32_t*)0x4001200c;
	*SMPR1 |= (0b111 << 18);
	*CR2 |= 1; 
}

float ADC_Get_Temp()
{
	uint32_t* CR2 = (uint32_t*)0x40012008;
	*CR2 |= (1<<22);
	uint32_t* SR = (uint32_t*)0x40012000;
	uint32_t current_tick = systick_cnt;
	uint32_t timeout = 1000;

	while(((*SR >>2)&1) == 0){
		if((current_tick + timeout) > systick_cnt)
			return -1;	
	}

	*SR &= ~(1<<2);
	uint32_t* JDR1 = (uint32_t*)0x4001203c;
	uint16_t raw_data = *JDR1;
	float vsense = (3000 * raw_data) /4095.0;
	float temp = ((vsense - 760) / 2.5) + 25;
	return temp;

}
void main()
{
	led_init();
	system_tick_init();
	ADC_Init();
	UART1_Init();
	while(1)
	{
		print_log("raw_data: %.3f oC\r\n", ADC_Get_Temp());
		Custom_delay(1000);
	}
}
