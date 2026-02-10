/**
  ******************************************************************************
  * @file    user_tasks.h
  * @brief   FreeRTOS User Task Implementation Header
  * @date    2026-02-06
  ******************************************************************************
  * @description
  * 用户FreeRTOS任务函数实现
  * 这些函数会在 freertos.c 的 USER CODE 区域被调用
  * 
  * 架构说明:
  * - CubeMX生成的 freertos.c 中定义了5个 __weak 任务函数
  * - 本文件提供具体实现,在 freertos.c 的 USER CODE 区域调用
  * - 好处: CubeMX重新生成代码不会影响业务逻辑
  ******************************************************************************
  */

#ifndef __USER_TASKS_H__
#define __USER_TASKS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  默认任务实现
 * @param  argument: 任务参数(未使用)
 * @note   优先级: Low
 *         堆栈: 128 words
 *         功能: 调用 User_Process() 处理透传业务
 */
void UserTask_Default(void const * argument);

/**
 * @brief  RS485接收任务实现
 * @param  argument: 任务参数(未使用)
 * @note   优先级: AboveNormal
 *         堆栈: 512 words
 *         功能: 预留,当前未使用(透传逻辑在DefaultTask中)
 */
void UserTask_RS485_RxHandler(void const * argument);

/**
 * @brief  RG200U接收任务实现
 * @param  argument: 任务参数(未使用)
 * @note   优先级: AboveNormal
 *         堆栈: 512 words
 *         功能: 预留,当前未使用(透传逻辑在DefaultTask中)
 */
void UserTask_RG200U_RxHandler(void const * argument);

/**
 * @brief  RS485发送任务实现
 * @param  argument: 任务参数(未使用)
 * @note   优先级: Normal
 *         堆栈: 512 words
 *         功能: 预留,当前未使用(透传逻辑在DefaultTask中)
 */
void UserTask_RS485_TxHandler(void const * argument);

/**
 * @brief  RG200U发送任务实现
 * @param  argument: 任务参数(未使用)
 * @note   优先级: Normal
 *         堆栈: 512 words
 *         功能: 预留,当前未使用(透传逻辑在DefaultTask中)
 */
void UserTask_RG200U_TxHandler(void const * argument);

#ifdef __cplusplus
}
#endif

#endif /* __USER_TASKS_H__ */
