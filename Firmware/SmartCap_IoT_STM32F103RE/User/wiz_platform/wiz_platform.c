#include "wiz_platform.h"
#include "wizchip_conf.h"
#include "main.h"
#include "gpio.h"
#include "wiz_interface.h"
#include <stdint.h>

extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim2;

/**
 * @brief   SPI 选择 wizchip
 * @param   无
 * @return  无
 */
void wizchip_select(void)
{
    HAL_GPIO_WritePin(SCSn_GPIO_Port, SCSn_Pin, GPIO_PIN_RESET);
}

/**
 * @brief   SPI 取消选择 wizchip
 * @param   无
 * @return  无
 */
void wizchip_deselect(void)
{
    HAL_GPIO_WritePin(SCSn_GPIO_Port, SCSn_Pin, GPIO_PIN_SET);
}

/**
 * @brief   SPI 向 wizchip 写入 1 字节
 * @param   dat:1 字节数据
 * @return  无
 */
void wizchip_write_byte(uint8_t dat)
{
    HAL_SPI_Transmit(&hspi2, &dat, 1, 0xffff);
}

/**
 * @brief   SPI 从 wizchip 读取 1 字节
 * @param   无
 * @return  1 字节数据
 */
uint8_t wizchip_read_byte(void)
{
    uint8_t dat;
    HAL_SPI_Receive(&hspi2, &dat, 1, 0xffff);
    return dat;
}

/**
 * @brief   SPI 向 wizchip 写入缓冲区
 * @param   buff:写入缓冲区
 * @param   len:写入长度
 * @return  无
 */
void wizchip_write_buff(uint8_t *buf, uint16_t len)
{
    HAL_SPI_Transmit(&hspi2, buf, len, 0xffff);
}

/**
 * @brief   SPI 从 wizchip 读取缓冲区
 * @param   buff:读取缓冲区
 * @param   len:读取长度
 * @return  无
 */
void wizchip_read_buff(uint8_t *buf, uint16_t len)
{
    HAL_SPI_Receive(&hspi2, buf, len, 0xffff);
}

/**
 * @brief   硬件重置 wizchip
 * @param   无
 * @return  无
 */
void wizchip_reset(void)
{
    HAL_GPIO_WritePin(RSTn_GPIO_Port, RSTn_Pin, GPIO_PIN_SET);
    wiz_user_delay_ms(10);
    HAL_GPIO_WritePin(RSTn_GPIO_Port, RSTn_Pin, GPIO_PIN_RESET);
    wiz_user_delay_ms(10);
    HAL_GPIO_WritePin(RSTn_GPIO_Port, RSTn_Pin, GPIO_PIN_SET);
    wiz_user_delay_ms(10);
}

/**
 * @brief   wizchip spi 回调注册
 * @param   无
 * @return  无
 */
void wizchip_spi_cb_reg(void)
{
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
    reg_wizchip_spi_cbfunc(wizchip_read_byte, wizchip_write_byte);
    reg_wizchip_spiburst_cbfunc(wizchip_read_buff, wizchip_write_buff);
}

/**
 * @brief   硬件平台定时器中断回调函数
 * @note    已移到 main.c 的 HAL_TIM_PeriodElapsedCallback 中处理
 */
// void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
// {
//     if (htim->Instance == TIM2)
//     {
//         wiz_timer_handler();
//     }
// }

/**
 * @brief   打开 wiz 定时器中断
 * @param   无
 * @return  无
 */
void wiz_tim_irq_enable(void)
{
    HAL_TIM_Base_Start_IT(&htim2);
}

/**
 * @brief   关闭 wiz 定时器中断
 * @param   无
 * @return  无
 */
void wiz_tim_irq_disable(void)
{
    HAL_TIM_Base_Stop_IT(&htim2);
}
