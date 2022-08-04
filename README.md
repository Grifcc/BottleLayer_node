#  奶山羊精细化管理底层节点程序


## 1.系统架构
+ 主控芯片STM32F103C8T6
+ 基于STM HAL库，使用cubeMx配置资源，CubeMx固件版本V1.8.4
+ MDK-ARM 5
+ 通信采用freeModbusV1.6从机协议栈+RS485总线(非自动流控)
+ 使用freertosV2.0

## 2.资源配置
+  外部8MHz晶振，主频72MHz
+  TIM3 作freemodbus定时器
+  TIM4 Timebase Source
+  SysTick 作freertos心跳
+  UART1 log输出，115200 8 N 1  无中断
+  UART2 RS485输出 ， 9600 8 N 1 全局收发硬中断
+  UART3 RFID接收输入，使能DMA，9600 8 N 1 全局收硬中断
+  PB9 OUTPUT 片上LED
+  PB1 OUTPUT RFID  Reset
+  PB2 INPUT  电磁开关输入
+  更多详见BottleLayer_node.ioc

## 3.软件设计
+ 1).  使用freertos多线程，采用三个线程，线程1 维持RFID心跳，线程2 处理RFID数据，线程3 freemodbus协议栈非阻塞运行。
+ 2). 由于主机轮询有时间间隔，采用动态注册信息，循环标发送。双指针处理，Send_ptr用于循环发送，ptr用于动态注册。

##  TODO

+ a. ID Flash固化
+ b. 上位机写指令执行
+ c. IAP升级
