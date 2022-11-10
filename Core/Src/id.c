#include "id.h"

#include "main.h"
#include "cmsis_os.h"
#include "gpio.h"

uint8_t address=0xff;

/*
Address£ºPA15 PB3 PB4 PB5 PB6 PB7 PA7 PA6
*/
typedef  struct _PINS
{
	GPIO_TypeDef* GPIOBase;
	uint16_t GPIONum;
}PINS;
PINS Addrinput[8]={ {GPIOA,GPIO_PIN_15},
										{GPIOB,GPIO_PIN_3},
										{GPIOB,GPIO_PIN_4},
										{GPIOB,GPIO_PIN_5},
										{GPIOB,GPIO_PIN_6},
										{GPIOB,GPIO_PIN_7},
										{GPIOA,GPIO_PIN_7},
										{GPIOA,GPIO_PIN_6}};
void Getaddress(void)
{
	uint8_t  addrtmp=0x00;
	for(uint8_t i=0;i<8;i++)
	{
		addrtmp=addrtmp<<1|(~HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12));
	}
	address=addrtmp;
}
