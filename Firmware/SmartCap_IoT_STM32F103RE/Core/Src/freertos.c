/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "user_tasks.h"  // 用户任务实现

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId RS485_RxTaskHandle;
osThreadId RG200U_RxTaskHandle;
osThreadId RS485_TxTaskHandle;
osThreadId RG200U_TxTaskHandle;
osMessageQId Queue_RS485_To_RG200UHandle;
osMessageQId Queue_RG200U_To_RS485Handle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void Task_RS485_Handler(void const * argument);
void Task_RG200U_Handler(void const * argument);
void Task_RS485_Transmit(void const * argument);
void Task_RG200U_Transmit(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of Queue_RS485_To_RG200U */
  osMessageQDef(Queue_RS485_To_RG200U, 256, 1);
  Queue_RS485_To_RG200UHandle = osMessageCreate(osMessageQ(Queue_RS485_To_RG200U), NULL);

  /* definition and creation of Queue_RG200U_To_RS485 */
  osMessageQDef(Queue_RG200U_To_RS485, 256, 1);
  Queue_RG200U_To_RS485Handle = osMessageCreate(osMessageQ(Queue_RG200U_To_RS485), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityLow, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of RS485_RxTask */
  osThreadDef(RS485_RxTask, Task_RS485_Handler, osPriorityAboveNormal, 0, 512);
  RS485_RxTaskHandle = osThreadCreate(osThread(RS485_RxTask), NULL);

  /* definition and creation of RG200U_RxTask */
  osThreadDef(RG200U_RxTask, Task_RG200U_Handler, osPriorityAboveNormal, 0, 512);
  RG200U_RxTaskHandle = osThreadCreate(osThread(RG200U_RxTask), NULL);

  /* definition and creation of RS485_TxTask */
  osThreadDef(RS485_TxTask, Task_RS485_Transmit, osPriorityNormal, 0, 512);
  RS485_TxTaskHandle = osThreadCreate(osThread(RS485_TxTask), NULL);

  /* definition and creation of RG200U_TxTask */
  osThreadDef(RG200U_TxTask, Task_RG200U_Transmit, osPriorityNormal, 0, 512);
  RG200U_TxTaskHandle = osThreadCreate(osThread(RG200U_TxTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
__weak void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  
  /* 转发到用户任务实现 */
  UserTask_Default(argument);
  
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_Task_RS485_Handler */
/**
* @brief Function implementing the RS485_RxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_RS485_Handler */
__weak void Task_RS485_Handler(void const * argument)
{
  /* USER CODE BEGIN Task_RS485_Handler */
  
  /* 转发到用户任务实现 */
  UserTask_RS485_RxHandler(argument);
  
  /* USER CODE END Task_RS485_Handler */
}

/* USER CODE BEGIN Header_Task_RG200U_Handler */
/**
* @brief Function implementing the RG200U_RxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_RG200U_Handler */
__weak void Task_RG200U_Handler(void const * argument)
{
  /* USER CODE BEGIN Task_RG200U_Handler */
  
  /* 转发到用户任务实现 */
  UserTask_RG200U_RxHandler(argument);
  
  /* USER CODE END Task_RG200U_Handler */
}

/* USER CODE BEGIN Header_Task_RS485_Transmit */
/**
* @brief Function implementing the RS485_TxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_RS485_Transmit */
__weak void Task_RS485_Transmit(void const * argument)
{
  /* USER CODE BEGIN Task_RS485_Transmit */
  
  /* 转发到用户任务实现 */
  UserTask_RS485_TxHandler(argument);
  
  /* USER CODE END Task_RS485_Transmit */
}

/* USER CODE BEGIN Header_Task_RG200U_Transmit */
/**
* @brief Function implementing the RG200U_TxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_RG200U_Transmit */
__weak void Task_RG200U_Transmit(void const * argument)
{
  /* USER CODE BEGIN Task_RG200U_Transmit */
  
  /* 转发到用户任务实现 */
  UserTask_RG200U_TxHandler(argument);
  
  /* USER CODE END Task_RG200U_Transmit */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

