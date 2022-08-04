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
uint8_t Rx_Cplt_Flag=0;  // RFID������ɱ�־λ
uint8_t RFID_RESET_FLAG=1;  //RFID���ѱ�־λ

uint8_t RX_len;    //RFID�������ݳ��ȣ��ֽڣ�
uint8_t Uart3_RX_Buffer[MAX_BUFFER_SIZE];  //RFID���ջ�����
/* USER CODE END Variables */
/* Definitions for RFID_Reset_Task */
osThreadId_t RFID_Reset_TaskHandle;   //RFIDά���������

 //RFIDά�������߳�
const osThreadAttr_t RFID_Reset_Task_attributes = {
  .name = "RFID_Reset_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for RFID_RX */
osThreadId_t RFID_RXHandle;  //RFID�������ݾ��

 //RFID���������߳�
const osThreadAttr_t RFID_RX_attributes = {
  .name = "RFID_RX",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for RFID_DATA_Queue */
osMessageQueueId_t RFID_DATA_QueueHandle;   //RFID���ݴ�����о��
//RFID���ݴ������
const osMessageQueueAttr_t RFID_DATA_Queue_attributes = {
  .name = "RFID_DATA_Queue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern void modbus_init(void);    //modbus��ʼ�����������Middlerwares/Freemodbus/uesr_app.c
uint8_t checksum(uint8_t *data);  //У��
float GetTemp(uint8_t *ns);   //��ȡ�¶�
/* USER CODE END FunctionPrototypes */

void StartRFIDResetTask(void *argument);  //RFID�����߳�
void StartRFID_RX(void *argument);   //RFID���������߳�

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
		/*����RFIDģ������з��������Ż��������
			PB1ΪRFID Reset��PB9Ϊ����ָʾ��
			����һ������δ�������ǰRFID_RESET_FLAGΪ0������RFIDģ��������ã��˱�־λ�൱�ڷ�æ��־λ
		  ����Ƶ��1Hz
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
	
	/* ����˼��Ϊ�����ж�+DMA����֤���������ݵ�������*/
	__HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);  //���������ж�
	HAL_UART_Receive_DMA(&huart3, (uint8_t*)Uart3_RX_Buffer, MAX_BUFFER_SIZE);   //����DMA����
	
	uint8_t tmpstr[10];   //��ʽ���¶��ַ����Ļ�����
	uint8_t strTime[10];   //ʱ����ʽ��������
  uint8_t  len=0;       //��ʽ���ַ�������
	
	RFID_messageQ_t  Message;   //���д��͵����ݽṹ�����main.h   
	BaseType_t xReturn =pdPASS; 
	
	volatile  TickType_t DebounceTime=0;  //����ʱ��
	volatile  TickType_t resumTime=0;     //��ſ��رպ�ʱ��
	volatile  TickType_t key_pretime=0;   //�洢���ظձպϵ�ʱ�䣬��������
	
	uint8_t Msg_flag=0;   //�ж��Ƿ��б�ǩ���ݶ����ı�־λ
	uint8_t key_flag=0;   //�жϵ�ſ����Ƿ�պϵı�־λ�����ڼ���պ�ʱ��
	uint8_t Trig_flag=0;  //��ſ����״δ�������������

	/*�����ռ��ʼ��*/
	memset(tmpstr,0,10);
	memset(strTime,0,10);
	memset(&Message,0,sizeof(Message));
	/* Infinite loop */
	for(;;)
	{
		
		/*��ſ��ط�����ʽ����*/
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==RESET  && key_flag==0 && Trig_flag==0)
		{
				DebounceTime=xTaskGetTickCount();
				Trig_flag=1;
		}
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==RESET  && key_flag==0 && Trig_flag==1)
		{	
				/*ȷ�ϱպ�*/
				if(GetTimeBetween(xTaskGetTickCount(),DebounceTime)>30)
				{
					key_pretime=xTaskGetTickCount();  //�պϿ�ʼ��ʱ
					key_flag=1;
					DebounceTime=0;
					Trig_flag=0;
				}
		}
		
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==SET  && key_flag==0)
		{
			/*�ϴδ������ڶ���*/
			DebounceTime=0;
			Trig_flag=0;
		}
		/*��������*/
		
		if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==SET  && key_flag==1)
		{
			/*��ſ��ضϿ�*/
			resumTime=GetTimeBetween(xTaskGetTickCount(),key_pretime);  /*��������*/
			key_flag=0;
		}
		
		/*��������*/
		if(Rx_Cplt_Flag==1)
		{
			/*�����ݽ������*/
			if(Uart3_RX_Buffer[0]==0x02 && Uart3_RX_Buffer[29]==0x03)  //У��֡ͷ��֡β
			{
					//У��ɹ�
					RFID_RESET_FLAG=0; //RFID��־λ��1���������ݣ���ͣRFID�Ķ�ȡ
					if(checksum(Uart3_RX_Buffer+1))   //�Ӻ�У��������ȷ�ԣ����RFIDģ���ĵ�
					{
						len=sprintf((char *)tmpstr,"%.2f\r\n",GetTemp(Uart3_RX_Buffer+21));  //��ȡ�¶�

						//ת��14���ֽڵĿ��ţ�ǰ10���ֽ�Ϊ���ţ���4���ֽ�Ϊ������,����������룬��λ��ǰ��λ�ں�
						/*   ��������chuli��ʽ
						ԭʼ���ݣ�       9 8 7 6 5 4 3 2 1 0
						��modbus���ͺ� ��8 7 6 7 4 5 2 3 0 1     modbusΪ16λ�Ĵ������ҵ�λ��ǰ����λ�ں�
						�ִ���Ϊ ��      1 0 3 2 5 4 7 6 9 8
						�ַ��͵�����Ϊ�� 0 1 2 3 4 5 6 7 8 9
						*/
						for(int k=0;k<5;k++)
						{
							Message.info[9-2*k]=Uart3_RX_Buffer[1+2*k+1];
							Message.info[9-2*k-1]=Uart3_RX_Buffer[1+2*k];
						}
						//ת���¶�
						for(int k=0;k<2;k++)
						{
							Message.info[10+2*k]=tmpstr[2*k+1];
							Message.info[10+2*k+1]=tmpstr[2*k];
						}
//					HAL_UART_Transmit(&huart1,Uart3_RX_Buffer+1,30,0x1000);
//					HAL_UART_Transmit(&huart1,(uint8_t *)" ",1,0x1000);
//					HAL_UART_Transmit(&huart1,tmpstr,len,0x1000);
						Msg_flag=1;  //�п�����
					}	
					/*���ݴ�����ɣ�����־λ��λ�������¿���DMA����*/
			}
			RX_len=0;
			Rx_Cplt_Flag=0;
			RFID_RESET_FLAG=1;
			memset(Uart3_RX_Buffer,0,MAX_BUFFER_SIZE);
			HAL_UART_Receive_DMA(&huart3, (uint8_t*)Uart3_RX_Buffer, MAX_BUFFER_SIZE); 
			
		}
		
		/*��ſ����Ѿ�����*/
		if(resumTime!=0)
		{
			if(Msg_flag==0)  /*�ж��Ƿ������ݶ���*/
			{
				//test info��ֵ
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
			/*��Ϣ����*/
			xReturn=xQueueSend(RFID_DATA_QueueHandle,&Message,0);
			Msg_flag=0;
			resumTime=0;
		}
	}
	/* USER CODE END StartRFID_RX */
}


/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* �Ӻ�У�飬���RFIDģ�鼼������*/
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
/* �¶ȴ��������ƽ⣬����ʹ��ֱ�Ӹ���*/
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

