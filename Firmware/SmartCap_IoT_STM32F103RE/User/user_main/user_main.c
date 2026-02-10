/* Includes ------------------------------------------------------------------*/
#include "User_main.h"
#include "rs485.h"
#include "rg200u.h"

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  用户硬件初始化
 */
void User_main(void)
{
    RS485_Init();
    RG200U_Init();
}


