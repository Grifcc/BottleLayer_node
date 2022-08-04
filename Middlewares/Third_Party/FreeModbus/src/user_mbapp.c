/*
 * modbus�������
 * File: $Id$
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "queue.h"
#include "port.h"

#include "string.h"
#include "id.h"

extern osMessageQueueId_t RFID_DATA_QueueHandle;  //��Ϣ����
/* ----------------------- Defines ------------------------------------------*/
#define REG_INPUT_START 1  
#define REG_INPUT_NREGS 9  //���ݼĴ�������

#define REG_HOLDING_START 1
#define REG_HOLDING_NREGS 8   //���ּĴ�������

/* ----------------------- Static variables ---------------------------------*/
static USHORT   usRegInputStart = REG_INPUT_START;
static USHORT   usRegInputBuf[REG_INPUT_NREGS];   //16λ

//static USHORT   usRegHoldingStart = REG_HOLDING_START;
static USHORT   usRegHoldingBuf[REG_HOLDING_NREGS];

uint8_t  sheepInfoSheet[10][19];  // ǰ18λΪ��Ϣ�����һλΪ��̬ע���־��1����ռ�� 0����ע��
uint8_t  sendPtr=0;    //����ָ��

uint8_t 	Inquired_flag=0;  //��ѯ��־λ

/* ----------------------- Start implementation -----------------------------*/
/* Definitions for ModbusSlaveTask */
osThreadId_t modbusTaskHandle;   //modbus������
//modbus�߳�
const osThreadAttr_t modbusTask_attributes = {
  .name = "ModbusSlaveTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 512 * 4
};
void modbus_task( void* args )
{
    eMBInit( MB_RTU, ID, 0, 9600, MB_PAR_NONE );  //modbusRTU 9600 ��У��

    /* Enable the Modbus Protocol Stack. */
    eMBEnable(  );
		BaseType_t xReturn=pdPASS;
		RFID_messageQ_t Message_rx;  //�������ݻ�����
	  /*�����ռ��ʼ��*/
		memset(&Message_rx,0,sizeof(Message_rx));
    for( ;; )
    {
			//�ж������Ƿ�ȡ��
			if(Inquired_flag==1)
			{
				memset((char *)usRegInputBuf,0,sizeof(usRegInputBuf)); //��ռĴ���
				sendPtr++; //����ָ���ۼ�
				Inquired_flag=0;
			}
			//��ָ֤����Ч
			if(sendPtr>=10)
			{
				sendPtr=0;
			}
			//���ܶ���
			xReturn = xQueueReceive(RFID_DATA_QueueHandle,&Message_rx,0);
			if(xReturn==pdPASS)
			{
				//��������������ᳬ��10
				for(int ptr=0;ptr<10;ptr++)
				{
					/* ��Ϣ��������Ϣ���ж�̬ע������*/
					if(sheepInfoSheet[ptr][18]==0)
					{
						strncpy((char *)sheepInfoSheet[ptr],(char *)(Message_rx.info),18);
						sheepInfoSheet[ptr][18]=1;
						break;
					}
				}
			}
			//��Ĵ�����ֵ
			if(sheepInfoSheet[sendPtr][18]==1)
			{
				strncpy((char *)usRegInputBuf,(char *)(sheepInfoSheet[sendPtr]),18);
				memset(sheepInfoSheet[sendPtr],0,19);
			}
			else
			{
				sendPtr++; //���ҵ��հ׵�����
			}
			//modbus��ѯ
			( void )eMBPoll();
    }
}

void
modbus_init(void){
  modbusTaskHandle = osThreadNew(modbus_task, NULL, &modbusTask_attributes);
}
/****************************************************************************
* ����:eMBRegHoldingCB 
* ����:��Ӧ������:04 ��д����Ĵ���  
****************************************************************************/
eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= REG_INPUT_START )
        && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegInputStart );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
		//����ѯ��־λ��ֵ
		Inquired_flag=1;
    return eStatus;
}

/****************************************************************************
* ����:eMBRegHoldingCB 
* ����:��Ӧ������:03 ��д���ּĴ���  
****************************************************************************/
eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                 eMBRegisterMode eMode )
{
  eMBErrorCode    eStatus = MB_ENOERR;
  int             iRegIndex;
  uint16_t getdata;

  if( ( usAddress >= REG_HOLDING_START) &&
      ( usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS ) )
  {
      iRegIndex = ( int )( usAddress - REG_HOLDING_START );
      switch ( eMode )
      {
          /* Pass current register values to the protocol stack. */
      case MB_REG_READ:
          while( usNRegs > 0 )
          {
              *pucRegBuffer++ = ( UCHAR ) ( usRegHoldingBuf[iRegIndex] >> 8 );
              *pucRegBuffer++ = ( UCHAR ) ( usRegHoldingBuf[iRegIndex] & 0xFF );
              iRegIndex++;
              usNRegs--;
          }
          break;

          /* Update current register values with new values from the
           * protocol stack. */
      case MB_REG_WRITE:
          while( usNRegs > 0 )
          {
              getdata = *pucRegBuffer++ << 8;
              getdata |= *pucRegBuffer++;
              usRegHoldingBuf[iRegIndex] = getdata;
              iRegIndex++;
              usNRegs--;
          }
      }
  }
  else
  {
      eStatus = MB_ENOREG;
  }
  return eStatus;
}

/************************************************
* ����:eMBRegCoilsCB 
* ����:��Ӧ������:01 ����Ȧ     
************************************************/
eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils,
               eMBRegisterMode eMode )
{
    return MB_ENOREG;
}

eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    return MB_ENOREG;
}
