#include "wiz_interface.h"
#include "wiz_platform.h"
#include "wizchip_conf.h"
#include "dhcp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W5500_VERSION 0x04

struct wiz_timer
{
    void (*func)(void);     // 定时器触发时执行的回调函数指针
    uint32_t trigger_time;  // 触发时间(ms)
    uint32_t count_time;    // 当前计数值(ms)
    struct wiz_timer *next; // 下一个定时器节点
};

struct wiz_timer *wiz_timer_head = NULL;
volatile uint32_t wiz_delay_ms_count = 0;

/**
 * @brief 创建 wiz_timer 节点
 * @param func :定时器触发时执行的回调函数指针
 * @param time :触发时间，通常以毫秒为单位
 *
 * @return 成功创建时返回指向 wiz_timer 节点的指针，否则返回 NULL
 */
static struct wiz_timer *create_wiz_timer_node(void (*func)(void), uint32_t time)
{
    struct wiz_timer *newNode = (struct wiz_timer *)malloc(sizeof(struct wiz_timer));
    if (newNode == NULL)
    {
        printf("内存分配失败。\n");
        return NULL;
    }
    newNode->func = func;
    newNode->trigger_time = time;
    newNode->count_time = 0;
    newNode->next = NULL;
    return newNode;
}

/**
 * @brief 将新的定时器节点添加到定时器链表
 * @param func 回调函数指针，在定时器时间到达时调用
 * @param time 触发时间 (ms)
 */
void wiz_add_timer(void (*func)(void), uint32_t time)
{
    struct wiz_timer *newNode = create_wiz_timer_node(func, time);
    if (wiz_timer_head == NULL)
    {
        wiz_timer_head = newNode;
        return;
    }
    struct wiz_timer *temp = wiz_timer_head;
    while (temp->next != NULL)
    {
        temp = temp->next;
    }
    temp->next = newNode;
}

/**
 * @brief 删除指定回调函数的定时器
 * @param func 回调函数指针
 * @return 无
 */
void wiz_delete_timer(void (*func)(void))
{
    struct wiz_timer *temp = wiz_timer_head;
    struct wiz_timer *prev = NULL;

    if (temp != NULL && temp->func == func)
    {
        wiz_timer_head = temp->next;
        free(temp);
        return;
    }

    while (temp != NULL && temp->func != func)
    {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL)
        return;

    prev->next = temp->next;
    free(temp);
}

/**
 * @brief wiz 定时器事件处理器
 *
 * 你必须将此函数添加到你的 1ms 定时器中断中
 *
 */
void wiz_timer_handler(void)
{

    wiz_delay_ms_count++;
    struct wiz_timer *temp = wiz_timer_head;
    while (temp != NULL)
    {
        temp->count_time++;
        if (temp->count_time >= temp->trigger_time)
        {
            temp->count_time = 0;
            temp->func();
        }
        temp = temp->next;
    }
}

/**
 * @brief 毫秒延迟函数
 * @param nms :延迟时间
 */
void wiz_user_delay_ms(uint32_t nms)
{
    wiz_delay_ms_count = 0;
    while (wiz_delay_ms_count < nms)
    {
    }
}

/**
 * @brief 检查 WIZCHIP 版本
 */
void wizchip_version_check(void)
{
    uint8_t error_count = 0;
    while (1)
    {
        wiz_user_delay_ms(1000);
        if (getVERSIONR() != W5500_VERSION)
        {
            error_count++;
            if (error_count > 5)
            {
                printf("错误，W5500 版本应为 0x%02x，但读取到的 W5500 版本值为 0x%02x\r\n", W5500_VERSION, getVERSIONR());
                while (1)
                    ;
            }
        }
        else
        {
            break;
        }
    }
}

/**
 * @brief 打印 PHY 信息
 */
void wiz_print_phy_info(void)
{
    uint8_t get_phy_conf;
    get_phy_conf = getPHYCFGR();
    printf("当前速率 : %dMbps\r\n", get_phy_conf & 0x02 ? 100 : 10);
    printf("当前双工模式 : %s\r\n", get_phy_conf & 0x04 ? "全双工" : "半双工");
}

/**
 * @brief 以太网链路检测
 */
void wiz_phy_link_check(void)
{
    uint8_t phy_link_status;
    do
    {
        wiz_user_delay_ms(1000);
        ctlwizchip(CW_GET_PHYLINK, (void *)&phy_link_status);
        if (phy_link_status == PHY_LINK_ON)
        {
            printf("PHY 已连接\r\n");
            wiz_print_phy_info();
        }
        else
        {
            printf("PHY 未连接\r\n");
        }
    } while (phy_link_status == PHY_LINK_OFF);
}

