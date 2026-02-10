#include "user_main.h"
#include <stdio.h>
#include <stdint.h>
#include "wizchip_conf.h"
#include "wiz_interface.h"

/*wizchip->STM32 硬件引脚定义*/
//	wizchip_SCS    --->     STM32_GPIOD7
//	wizchip_SCLK	 --->     STM32_GPIOB13
//  wizchip_MISO	 --->     STM32_GPIOB14
//	wizchip_MOSI	 --->     STM32_GPIOB15
//	wizchip_RESET	 --->     STM32_GPIOD8
//	wizchip_INT    --->     STM32_GPIOD9

/* 网络信息 */
wiz_NetInfo default_net_info = {
    .mac = {0x00, 0x08, 0xdc, 0x12, 0x22, 0x12},
    .ip = {192, 168, 1, 110},
    .gw = {192, 168, 1, 1},
    .sn = {255, 255, 255, 0},
    .dns = {8, 8, 8, 8},
    .dhcp = NETINFO_STATIC}; // 静态IP
uint8_t ethernet_buf[ETHERNET_BUF_MAX_SIZE] = {0};

/**
 * @brief   用户运行程序
 * @param   无
 * @return  无
 */
void user_run(void)
{
  wiz_NetInfo net_info;
  printf("wizchip 网络配置示例\r\n");

  /* 初始化 wizchip */
  wizchip_initialize();

  network_init(ethernet_buf, &default_net_info);

  wizchip_getnetinfo(&net_info);
  printf("请尝试 ping %d.%d.%d.%d\r\n", net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3]);
  while (1)
  {
  }
}
