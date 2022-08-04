
#include "port.h"
#include "tim.h"
#include "usart.h"




void modbus_serial_irq(void){
  if(__HAL_UART_GET_IT_SOURCE(&huart2, UART_IT_RXNE) != RESET){
    prvvUARTRxISR();
    __HAL_UART_CLEAR_FLAG(&huart2, UART_IT_RXNE);
  }
  if(__HAL_UART_GET_IT_SOURCE(&huart2, UART_IT_TXE) != RESET){
    prvvUARTTxReadyISR();
    __HAL_UART_CLEAR_FLAG(&huart2, UART_IT_TXE);
  }
}

void modbus_timer_irq(void){
  if(__HAL_TIM_GET_IT_SOURCE(&htim3, TIM_IT_UPDATE) != RESET){
    prvvTIMERExpiredISR();
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_IT_UPDATE);
  }
}
