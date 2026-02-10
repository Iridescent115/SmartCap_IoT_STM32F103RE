#ifndef __WIZ_PLATFORM_H__
#define __WIZ_PLATFORM_H__

#include <stdint.h>

/**
 * @brief   硬件重置 wizchip
 * @param   无
 * @return  无
 */
void wizchip_reset(void);

/**
 * @brief   注册 WIZCHIP SPI 回调函数
 * @param   无
 * @return  无
 */
void wizchip_spi_cb_reg(void);

/**
 * @brief   打开 wiz 定时器中断
 * @param   无
 * @return  无
 */
void wiz_tim_irq_enable(void);

/**
 * @brief   关闭 wiz 定时器中断
 * @param   无
 * @return  无
 */
void wiz_tim_irq_disable(void);
#endif
