/**
  ******************************************************************************
  * @file    rg200u.c
  * @brief   RG200U 4G Module Driver Implementation
  * @date    2026-02-05
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "rg200u.h"
#include "rs485.h"
#include "usart.h"
#include "main.h"    /* 包含继电器GPIO定义 */
#include <string.h>
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define AT_RESPONSE_TIMEOUT    5000   /* AT指令响应超时(ms) */
#define AT_RESPONSE_BUF_SIZE   512    /* AT响应缓冲区大小 */

/* DEBUG宏定义 - 根据RG200U_DEBUG_ENABLE控制调试信息输出 */
#if RG200U_DEBUG_ENABLE
    #define DEBUG_PRINT(msg) \
        do { \
            RS485_SetTransmitMode(); \
            HAL_Delay(1); \
            RS485_SendString_NoDirChange(msg); \
            RS485_SetReceiveMode(); \
            HAL_Delay(10); \
        } while(0)
#else
    #define DEBUG_PRINT(msg)  /* 调试关闭时为空 */
#endif

/* Private variables ---------------------------------------------------------*/
static uint8_t rg200u_rx_buffer[RG200U_RX_BUFFER_SIZE];  /* 接收环形缓冲区 */
static volatile uint16_t rx_write_index = 0;             /* 写指针 */
static volatile uint16_t rx_read_index = 0;              /* 读指针 */

uint8_t uart_rx_byte;  /* UART单字节接收缓冲(全局变量,供中断和外部使用) */

/* TCP连接状态 */
static TCP_State_t tcp_state = TCP_STATE_DISCONNECTED;

/* Private function prototypes -----------------------------------------------*/
static uint8_t RG200U_SendATCommand(const char *cmd, char *response, uint16_t timeout);
static void RG200U_ExtractString(const char *src, const char *start_tag, const char *end_tag, char *dest, uint16_t max_len);
static void RG200U_ProcessCommand(const char *cmd_data, uint16_t len);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  发送AT指令并等待响应
 * @param  cmd: AT指令字符串
 * @param  response: 响应缓冲区
 * @param  timeout: 超时时间(ms)
 * @retval 1:成功 0:失败
 */
static uint8_t RG200U_SendATCommand(const char *cmd, char *response, uint16_t timeout)
{
    uint16_t index = 0;
    uint32_t start_tick = HAL_GetTick();
    
    /* 清空响应缓冲 */
    memset(response, 0, AT_RESPONSE_BUF_SIZE);
    
    /* 清空接收缓冲区 */
    rx_read_index = rx_write_index;
    
    /* 发送AT指令 */
    HAL_UART_Transmit(&huart5, (uint8_t *)cmd, strlen(cmd), 1000);
    
    /* 等待响应 */
    while ((HAL_GetTick() - start_tick) < timeout)
    {
        uint8_t data;
        if (RG200U_ReceiveByte(&data))
        {
            if (index < (AT_RESPONSE_BUF_SIZE - 1))
            {
                response[index++] = data;
                response[index] = '\0';
                
                /* 检查是否收到完整响应 */
                if (strstr(response, "OK\r\n") || strstr(response, "ERROR\r\n"))
                {
                    return 1;
                }
            }
        }
        HAL_Delay(1);
    }
    
    return 0;  /* 超时 */
}

/**
 * @brief  等待特定响应
 * @param  expected: 期待的字符串
 * @param  response: 响应缓冲区
 * @param  max_len: 最大长度
 * @param  timeout_ms: 超时时间(毫秒)
 * @retval 1:收到 0:超时
 */
static uint8_t RG200U_WaitForResponse(const char *expected, char *response, uint16_t max_len, uint32_t timeout_ms)
{
    uint32_t start_time = HAL_GetTick();
    uint16_t index = 0;
    uint8_t data;
    uint8_t found = 0;
    uint32_t last_rx_time = 0;
    
    memset(response, 0, max_len);
    
    while ((HAL_GetTick() - start_time) < timeout_ms)
    {
        if (RG200U_ReceiveByte(&data))
        {
            if (index < (max_len - 1))
            {
                response[index++] = data;
                response[index] = '\0';
                last_rx_time = HAL_GetTick();
                
                /* 检查是否收到期待的字符串 */
                if (strstr(response, expected))
                {
                    found = 1;
                }
            }
        }
        
        /* 如果已经找到expected字符串，并且100ms内没有新数据，认为接收完成 */
        if (found && ((HAL_GetTick() - last_rx_time) > 100))
        {
            return 1;
        }
        
        HAL_Delay(1);
    }
    
    return 0;
}

