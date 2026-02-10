/**
  ******************************************************************************
  * @file    rg200u.h
  * @brief   RG200U 4G Module Driver Header
  ******************************************************************************
  */

#ifndef __RG200U_H__
#define __RG200U_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported defines ----------------------------------------------------------*/
#define RG200U_RX_BUFFER_SIZE   256

/* 调试开关 - 设置为1启用调试信息,设置为0禁用所有调试信息 */
#define RG200U_DEBUG_ENABLE     0    /* 1=显示调试信息, 0=隐藏调试信息 */

/* TCP服务器配置 */
#define TCP_SERVER_IP    "2401:ce00:c5af:75d0::f8a"              /* 服务器IPv4地址 */
#define TCP_SERVER_PORT  "18655"                     /* 服务器端口 */
#define TCP_SOCKET_ID    0                           /* Socket ID */

/* Exported types ------------------------------------------------------------*/
typedef enum {
    TCP_STATE_DISCONNECTED = 0,
    TCP_STATE_CONNECTING,
    TCP_STATE_CONNECTED,
    TCP_STATE_ERROR
} TCP_State_t;

/* Exported functions --------------------------------------------------------*/

void RG200U_Init(void);
void RG200U_SendByte(uint8_t data);
void RG200U_SendString(const char *str);
void RG200U_SendBuffer(const uint8_t *buf, uint16_t len);
bool RG200U_ReceiveByte(uint8_t *data);
void RG200U_UART_RxCallback(void);

/* TCP服务器连接相关函数 */
uint8_t RG200U_ConnectTCPServer(void);
TCP_State_t RG200U_GetTCPState(void);
uint8_t RG200U_ReadTCPData(char *buffer, uint16_t max_len);
void RG200U_ProcessTCPMessage(void);


#ifdef __cplusplus
}
#endif

#endif /* __RG200U_H__ */