/**
 * @brief   wizchip 初始化函数
 * @param   无
 * @return  无
 */
void wizchip_initialize(void)
{
    /* 启用定时器中断 */
    wiz_tim_irq_enable();

    /* 注册 wizchip spi */
    wizchip_spi_cb_reg();

    /* 重置 wizchip */
    wizchip_reset();

    /* 读取版本寄存器 */
    wizchip_version_check();

    /* 检查 PHY 链路状态，使 PHY 正常启动 */
    wiz_phy_link_check();
}

/**
 * @brief   打印网络信息
 * @param   无
 * @return  无
 */
void print_network_information(void)
{
    wiz_NetInfo net_info;
    wizchip_getnetinfo(&net_info); // 获取芯片配置信息

    if (net_info.dhcp == NETINFO_DHCP)
    {
        printf("====================================================================================================\r\n");
        printf(" %s 网络配置 : DHCP\r\n\r\n", _WIZCHIP_ID_);
    }
    else
    {
        printf("====================================================================================================\r\n");
        printf(" %s 网络配置 : 静态\r\n\r\n", _WIZCHIP_ID_);
    }

    printf(" MAC         : %02X:%02X:%02X:%02X:%02X:%02X\r\n", net_info.mac[0], net_info.mac[1], net_info.mac[2], net_info.mac[3], net_info.mac[4], net_info.mac[5]);
    printf(" IP          : %d.%d.%d.%d\r\n", net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3]);
    printf(" 子网掩码    : %d.%d.%d.%d\r\n", net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3]);
    printf(" 网关        : %d.%d.%d.%d\r\n", net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3]);
    printf(" DNS         : %d.%d.%d.%d\r\n", net_info.dns[0], net_info.dns[1], net_info.dns[2], net_info.dns[3]);
    printf("====================================================================================================\r\n\r\n");
}

/**
 * @brief DHCP 处理过程
 * @param sn :套接字编号
 * @param buffer :套接字缓冲区
 */
static uint8_t wiz_dhcp_process(uint8_t sn, uint8_t *buffer)
{
    wiz_NetInfo conf_info;
    uint8_t dhcp_run_flag = 1;
    uint8_t dhcp_ok_flag = 0;
    /* 将 DHCP_time_handler 注册到 1 秒定时器 */
    wiz_add_timer(DHCP_time_handler, 1000);
    DHCP_init(sn, buffer);
    printf("DHCP 运行中\r\n");
    while (1)
    {
        switch (DHCP_run()) // 执行 DHCP 客户端
        {
        case DHCP_IP_LEASED: // DHCP 成功获取网络信息
        {
            if (dhcp_ok_flag == 0)
            {
                dhcp_ok_flag = 1;
                dhcp_run_flag = 0;
            }
            break;
        }
        case DHCP_FAILED:
        {
            dhcp_run_flag = 0;
            break;
        }
        }
        if (dhcp_run_flag == 0)
        {
            printf("DHCP %s!\r\n", dhcp_ok_flag ? "成功" : "失败");
            DHCP_stop();

            /*DHCP 获取成功，取消注册 DHCP_time_handler*/
            wiz_delete_timer(DHCP_time_handler);

            if (dhcp_ok_flag)
            {
                getIPfromDHCP(conf_info.ip);
                getGWfromDHCP(conf_info.gw);
                getSNfromDHCP(conf_info.sn);
                getDNSfromDHCP(conf_info.dns);
                conf_info.dhcp = NETINFO_DHCP;
                getSHAR(conf_info.mac);
                wizchip_setnetinfo(&conf_info); // 将网络信息更新为 DHCP 获取的网络信息
                return 1;
            }
            return 0;
        }
    }
}

/**
 * @brief   设置网络信息
 *
 * 首先确定是否使用 DHCP。如果使用 DHCP，首先通过 DHCP 获取 IP 地址。
 * 当 DHCP 失败时，使用静态 IP 配置网络信息。如果使用静态 IP，直接配置网络信息
 *
 * @param   sn: 套接字ID
 * @param   ethernet_buff:
 * @param   net_info: 网络信息结构体
 * @return  无
 */
void network_init(uint8_t *ethernet_buff, wiz_NetInfo *conf_info)
{
    int ret;
    wizchip_setnetinfo(conf_info); // 配置网络信息
    if (conf_info->dhcp == NETINFO_DHCP)
    {
        ret = wiz_dhcp_process(0, ethernet_buff);
        if (ret == 0)
        {
            conf_info->dhcp = NETINFO_STATIC;
            wizchip_setnetinfo(conf_info);
        }
    }
    print_network_information();
}