/**
 * @brief  从字符串中提取内容
 * @param  src: 源字符串
 * @param  start_tag: 起始标记
 * @param  end_tag: 结束标记
 * @param  dest: 目标缓冲区
 * @param  max_len: 最大长度
 */
static void RG200U_ExtractString(const char *src, const char *start_tag, const char *end_tag, char *dest, uint16_t max_len)
{
    const char *start = strstr(src, start_tag);
    if (start)
    {
        start += strlen(start_tag);
        const char *end = strstr(start, end_tag);
        if (end)
        {
            uint16_t len = end - start;
            if (len > max_len - 1) len = max_len - 1;
            strncpy(dest, start, len);
            dest[len] = '\0';
        }
    }
}

/**
 * @brief  RG200U模块初始化及自检
 */
void RG200U_Init(void)
{
    char response[AT_RESPONSE_BUF_SIZE];
    char operator_name[64] = "Unknown";
    char ipv4[32] = "0.0.0.0";
    char ipv6[64] = "::";
    uint8_t test_ok = 0;
    uint8_t retry;
    
    /* 清空接收缓冲区 */
    memset((void *)rg200u_rx_buffer, 0, RG200U_RX_BUFFER_SIZE);
    rx_write_index = 0;
    rx_read_index = 0;
    
    /* ========== 关键修复: 先禁用UART5,等待RG200U启动 ========== */
    /* 禁用UART5,避免干扰RG200U启动 */
    HAL_UART_DeInit(&huart5);
    
    /* 切换到RS485发送模式,显示等待信息 */
    RS485_SetTransmitMode();
    HAL_Delay(2);
    
    RS485_SendString_NoDirChange("\r\n");
    RS485_SendString_NoDirChange("==================================\r\n");
    RS485_SendString_NoDirChange("  RG200U 4G Gateway Starting...\r\n");
    RS485_SendString_NoDirChange("==================================\r\n");
    RS485_SendString_NoDirChange("Hardware boot: [");
    
    /* 15秒进度条,每秒显示一个进度块 */
    for (uint8_t i = 0; i < 15; i++)
    {
        HAL_Delay(1000);
        RS485_SendString_NoDirChange("=");
    }
    
    RS485_SendString_NoDirChange("] Done\r\n\r\n");
    
    /* 切回RS485接收模式 */
    RS485_SetReceiveMode();
    HAL_Delay(2);
    
    /* ========== 重新初始化UART5 ========== */
    /* RG200U已完全启动,现在重新初始化UART5 */
    MX_UART5_Init();
    HAL_Delay(100);  /* 等待UART稳定 */
    
    /* 清空UART5接收缓冲 */
    __HAL_UART_FLUSH_DRREGISTER(&huart5);
    
    /* 启动UART5接收中断 */
    HAL_UART_Receive_IT(&huart5, &uart_rx_byte, 1);
    
    /* 切换到RS485发送模式 */
    RS485_SetTransmitMode();
    HAL_Delay(2);
    
    RS485_SendString_NoDirChange("=== RG200U 4G Module Self-Test ===\r\n\r\n");
    
    /* 步骤1: 测试AT指令 */
    RS485_SendString_NoDirChange("[1/5] Testing AT command...");
    for (retry = 0; retry < 3; retry++)
    {
        if (RG200U_SendATCommand("AT\r\n", response, 2000))
        {
            if (strstr(response, "OK"))
            {
                test_ok = 1;
                RS485_SendString_NoDirChange(" OK\r\n");
                break;
            }
        }
        HAL_Delay(500);
    }
    if (!test_ok)
    {
        RS485_SendString_NoDirChange(" FAILED\r\n");
        RS485_SendString_NoDirChange("\r\nError: RG200U not responding!\r\n");
        goto show_result;
    }
    
    /* 步骤2: 检查网络注册 (优先5G,兼容4G) */
    RS485_SendString_NoDirChange("[2/5] Checking network registration...");
    test_ok = 0;
    for (retry = 0; retry < 20; retry++)  /* 最多等待40秒 */
    {
        /* 先尝试5G注册状态 */
        if (RG200U_SendATCommand("AT+C5GREG?\r\n", response, 2000))
        {
            /* +C5GREG: 0,1 表示已注册5G本地网络 */
            /* +C5GREG: 0,5 表示已注册5G漫游网络 */
            if (strstr(response, "+C5GREG: 0,1") || strstr(response, "+C5GREG: 0,5"))
            {
                test_ok = 1;
                RS485_SendString_NoDirChange(" Registered (5G)\r\n");
                break;
            }
        }
        
        /* 如果5G未注册,再尝试4G */
        if (!test_ok && RG200U_SendATCommand("AT+CEREG?\r\n", response, 2000))
        {
            /* +CEREG: 0,1 表示已注册4G本地网络 */
            /* +CEREG: 0,5 表示已注册4G漫游网络 */
            if (strstr(response, "+CEREG: 0,1") || strstr(response, "+CEREG: 0,5"))
            {
                test_ok = 1;
                RS485_SendString_NoDirChange(" Registered (4G)\r\n");
                break;
            }
        }
        
        RS485_SendString_NoDirChange(".");
        HAL_Delay(2000);
    }
    if (!test_ok)
    {
        RS485_SendString_NoDirChange(" FAILED (not registered)\r\n");
    }
    
    /* 步骤3: 查询运营商信息 */
    RS485_SendString_NoDirChange("[3/5] Querying operator...");
    if (RG200U_SendATCommand("AT+COPS?\r\n", response, 3000))
    {
        /* 提取运营商名称: +COPS: 0,0,"CHN-UNICOM",13 */
        RG200U_ExtractString(response, "\"", "\"", operator_name, sizeof(operator_name));
        
        /* 显示运营商 */
        RS485_SendString_NoDirChange(" ");
        RS485_SendString_NoDirChange(operator_name);
        RS485_SendString_NoDirChange("\r\n");
    }
    else
    {
        RS485_SendString_NoDirChange(" Timeout\r\n");
    }
    
    /* 步骤4: 执行拨号上网 */
    RS485_SendString_NoDirChange("[4/5] Activating data connection...");
    if (RG200U_SendATCommand("AT+QNETDEVCTL=1,1,1\r\n", response, 15000))  /* 拨号可能需要较长时间 */
    {
        if (strstr(response, "OK"))
        {
            RS485_SendString_NoDirChange(" OK\r\n");
        }
        else if (strstr(response, "ERROR"))
        {
            /* 可能已经激活,继续 */
            RS485_SendString_NoDirChange(" Already active\r\n");
        }
    }
    else
    {
        RS485_SendString_NoDirChange(" Timeout\r\n");
    }
    
    /* 步骤5: 查询IP地址 */
    RS485_SendString_NoDirChange("[5/5] Querying IP address...");
    
    /* 等待IP地址分配完成(静默延时10秒) */
    for (uint8_t i = 0; i < 10; i++)
    {
        HAL_Delay(1000);
    }
    
    if (RG200U_SendATCommand("AT+CGPADDR\r\n", response, 3000))
    {
        /* 提取IPv4和IPv6: +CGPADDR: 1,"100.76.245.80","2408:8440:..." */
        const char *ip_start = strstr(response, "+CGPADDR:");
        if (ip_start)
        {
            /* 查找第一个引号(IPv4开始) */
            const char *ipv4_start = strchr(ip_start, '"');
            if (ipv4_start)
            {
                ipv4_start++;  /* 跳过引号 */
                
                /* 提取IPv4地址(到下一个引号) */
                int i = 0;
                while (*ipv4_start && *ipv4_start != '"' && i < 31)
                {
                    ipv4[i++] = *ipv4_start++;
                }
                ipv4[i] = '\0';
                
                /* 查找第二个引号对(IPv6) */
                if (*ipv4_start == '"')
                {
                    /* 跳过IPv4的结束引号,查找逗号后的下一个引号 */
                    const char *ipv6_start = strchr(ipv4_start + 1, '"');
                    if (ipv6_start)
                    {
                        ipv6_start++;  /* 跳过引号 */
                        
                        /* 提取IPv6地址(到下一个引号) */
                        i = 0;
                        while (*ipv6_start && *ipv6_start != '"' && i < 63)
                        {
                            ipv6[i++] = *ipv6_start++;
                        }
                        ipv6[i] = '\0';
                    }
                }
            }
        }
        
        RS485_SendString_NoDirChange(" OK\r\n");
    }
    else
    {
        RS485_SendString_NoDirChange(" Timeout\r\n");
    }
    
show_result:
    /* 显示欢迎信息 */
    RS485_SendString_NoDirChange("\r\n");
    RS485_SendString_NoDirChange("==================================\r\n");
    RS485_SendString_NoDirChange("  RG200U 4G Gateway Ready\r\n");
    RS485_SendString_NoDirChange("==================================\r\n");
    RS485_SendString_NoDirChange("Operator : ");
    RS485_SendString_NoDirChange(operator_name);
    RS485_SendString_NoDirChange("\r\n");
    RS485_SendString_NoDirChange("IPv4     : ");
    RS485_SendString_NoDirChange(ipv4);
    RS485_SendString_NoDirChange("\r\n");
    RS485_SendString_NoDirChange("IPv6     : ");
    RS485_SendString_NoDirChange(ipv6);
    RS485_SendString_NoDirChange("\r\n");
    RS485_SendString_NoDirChange("==================================\r\n");
    RS485_SendString_NoDirChange("Transparent mode enabled.\r\n\r\n");
    
    /* 等待发送完成 */
    HAL_Delay(10);
    
    /* ========== 关键: 清空接收缓冲区,避免残留AT响应被转发 ========== */
    rx_read_index = rx_write_index;  /* 丢弃所有缓冲数据 */
    
    /* ========== 连接TCP服务器 ========== */
    RS485_SendString_NoDirChange("\r\n");
    RS485_SendString_NoDirChange("[TCP] Connecting to server...\r\n");
    
    if (RG200U_ConnectTCPServer())
    {
        RS485_SendString_NoDirChange("[TCP] Connected to ");
        RS485_SendString_NoDirChange(TCP_SERVER_IP);
        RS485_SendString_NoDirChange(":");
        RS485_SendString_NoDirChange(TCP_SERVER_PORT);
        RS485_SendString_NoDirChange("\r\n");
        RS485_SendString_NoDirChange("[TCP] TCP transparent mode enabled.\r\n");
    }
    else
    {
        RS485_SendString_NoDirChange("[TCP] Connection failed!\r\n");
    }
    
    RS485_SendString_NoDirChange("\r\n");
    
    /* 等待发送完成 */
    HAL_Delay(10);
    
    /* 切回RS485接收模式 */
    RS485_SetReceiveMode();
    HAL_Delay(2);
}

