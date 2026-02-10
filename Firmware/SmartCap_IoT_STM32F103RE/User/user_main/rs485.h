/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : rs485.h
  * @brief          : RS485 communication module header file
  ******************************************************************************
  * @attention
  *
  * RS485 communication based on MAX13487 transceiver
  * - Uses USART1 (PB6/PB7) for data transmission
  * - Uses RE# (PB5) and SHDN# (PA15) for direction control
  * - Direct transparent transmission mode
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RS485_H
#define __RS485_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief  初始化RS485通讯
 * @note   配置方向控制引脚,设置为接收模式
 */
void RS485_Init(void);

/**
 * @brief  设置RS485为接收模式
 */
void RS485_SetReceiveMode(void);

/**
 * @brief  设置RS485为发送模式
 */
void RS485_SetTransmitMode(void);

/**
 * @brief  发送一个字节
 * @param  data: 要发送的字节
 * @note   自动切换到发送模式,发送完成后需手动切回接收模式
 */
void RS485_SendByte(uint8_t data);

/**
 * @brief  发送字符串
 * @param  str: 要发送的字符串(以\0结尾)
 */
void RS485_SendString(const char *str);

/**
 * @brief  发送字符串(不切换方向,用于连续发送)
 * @param  str: 要发送的字符串(以\0结尾)
 * @note   使用前需先手动调用RS485_SetTransmitMode()
 */
void RS485_SendString_NoDirChange(const char *str);

/**
 * @brief  发送缓冲区
 * @param  buf: 数据缓冲区指针
 * @param  len: 数据长度
 */
void RS485_SendBuffer(uint8_t *buf, uint16_t len);

/**
 * @brief  接收一个字节(非阻塞)
 * @param  data: 接收数据存储指针
 * @retval 1:收到数据  0:无数据
 */
uint8_t RS485_ReceiveByte(uint8_t *data);

/**
 * @brief  USART1接收中断回调函数
 */
void RS485_UART_RxCallback(void);

#ifdef __cplusplus
}
#endif

#endif /* __RS485_H */
