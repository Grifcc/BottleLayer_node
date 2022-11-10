#ifndef __ID_H
#define __ID_H

#include "main.h"
#include "cmsis_os.h"
#include "gpio.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	
//#define ID  12
	

void Getaddress(void);
extern uint8_t address;
#ifdef __cplusplus
}
#endif

#endif /* __ID_H */