/**
 * @brief  向RG200U发送单字节数据
 */
void RG200U_SendByte(uint8_t data)
{
    HAL_UART_Transmit(&huart5, &data, 1, 100);
}

/**
 * @brief  向RG200U发送字符串
 */
void RG200U_SendString(const char *str)
{
    HAL_UART_Transmit(&huart5, (uint8_t *)str, strlen(str), 1000);
}

/**
 * @brief  向RG200U发送数据缓冲区
 */
void RG200U_SendBuffer(const uint8_t *buf, uint16_t len)
{
    HAL_UART_Transmit(&huart5, (uint8_t *)buf, len, 1000);
}

/**
 * @brief  从RG200U接收单字节数据(非阻塞)
 */
bool RG200U_ReceiveByte(uint8_t *data)
{
    if (rx_read_index != rx_write_index)
    {
        *data = rg200u_rx_buffer[rx_read_index];
        rx_read_index = (rx_read_index + 1) % RG200U_RX_BUFFER_SIZE;
        return true;
    }
    return false;
}

/**
 * @brief  UART5接收中断回调函数
 */
void RG200U_UART_RxCallback(void)
{
    uint16_t next_write_index = (rx_write_index + 1) % RG200U_RX_BUFFER_SIZE;
    
    if (next_write_index != rx_read_index)
    {
        rg200u_rx_buffer[rx_write_index] = uart_rx_byte;
        rx_write_index = next_write_index;
    }
    
    HAL_UART_Receive_IT(&huart5, &uart_rx_byte, 1);
}

