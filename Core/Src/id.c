#include "id.h"

#include "main.h"
#include "cmsis_os.h"
#include "gpio.h"

uint8_t address=0x0;

/*
Address£ºPA6 PA7 PB7 PB6 PB5 PB4 PB3 PA15
*/
typedef  struct _PINS
{
	GPIO_TypeDef* GPIOBase;
	uint16_t GPIONum;
}PINS;
PINS Addrinput[8]={ {GPIOA,GPIO_PIN_6},
										{GPIOA,GPIO_PIN_7},
										{GPIOB,GPIO_PIN_7},
										{GPIOB,GPIO_PIN_6},
										{GPIOB,GPIO_PIN_5},
										{GPIOB,GPIO_PIN_4},
										{GPIOB,GPIO_PIN_3},
										{GPIOA,GPIO_PIN_15}};
void Getaddress(void)
{
	for(uint8_t i=0;i<8;i++)
	{
		address=address<<1 | ((~HAL_GPIO_ReadPin(Addrinput[i].GPIOBase,Addrinput[i].GPIONum))&0x01);
	}
}
