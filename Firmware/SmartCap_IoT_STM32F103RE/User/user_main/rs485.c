/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : rs485.c
  * @brief          : RS485 communication module implementation
  ******************************************************************************
  * @attention
  *
  * RS485 communication implementation
  * - Direct UART register access for reliable communication
  * - Transparent transmission mode (no buffering)
  * - RE#/SHDN# control for MAX13487 transceiver
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "rs485.h"
#include "usart.h"
#include "gpio.h"
#include "stm32f1xx.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define RS485_RX_BUFFER_SIZE  256

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint8_t rs485_rx_buffer[RS485_RX_BUFFER_SIZE];
static volatile uint16_t rs485_rx_write_index = 0;
static volatile uint16_t rs485_rx_read_index = 0;
static uint8_t rs485_uart_rx_byte;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  设置RS485为接收模式
 */
void RS485_SetReceiveMode(void)
{
    /* MAX13487接收模式: RE#=0(接收使能), SHDN#=1(芯片工作) */
    HAL_GPIO_WritePin(RS485_RE_GPIO_Port, RS485_RE_Pin, GPIO_PIN_RESET);   // RE# = 0
    HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_SET);     // SHDN# = 1
}

/**
 * @brief  设置RS485为发送模式
 */
void RS485_SetTransmitMode(void)
{
    /* MAX13487发送模式: RE#=1(接收禁止), SHDN#=1(芯片工作) */
    HAL_GPIO_WritePin(RS485_RE_GPIO_Port, RS485_RE_Pin, GPIO_PIN_SET);     // RE# = 1
    HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_SET);     // SHDN# = 1
}

/**
 * @brief  初始化RS485通讯
 */
void RS485_Init(void)
{
    HAL_Delay(100);
    
    RS485_SetReceiveMode();
    HAL_Delay(10);
    
    /* 启动UART中断接收 */
    HAL_UART_Receive_IT(&huart1, &rs485_uart_rx_byte, 1);
}

/**
 * @brief  发送一个字节
 */
void RS485_SendByte(uint8_t data)
{
    /* 等待发送数据寄存器空 */
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = data;
    
    /* 等待发送完成 */
    while(!(USART1->SR & USART_SR_TC));
}

/**
 * @brief  发送字符串(自动切换收发模式)
 */
void RS485_SendString(const char *str)
{
    /* 切换到发送模式 */
    RS485_SetTransmitMode();
    HAL_Delay(1);
    
    while(*str)
    {
        RS485_SendByte(*str++);
    }
    
    /* 等待发送完成后切回接收模式 */
    HAL_Delay(1);
    RS485_SetReceiveMode();
}

/**
 * @brief  发送字符串(不切换方向,用于连续发送)
 */
void RS485_SendString_NoDirChange(const char *str)
{
    while(*str)
    {
        RS485_SendByte(*str++);
    }
}

/**
 * @brief  发送缓冲区
 */
void RS485_SendBuffer(uint8_t *buf, uint16_t len)
{
    /* 切换到发送模式 */
    RS485_SetTransmitMode();
    HAL_Delay(1);
    
    for(uint16_t i = 0; i < len; i++)
    {
        RS485_SendByte(buf[i]);
    }
    
    /* 等待发送完成后切回接收模式 */
    HAL_Delay(1);
    RS485_SetReceiveMode();
}

/**
 * @brief  接收一个字节(非阻塞)
 */
uint8_t RS485_ReceiveByte(uint8_t *data)
{
    if (rs485_rx_read_index != rs485_rx_write_index)
    {
        *data = rs485_rx_buffer[rs485_rx_read_index];
        rs485_rx_read_index = (rs485_rx_read_index + 1) % RS485_RX_BUFFER_SIZE;
        return 1;
    }
    return 0;
}

/**
 * @brief  USART1接收中断回调函数
 */
void RS485_UART_RxCallback(void)
{
    uint16_t next_write_index = (rs485_rx_write_index + 1) % RS485_RX_BUFFER_SIZE;
    
    if (next_write_index != rs485_rx_read_index)
    {
        rs485_rx_buffer[rs485_rx_write_index] = rs485_uart_rx_byte;
        rs485_rx_write_index = next_write_index;
    }
    
    HAL_UART_Receive_IT(&huart1, &rs485_uart_rx_byte, 1);
}