/**
 * @brief  连接TCP服务器
 * @retval 1:成功 0:失败
 */
uint8_t RG200U_ConnectTCPServer(void)
{
    char response[AT_RESPONSE_BUF_SIZE];
    char cmd[128];
    
    /* 清空接收缓冲 */
    rx_read_index = rx_write_index;
    
    /* 关闭可能存在的旧TCP连接（静默执行，不打印日志） */
    RG200U_SendATCommand("AT+QICLOSE=0\r\n", response, 2000);

    /* 配置URC输出到所有端口(连接前确保配置生效) */
    RG200U_SendATCommand("AT+QURCCFG=\"urcport\",\"all\"\r\n", response, 2000);
    
    /* 构造连接命令: AT+QIOPEN=1,0,"TCP","服务器IP",端口,0,0 */
    snprintf(cmd, sizeof(cmd), "AT+QIOPEN=1,%d,\"TCP\",\"%s\",%s,0,0\r\n", 
             TCP_SOCKET_ID, TCP_SERVER_IP, TCP_SERVER_PORT);
    
    /* 切换到RS485发送模式显示调试信息 */
#if RG200U_DEBUG_ENABLE
    RS485_SetTransmitMode();
    HAL_Delay(1);
    RS485_SendString_NoDirChange("[DEBUG] Send: ");
    RS485_SendString_NoDirChange(cmd);
    RS485_SetReceiveMode();
    HAL_Delay(10);
#endif
    
    /* 发送连接命令 */
    tcp_state = TCP_STATE_CONNECTING;
    
    if (RG200U_SendATCommand(cmd, response, 5000))  /* 先等5秒获取OK */
    {
        /* 显示第一次响应 */
#if RG200U_DEBUG_ENABLE
        RS485_SetTransmitMode();
        HAL_Delay(1);
        RS485_SendString_NoDirChange("[DEBUG] Response 1: ");
        RS485_SendString_NoDirChange(response);
        RS485_SendString_NoDirChange("\r\n");
        RS485_SetReceiveMode();
        HAL_Delay(10);
#endif
        
        /* 检查是否收到OK */
        if (strstr(response, "OK"))
        {
            /* OK收到，现在等待+QIOPEN通知（可能需要几秒） */
            /* 使用一次性长时间等待，而不是多次短时间等待 */
            memset(response, 0, sizeof(response));
#if RG200U_DEBUG_ENABLE
            RS485_SetTransmitMode();
            HAL_Delay(1);
            RS485_SendString_NoDirChange("[DEBUG] Waiting for +QIOPEN (up to 30s)...\r\n");
            RS485_SetReceiveMode();
            HAL_Delay(10);
#endif
            
            if (RG200U_WaitForResponse("+QIOPEN:", response, sizeof(response), 30000))  /* 等待30秒 */
            {
                /* 显示+QIOPEN响应 */
#if RG200U_DEBUG_ENABLE
                RS485_SetTransmitMode();
                HAL_Delay(1);
                RS485_SendString_NoDirChange("[DEBUG] QIOPEN: ");
                RS485_SendString_NoDirChange(response);
                RS485_SendString_NoDirChange("\r\n");
                RS485_SetReceiveMode();
                HAL_Delay(10);
#endif
                
                /* 解析连接结果: +QIOPEN: <connectID>,<err> */
                /* 如果err=0表示成功，err!=0表示失败 */
                if (strstr(response, "+QIOPEN: 0,0") || strstr(response, "+QIOPEN: 0, 0"))
                {
                    tcp_state = TCP_STATE_CONNECTED;
                    rx_read_index = rx_write_index;  /* 清空缓冲 */
                    return 1;
                }
                else
                {
                    /* 连接失败，解析并显示错误码 */
                    const char *qiopen = strstr(response, "+QIOPEN:");
                    if (qiopen)
                    {
                        /* 提取错误码 */
                        int conn_id, err_code;
                        if (sscanf(qiopen, "+QIOPEN: %d,%d", &conn_id, &err_code) == 2)
                        {
                            char err_msg[100];
                            RS485_SetTransmitMode();
                            HAL_Delay(1);
                            RS485_SendString_NoDirChange("[ERROR] Connection failed with code ");
                            snprintf(err_msg, sizeof(err_msg), "%d", err_code);
                            RS485_SendString_NoDirChange(err_msg);
                            
                            /* 显示错误码含义 */
                            RS485_SendString_NoDirChange(" (");
                            switch(err_code)
                            {
                                case 0:   RS485_SendString_NoDirChange("Operation success"); break;
                                case 550: RS485_SendString_NoDirChange("Unknown error"); break;
                                case 551: RS485_SendString_NoDirChange("Operation blocked"); break;
                                case 552: RS485_SendString_NoDirChange("Invalid parameters"); break;
                                case 553: RS485_SendString_NoDirChange("Memory not enough"); break;
                                case 554: RS485_SendString_NoDirChange("Socket creation failed"); break;
                                case 555: RS485_SendString_NoDirChange("Operation not supported"); break;
                                case 556: RS485_SendString_NoDirChange("Socket bind failed"); break;
                                case 557: RS485_SendString_NoDirChange("Socket listen failed"); break;
                                case 558: RS485_SendString_NoDirChange("Socket write failed"); break;
                                case 559: RS485_SendString_NoDirChange("Socket read failed"); break;
                                case 560: RS485_SendString_NoDirChange("Socket accept failed"); break;
                                case 561: RS485_SendString_NoDirChange("PDP context opening failed"); break;
                                case 562: RS485_SendString_NoDirChange("PDP context closure failed"); break;
                                case 563: RS485_SendString_NoDirChange("Socket identity has been used"); break;
                                case 564: RS485_SendString_NoDirChange("DNS busy"); break;
                                case 565: RS485_SendString_NoDirChange("DNS parse failed"); break;
                                case 566: RS485_SendString_NoDirChange("Socket connect failed"); break;
                                case 567: RS485_SendString_NoDirChange("Socket has been closed"); break;
                                case 568: RS485_SendString_NoDirChange("Operation busy"); break;
                                case 569: RS485_SendString_NoDirChange("Operation timeout"); break;
                                case 570: RS485_SendString_NoDirChange("PDP context broken down"); break;
                                case 571: RS485_SendString_NoDirChange("Cancel sending"); break;
                                case 572: RS485_SendString_NoDirChange("Operation not allowed"); break;
                                case 573: RS485_SendString_NoDirChange("APN not configured"); break;
                                case 574: RS485_SendString_NoDirChange("Port busy"); break;
                                default:  RS485_SendString_NoDirChange("Unknown"); break;
                            }
                            RS485_SendString_NoDirChange(")\r\n");
                            RS485_SetReceiveMode();
                        }
                    }
                    
                    tcp_state = TCP_STATE_ERROR;
                    return 0;
                }
            }
            
            /* 超时未收到+QIOPEN */
#if RG200U_DEBUG_ENABLE
            RS485_SetTransmitMode();
            HAL_Delay(1);
            RS485_SendString_NoDirChange("[DEBUG] Timeout waiting for +QIOPEN\r\n");
            RS485_SetReceiveMode();
#endif
        }
        else
        {
            /* 未收到OK */
#if RG200U_DEBUG_ENABLE
            RS485_SetTransmitMode();
            HAL_Delay(1);
            RS485_SendString_NoDirChange("[DEBUG] No OK received\r\n");
            RS485_SetReceiveMode();
#endif
        }
    }
    else
    {
        /* 命令发送失败 */
#if RG200U_DEBUG_ENABLE
        RS485_SetTransmitMode();
        HAL_Delay(1);
        RS485_SendString_NoDirChange("[DEBUG] AT command failed\r\n");
        RS485_SetReceiveMode();
#endif
    }
    
    tcp_state = TCP_STATE_ERROR;
    return 0;
}

