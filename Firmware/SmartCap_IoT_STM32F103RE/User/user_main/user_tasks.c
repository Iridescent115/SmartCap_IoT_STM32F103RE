/**
  ******************************************************************************
  * @file    user_tasks.c
  * @brief   FreeRTOS User Task Implementation
  * @date    2026-02-06
  ******************************************************************************
  * @description
  * 用户FreeRTOS任务函数实现 - 队列方案
  * 
  * 架构设计:
  * - RS485_RxTask: 从RS485接收 -> 写入Queue_RS485_To_RG200U
  * - RG200U_TxTask: 从Queue_RS485_To_RG200U读取 -> 发送到RG200U
  * - RG200U_RxTask: 从RG200U接收 -> 写入Queue_RG200U_To_RS485
  * - RS485_TxTask: 从Queue_RG200U_To_RS485读取 -> 发送到RS485
  * 
  * 优点:
  * - 接收任务高优先级,不丢数据
  * - 发送任务普通优先级,队列缓冲
  * - 任务之间通过队列解耦
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "user_tasks.h"
#include "User_main.h"
#include "rs485.h"
#include "rg200u.h"

/* Private variables ---------------------------------------------------------*/
/* 队列句柄(在freertos.c中定义,这里声明为外部变量) */
extern osMessageQId Queue_RS485_To_RG200UHandle;
extern osMessageQId Queue_RG200U_To_RS485Handle;

/* RS485发送状态管理 */
static uint8_t rs485_tx_active = 0;      // RS485发送激活标志
static uint32_t rs485_last_tx_tick = 0;  // 最后一次发送时间

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  默认任务实现 - 空闲任务
 * @param  argument: 任务参数(未使用)
 * @note   优先级最低,只用于LED指示或监控
 *         主要业务逻辑已经拆分到4个专用任务中
 */
void UserTask_Default(void const * argument)
{
    /* 无限循环 */
    for(;;)
    {
        /* LED闪烁或系统监控任务
         * 例如: 监控堆栈使用率、任务运行状态等
         */
        
        // 预留:添加LED指示、看门狗喂狗等
        
        osDelay(500);  // 500ms周期,节省CPU资源
    }
}

/**
 * @brief  RS485接收任务实现
 * @param  argument: 任务参数(未使用)
 * @note   优先级: AboveNormal (高优先级)
 *         功能: 从RS485接收数据,写入队列Queue_RS485_To_RG200U
 *         特点: 高优先级保证不丢数据
 */
void UserTask_RS485_RxHandler(void const * argument)
{
    uint8_t recv_byte;
    osStatus status;
    
    /* 无限循环 */
    for(;;)
    {
        /* 从RS485接收一个字节(非阻塞) */
        if (RS485_ReceiveByte(&recv_byte))
        {
            /* 将数据写入队列,发送给RG200U */
            status = osMessagePut(Queue_RS485_To_RG200UHandle, (uint32_t)recv_byte, 10);
            
            if (status != osOK)
            {
                /* 队列满,数据丢失 */
            }
        }
        
        /* 短延时,避免CPU占用过高 */
        osDelay(1);
    }
}

/**
 * @brief  RG200U接收任务实现
 * @param  argument: 任务参数(未使用)
 * @note   优先级: AboveNormal (高优先级)
 *         功能: 从RG200U接收数据,写入队列Queue_RG200U_To_RS485
 *         同时处理TCP服务器消息
 */
void UserTask_RG200U_RxHandler(void const * argument)
{
    uint8_t recv_byte;
    osStatus status;
    
    /* 无限循环 */
    for(;;)
    {
        /* 处理TCP服务器消息（检测+QIURC通知） */
        RG200U_ProcessTCPMessage();
        
        /* 从RG200U接收一个字节(非阻塞) */
        if (RG200U_ReceiveByte(&recv_byte))
        {
            /* 将数据写入队列,发送给RS485 */
            status = osMessagePut(Queue_RG200U_To_RS485Handle, (uint32_t)recv_byte, 10);
            
            if (status != osOK)
            {
                /* 队列满,数据丢失 */
            }
        }
        
        /* 短延时,避免CPU占用过高 */
        osDelay(1);
    }
}

/**
 * @brief  RS485发送任务实现
 * @param  argument: 任务参数(未使用)
 * @note   优先级: Normal (普通优先级)
 *         功能: 从Queue_RG200U_To_RS485队列读取数据,发送到RS485
 *         特点: 自动管理RS485收发方向切换
 */
void UserTask_RS485_TxHandler(void const * argument)
{
    osEvent event;
    uint8_t tx_byte;
    
    /* 无限循环 */
    for(;;)
    {
        /* 从队列读取数据 (阻塞等待,超时10ms) */
        event = osMessageGet(Queue_RG200U_To_RS485Handle, 10);
        
        if (event.status == osEventMessage)
        {
            /* 获取数据 */
            tx_byte = (uint8_t)event.value.v;
            
            /* 如果是第一个字节,切换到发送模式 */
            if (!rs485_tx_active)
            {
                RS485_SetTransmitMode();
                osDelay(1);  // 等待收发器切换
                rs485_tx_active = 1;
            }
            
            /* 发送数据 */
            RS485_SendByte(tx_byte);
            
            /* 更新最后发送时间 */
            rs485_last_tx_tick = osKernelSysTick();
        }
        else
        {
            /* 队列无数据
             * 检查是否需要切换回接收模式
             */
            if (rs485_tx_active)
            {
                /* 如果超过10ms没有新数据,切换回接收模式 */
                if ((osKernelSysTick() - rs485_last_tx_tick) > 10)
                {
                    osDelay(2);  // 等待最后的数据发送完成
                    RS485_SetReceiveMode();
                    rs485_tx_active = 0;
                }
            }
        }
    }
}

/**
 * @brief  RG200U发送任务实现
 * @param  argument: 任务参数(未使用)
 * @note   优先级: Normal (普通优先级)
 *         功能: 从Queue_RS485_To_RG200U队列读取数据,发送到RG200U
 *         特点: 简单的队列到串口转发
 */
void UserTask_RG200U_TxHandler(void const * argument)
{
    osEvent event;
    uint8_t tx_byte;
    
    /* 无限循环 */
    for(;;)
    {
        /* 从队列读取数据 (阻塞等待,超时100ms) */
        event = osMessageGet(Queue_RS485_To_RG200UHandle, 100);
        
        if (event.status == osEventMessage)
        {
            /* 获取数据 */
            tx_byte = (uint8_t)event.value.v;
            
            /* 发送到RG200U */
            RG200U_SendByte(tx_byte);
        }
        /* 如果超时,继续等待下一个数据 */
    }
}
