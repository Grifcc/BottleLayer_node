/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "queue.h"
#include "usart.h"
#include "ctype.h"
#include <stdio.h>
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
uint8_t Rx_Cplt_Flag=0;  // RFID接收完成标志位
uint8_t RFID_RESET_FLAG=1;  //RFID唤醒标志位

uint8_t RX_len;    //RFID接收数据长度（字节）
uint8_t Uart3_RX_Buffer[MAX_BUFFER_SIZE];  //RFID接收缓冲区
/* USER CODE END Variables */
/* Definitions for RFID_Reset_Task */
osThreadId_t RFID_Reset_TaskHandle;   //RFID维持心跳句柄

 //RFID维持心跳线程
const osThreadAttr_t RFID_Reset_Task_attributes = {
  .name = "RFID_Reset_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for RFID_RX */
osThreadId_t RFID_RXHandle;  //RFID接收数据句柄

 //RFID接收数据线程
const osThreadAttr_t RFID_RX_attributes = {
  .name = "RFID_RX",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for RFID_DATA_Queue */
osMessageQueueId_t RFID_DATA_QueueHandle;   //RFID数据传输队列句柄
//RFID数据传输队列
const osMessageQueueAttr_t RFID_DATA_Queue_attributes = {
  .name = "RFID_DATA_Queue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern void modbus_init(void);    //modbus初始化函数，详见Middlerwares/Freemodbus/uesr_app.c
uint8_t checksum(uint8_t *data);  //校验
float GetTemp(uint8_t *ns);   //读取温度
/* USER CODE END FunctionPrototypes */

void StartRFIDResetTask(void *argument);  //RFID心跳线程
void StartRFID_RX(void *argument);   //RFID接收数据线程

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of RFID_DATA_Queue */
  RFID_DATA_QueueHandle = osMessageQueueNew (4, sizeof(RFID_messageQ_t), &RFID_DATA_Queue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of RFID_Reset_Task */
  RFID_Reset_TaskHandle = osThreadNew(StartRFIDResetTask, NULL, &RFID_Reset_Task_attributes);

  /* creation of RFID_RX */
  RFID_RXHandle = osThreadNew(StartRFID_RX, NULL, &RFID_RX_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
	modbus_init();
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartRFIDResetTask */
/**
	* @brief  Function implementing the RFID_Reset_Task thread.
	* @param  argument: Not used
	* @retval None
	*/
/* USER CODE END Header_StartRFIDResetTask */
void StartRFIDResetTask(void *argument)
{
  /* USER CODE BEGIN StartRFIDResetTask */
	/* Infinite loop */
	for(;;)
	{
		/*由于RFID模块必须有方波心跳才会持续工作
			PB1为RFID Reset，PB9为心跳指示灯
			在上一包数据未处理完成前RFID_RESET_FLAG为0，不对RFID模块进行重置，此标志位相当于繁忙标志位
		  心跳频率1Hz
		*/
		if(RFID_RESET_FLAG)   
		{
				 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);
				 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_SET);
				 osDelay(500);
				 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
				 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_RESET);
				 osDelay(500);
		}
	}
  /* USER CODE END StartRFIDResetTask */
}

/* USER CODE BEGIN Header_StartRFID_RX */
/**
* @brief Function implementing the RFID_RX thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRFID_RX */
void StartRFID_RX(void *argument)
{
	/* USER CODE BEGIN StartRFID_RX */
	
	/* 接收思想为空闲中断+DMA，保证不定长数据的完整性*/
	__HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);  //开启空闲中断
	HAL_UART_Receive_DMA(&huart3, (uint8_t*)Uart3_RX_Buffer, MAX_BUFFER_SIZE);   //开启DMA接收
	
	uint8_t tmpstr[10];   //格式化温度字符串的缓存区
	uint8_t strTime[10];   //时长格式化缓冲区
  uint8_t  len=0;       //格式化字符串长度
	
	RFID_messageQ_t  Message;   //队列传送的数据结构，详见main.h   
	BaseType_t xReturn =pdPASS; 
	
	volatile  TickType_t DebounceTime=0;  //消抖时长
	volatile  TickType_t resumTime=0;     //电磁开关闭合时长
	volatile  TickType_t key_pretime=0;   //存储开关刚闭合的时间，用于消抖
	
	uint8_t Msg_flag=0;   //判断是否有标签数据读到的标志位
	uint8_t key_flag=0;   //判断电磁开关是否闭合的标志位，用于计算闭合时长
	uint8_t Trig_flag=0;  //电磁开关首次触发，用于消抖

	/*变量空间初始化*/
	memset(tmpstr,0,10);
	memset(strTime,0,10);
	memset(&Message,0,sizeof(Message));
	/* Infinite loop */
	for(;;)
	{
		
		/*电磁开关非阻塞式消抖*/
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==RESET  && key_flag==0 && Trig_flag==0)
		{
				DebounceTime=xTaskGetTickCount();
				Trig_flag=1;
		}
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==RESET  && key_flag==0 && Trig_flag==1)
		{	
				/*确认闭合*/
				if(GetTimeBetween(xTaskGetTickCount(),DebounceTime)>30)
				{
					key_pretime=xTaskGetTickCount();  //闭合开始计时
					key_flag=1;
					DebounceTime=0;
					Trig_flag=0;
				}
		}
		
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==SET  && key_flag==0)
		{
			/*上次触发属于抖动*/
			DebounceTime=0;
			Trig_flag=0;
		}
		/*消抖结束*/
		
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==SET  && key_flag==1)
		{
			/*电磁开关断开*/
			resumTime=GetTimeBetween(xTaskGetTickCount(),key_pretime);  /*计数结束*/
			key_flag=0;
		}
		
		/*接收数据*/
		if(Rx_Cplt_Flag==1)
		{
			/*有数据接收完成*/
			if(Uart3_RX_Buffer[0]==0x02 && Uart3_RX_Buffer[29]==0x03)  //校验帧头和帧尾
			{
					//校验成功
					RFID_RESET_FLAG=0; //RFID标志位置1，处理数据，暂停RFID的读取
					if(checksum(Uart3_RX_Buffer+1))   //加和校验数据正确性，详见RFID模块文档
					{
						len=sprintf((char *)tmpstr,"%.2f\r\n",GetTemp(Uart3_RX_Buffer+21));  //提取温度

						//转发14个字节的卡号，前10个字节为卡号，后4个字节为国家码,现无需国家码，低位在前高位在后
						/*   以下数据chuli方式
						原始数据：       9 8 7 6 5 4 3 2 1 0
						经modbus发送后 ：8 7 6 7 4 5 2 3 0 1     modbus为16位寄存器，且地位在前，高位在后
						现处理为 ：      1 0 3 2 5 4 7 6 9 8
						现发送到主机为： 0 1 2 3 4 5 6 7 8 9
						*/
						for(int k=0;k<5;k++)
						{
							Message.info[9-2*k]=Uart3_RX_Buffer[1+2*k+1];
							Message.info[9-2*k-1]=Uart3_RX_Buffer[1+2*k];
						}
						//转发温度
						for(int k=0;k<2;k++)
						{
							Message.info[10+2*k]=tmpstr[2*k+1];
							Message.info[10+2*k+1]=tmpstr[2*k];
						}
//					HAL_UART_Transmit(&huart1,Uart3_RX_Buffer+1,30,0x1000);
//					HAL_UART_Transmit(&huart1,(uint8_t *)" ",1,0x1000);
//					HAL_UART_Transmit(&huart1,tmpstr,len,0x1000);
						Msg_flag=1;  //有卡读到
					}	
					/*数据处理完成，各标志位归位，并重新开启DMA接收*/
			}
			RX_len=0;
			Rx_Cplt_Flag=0;
			RFID_RESET_FLAG=1;
			memset(Uart3_RX_Buffer,0,MAX_BUFFER_SIZE);
			HAL_UART_Receive_DMA(&huart3, (uint8_t*)Uart3_RX_Buffer, MAX_BUFFER_SIZE); 
			
		}
		
		/*电磁开关已经触发*/
		if(resumTime!=0)
		{
			if(Msg_flag==0)  /*判断是否有数据读到*/
			{
				//test info赋值
				for(int k=0;k<7;k++)
				{
						Message.info[2*k]=testIdAndtemp[2*k+1];
						Message.info[2*k+1]=testIdAndtemp[2*k];
				}
			}
			sprintf((char *)strTime,"%04d",resumTime/1000);
			for(int k=0;k<2;k++)
			{
				Message.info[14+2*k]=strTime[2*k+1];
				Message.info[14+2*k+1]=strTime[2*k];
			}
			/*消息队列*/
			xReturn=xQueueSend(RFID_DATA_QueueHandle,&Message,0);
			Msg_flag=0;
			resumTime=0;
		}
	}
	/* USER CODE END StartRFID_RX */
}


/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* 加和校验，详见RFID模块技术资料*/
uint8_t checksum(uint8_t *data)
{
	uint8_t sum=0;
	for(int i=0;i<26;i++)
	{
		sum^=*(data+i);
	}
	if(sum == *(data+27) && (~sum) == *(data+28))
	{
		return HAL_OK;
	}
	else
	{
		return  HAL_ERROR;
	}
}
/* 温度处理，逆向破解，如有使用直接复制*/
float GetTemp(uint8_t *ns)  
{  
		uint8_t s[2];
		s[0]=*(ns+1);
		s[1]=*(ns);
		uint8_t i;  
		uint16_t n = 0;  
		if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))  
		{  
				i = 2;  
		}  
		else  
		{  
				i = 0;  
		}  
		for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i)  
		{  
				if (tolower(s[i]) > '9')  
				{  
						n = 16 * n + (10 + tolower(s[i]) - 'a');  
				}  
				else  
				{  
						n = 16 * n + (tolower(s[i]) - '0');  
				}  
		}  
		return 0.111826 * n + 23.315956;
}  
/* USER CODE END Application */