/**
 * @brief  获取TCP连接状态
 * @retval TCP连接状态
 */
TCP_State_t RG200U_GetTCPState(void)
{
    return tcp_state;
}

/**
 * @brief  读取TCP数据
 * @param  buffer: 数据缓冲区
 * @param  max_len: 最大长度
 * @retval 实际读取的字节数
 */
uint8_t RG200U_ReadTCPData(char *buffer, uint16_t max_len)
{
    char response[512];
    char cmd[32];
    uint16_t index = 0;
    uint32_t start_tick;
    uint8_t data;
    int data_len = 0;
    
    /* 构造读取命令: AT+QIRD=0,1500 */
    snprintf(cmd, sizeof(cmd), "AT+QIRD=%d,%d\r\n", TCP_SOCKET_ID, max_len);
    
    /* 清空响应缓冲 */
    memset(response, 0, sizeof(response));
    
    /* 清空接收缓冲区 */
    rx_read_index = rx_write_index;
    
    /* 发送读取命令 */
    HAL_UART_Transmit(&huart5, (uint8_t *)cmd, strlen(cmd), 1000);
    HAL_Delay(100);  /* 等待模块处理 */
    
    /* 接收响应 */
    start_tick = HAL_GetTick();
    while ((HAL_GetTick() - start_tick) < 2000 && index < sizeof(response) - 1)
    {
        if (RG200U_ReceiveByte(&data))
        {
            response[index++] = data;
            response[index] = '\0';
            
            /* 如果收到OK,说明响应完整 */
            if (strstr(response, "OK\r\n"))
            {
                break;
            }
        }
        HAL_Delay(1);
    }
    
    /* 解析响应: +QIRD: <length>\r\n<data>\r\nOK */
    const char *qird = strstr(response, "+QIRD:");
    if (qird)
    {
        /* 提取数据长度 */
        if (sscanf(qird + 7, "%d", &data_len) == 1 && data_len > 0)
        {
            /* 查找数据起始位置（+QIRD行之后的\n） */
            const char *line_end = strchr(qird, '\n');
            if (line_end)
            {
                const char *data_start = line_end + 1;  /* 跳过\n */
                
                /* 查找数据结束位置（OK之前） */
                const char *ok_pos = strstr(data_start, "\r\nOK");
                if (ok_pos)
                {
                    /* 计算实际数据长度 */
                    int actual_len = ok_pos - data_start;
                    
                    /* 复制数据到缓冲区 */
                    int copy_len = (actual_len < max_len) ? actual_len : max_len;
                    if (copy_len < data_len)
                    {
                        copy_len = (data_len < max_len) ? data_len : max_len;
                    }
                    
                    memcpy(buffer, data_start, copy_len);
                    buffer[copy_len] = '\0';
                    
                    return copy_len;
                }
            }
        }
    }
    
    return 0;
}

