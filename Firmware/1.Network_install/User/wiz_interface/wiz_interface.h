#ifndef __WIZ_INTERFACE_H__
#define __WIZ_INTERFACE_H__

#include "wizchip_conf.h"

/**
 * @brief 将新的定时器节点添加到定时器链表
 * @param func 回调函数指针，在定时器时间到达时调用
 * @param time 触发时间 (ms)
 */
void wiz_add_timer(void (*func)(void), uint32_t time);

/**
 * @brief 删除指定回调函数的定时器
 * @param func 回调函数指针
 * @return 无
 */
void wiz_delete_timer(void (*func)(void));

/**
 * @brief wiz 定时器事件处理器
 *
 * 你必须将此函数添加到你的 1ms 定时器中断中
 *
 */
void wiz_timer_handler(void);

/**
 * @brief 毫秒延迟函数
 * @param nms :延迟时间
 */
void wiz_user_delay_ms(uint32_t nms);

/**
 * @brief   wizchip 初始化函数
 * @param   无
 * @return  无
 */
void wizchip_initialize(void);

/**
 * @brief   打印网络信息
 * @param   无
 * @return  无
 */
void print_network_information(void);

/**
 * @brief   设置网络信息
 * @param   sn: 套接字ID
 * @param   ethernet_buff:
 * @param   net_info: 网络信息结构体
 * @return  无
 */
void network_init(uint8_t *ethernet_buff, wiz_NetInfo *conf_info);

/**
 * @brief 检查 WIZCHIP 版本
 */
void wizchip_version_check(void);

/**
 * @brief 以太网链路检测
 */
void wiz_phy_link_check(void);

/**
 * @brief 打印 PHY 信息
 */
void wiz_print_phy_info(void);
#endif
