/*
 * modbus处理程序
 * File: $Id$
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "queue.h"
#include "port.h"

#include "string.h"
#include "id.h"

extern osMessageQueueId_t RFID_DATA_QueueHandle;  //消息队列
/* ----------------------- Defines ------------------------------------------*/
#define REG_INPUT_START 1  
#define REG_INPUT_NREGS 9  //数据寄存器长度

#define REG_HOLDING_START 1
#define REG_HOLDING_NREGS 8   //保持寄存器长度

/* ----------------------- Static variables ---------------------------------*/
static USHORT   usRegInputStart = REG_INPUT_START;
static USHORT   usRegInputBuf[REG_INPUT_NREGS];   //16位

//static USHORT   usRegHoldingStart = REG_HOLDING_START;
static USHORT   usRegHoldingBuf[REG_HOLDING_NREGS];

uint8_t  sheepInfoSheet[10][19];  // 前18位为信息，最后一位为动态注册标志，1：已占用 0：可注册
uint8_t  sendPtr=0;    //发送指针

uint8_t 	Inquired_flag=0;  //查询标志位

/* ----------------------- Start implementation -----------------------------*/
/* Definitions for ModbusSlaveTask */
osThreadId_t modbusTaskHandle;   //modbus处理句柄
//modbus线程
const osThreadAttr_t modbusTask_attributes = {
  .name = "ModbusSlaveTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 512 * 4
};
void modbus_task( void* args )
{
    eMBInit( MB_RTU, ID, 0, 9600, MB_PAR_NONE );  //modbusRTU 9600 无校验

    /* Enable the Modbus Protocol Stack. */
    eMBEnable(  );
		BaseType_t xReturn=pdPASS;
		RFID_messageQ_t Message_rx;  //接收数据缓存区
	  /*变量空间初始化*/
		memset(&Message_rx,0,sizeof(Message_rx));
    for( ;; )
    {
			//判断数据是否被取走
			if(Inquired_flag==1)
			{
				memset((char *)usRegInputBuf,0,sizeof(usRegInputBuf)); //清空寄存器
				sendPtr++; //发送指针累加
				Inquired_flag=0;
			}
			//保证指针有效
			if(sendPtr>=10)
			{
				sendPtr=0;
			}
			//接受队列
			xReturn = xQueueReceive(RFID_DATA_QueueHandle,&Message_rx,0);
			if(xReturn==pdPASS)
			{
				//假设队列数量不会超过10
				for(int ptr=0;ptr<10;ptr++)
				{
					/* 消息队列向信息标中动态注册数据*/
					if(sheepInfoSheet[ptr][18]==0)
					{
						strncpy((char *)sheepInfoSheet[ptr],(char *)(Message_rx.info),18);
						sheepInfoSheet[ptr][18]=1;
						break;
					}
				}
			}
			//向寄存器赋值
			if(sheepInfoSheet[sendPtr][18]==1)
			{
				strncpy((char *)usRegInputBuf,(char *)(sheepInfoSheet[sendPtr]),18);
				memset(sheepInfoSheet[sendPtr],0,19);
			}
			else
			{
				sendPtr++; //查找到空白的区域
			}
			//modbus轮询
			( void )eMBPoll();
    }
}

void
modbus_init(void){
  modbusTaskHandle = osThreadNew(modbus_task, NULL, &modbusTask_attributes);
}
/****************************************************************************
* 名称:eMBRegHoldingCB 
* 功能:对应功能码:04 读写输入寄存器  
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
		//被查询标志位赋值
		Inquired_flag=1;
    return eStatus;
}

/****************************************************************************
* 名称:eMBRegHoldingCB 
* 功能:对应功能码:03 读写保持寄存器  
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
* 名称:eMBRegCoilsCB 
* 功能:对应功能码:01 读线圈     
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