/**
 * @brief  处理TCP消息（检测+QIURC通知并读取数据）
 * @note   应该在主循环或任务中定期调用此函数
 */
void RG200U_ProcessTCPMessage(void)
{
    static char buffer[RG200U_RX_BUFFER_SIZE];  /* 改为静态，保留上次的数据 */
    static uint16_t index = 0;
    uint8_t data;
#if RG200U_DEBUG_ENABLE
    static uint32_t last_debug_time = 0;
#endif
    
    /* 检查是否有未处理的数据 */
    while (RG200U_ReceiveByte(&data))
    {
        if (index < (RG200U_RX_BUFFER_SIZE - 1))
        {
            buffer[index++] = data;
            buffer[index] = '\0';
            
            /* 每5秒打印一次缓冲区内容用于调试 */
#if RG200U_DEBUG_ENABLE
            if ((HAL_GetTick() - last_debug_time) > 5000 && index > 0)
            {
                RS485_SetTransmitMode();
                HAL_Delay(1);
                RS485_SendString_NoDirChange("\r\n[DEBUG] Buffer: ");
                RS485_SendString_NoDirChange(buffer);
                RS485_SendString_NoDirChange("\r\n");
                RS485_SetReceiveMode();
                HAL_Delay(10);
                last_debug_time = HAL_GetTick();
            }
#endif
            
            /* 检查是否收到TCP数据通知: +QIURC: "recv",0 */
            if (strstr(buffer, "+QIURC: \"recv\""))
            {
                char tcp_data[512];
                uint8_t len;
                
                /* 显示检测到通知 */
#if RG200U_DEBUG_ENABLE
                RS485_SetTransmitMode();
                HAL_Delay(1);
                RS485_SendString_NoDirChange("\r\n[DEBUG] Detected +QIURC recv notification\r\n");
                RS485_SetReceiveMode();
                HAL_Delay(10);
#endif
                
                /* 收到TCP数据通知，读取数据 */
                HAL_Delay(50);  /* 短暂延时，等待数据准备好 */
                
                len = RG200U_ReadTCPData(tcp_data, sizeof(tcp_data) - 1);
                
                /* 显示读取结果 */
#if RG200U_DEBUG_ENABLE
                RS485_SetTransmitMode();
                HAL_Delay(1);
                RS485_SendString_NoDirChange("[DEBUG] Read length: ");
                {
                    char len_str[16];
                    snprintf(len_str, sizeof(len_str), "%d", len);
                    RS485_SendString_NoDirChange(len_str);
                }
                RS485_SendString_NoDirChange("\r\n");
                RS485_SetReceiveMode();
                HAL_Delay(10);
#endif
                
                if (len > 0)
                {
                    /* 将TCP数据通过RS485发送到上位机 */
                    RS485_SetTransmitMode();
                    HAL_Delay(1);
                    
                    RS485_SendString_NoDirChange("\r\n[TCP RX] ");
                    RS485_SendBuffer((uint8_t *)tcp_data, len);
                    RS485_SendString_NoDirChange("\r\n");
                    
                    HAL_Delay(2);
                    RS485_SetReceiveMode();
                    
                    /* 处理接收到的命令 */
                    RG200U_ProcessCommand(tcp_data, len);
                }
                
                /* 清空缓冲，准备下一次检测 */
                index = 0;
                memset(buffer, 0, sizeof(buffer));
            }
        }
        else
        {
            /* 缓冲区满，清空 */
#if RG200U_DEBUG_ENABLE
            RS485_SetTransmitMode();
            HAL_Delay(1);
            RS485_SendString_NoDirChange("\r\n[DEBUG] Buffer overflow, clearing\r\n");
            RS485_SetReceiveMode();
            HAL_Delay(10);
#endif
            
            index = 0;
            memset(buffer, 0, sizeof(buffer));
        }
    }
}

/**
 * @brief  处理TCP接收到的命令
 * @param  cmd_data: 命令数据
 * @param  len: 数据长度
 */
static void RG200U_ProcessCommand(const char *cmd_data, uint16_t len)
{
    /* 确保数据以null结尾 */
    char cmd[128];
    if (len >= sizeof(cmd)) {
        len = sizeof(cmd) - 1;
    }
    memcpy(cmd, cmd_data, len);
    cmd[len] = '\0';
    
    /* 去除可能的换行符 */
    char *newline = strchr(cmd, '\r');
    if (newline) *newline = '\0';
    newline = strchr(cmd, '\n');
    if (newline) *newline = '\0';
    
    /* 命令处理 */
    RS485_SetTransmitMode();
    HAL_Delay(1);
    
    if (strcmp(cmd, "RELAY1_ON") == 0)
    {
        /* 继电器1打开 */
        HAL_GPIO_WritePin(RELAY_K1_GPIO_Port, RELAY_K1_Pin, GPIO_PIN_SET);
        RS485_SendString_NoDirChange("[CMD] RELAY1 ON\r\n");
    }
    else if (strcmp(cmd, "RELAY1_OFF") == 0)
    {
        /* 继电器1关闭 */
        HAL_GPIO_WritePin(RELAY_K1_GPIO_Port, RELAY_K1_Pin, GPIO_PIN_RESET);
        RS485_SendString_NoDirChange("[CMD] RELAY1 OFF\r\n");
    }
    else if (strcmp(cmd, "RELAY2_ON") == 0)
    {
        /* 继电器2打开 */
        HAL_GPIO_WritePin(RELAY_K2_GPIO_Port, RELAY_K2_Pin, GPIO_PIN_SET);
        RS485_SendString_NoDirChange("[CMD] RELAY2 ON\r\n");
    }
    else if (strcmp(cmd, "RELAY2_OFF") == 0)
    {
        /* 继电器2关闭 */
        HAL_GPIO_WritePin(RELAY_K2_GPIO_Port, RELAY_K2_Pin, GPIO_PIN_RESET);
        RS485_SendString_NoDirChange("[CMD] RELAY2 OFF\r\n");
    }
    else
    {
        /* 未知命令 */
        RS485_SendString_NoDirChange("[CMD] Unknown: ");
        RS485_SendString_NoDirChange(cmd);
        RS485_SendString_NoDirChange("\r\n");
    }
    
    HAL_Delay(2);
    RS485_SetReceiveMode();
}

