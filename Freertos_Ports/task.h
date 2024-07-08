#ifndef __TASK_H
#define __TASK_H

#include "list.h"
#include "projdefs.h"
#include "port.h"

/*任务切换*/
#define taskYIELD() portYIELD()

/*空闲任务优先级*/
#define tskIDLE_PRIORITY ( ( UBaseType_t ) 0U )


typedef void * TaskHandle_t;

/*进入临界段不保护不可嵌套*/
#define taskENTER_CRITICAL() portENTER_CRITICAL()
/*进入临界段保护可嵌套*/
#define taskENTER_CRITICAL_FROM_ISR() portSET_INTERRUPT_MASK_FROM_ISR()

#define taskEXIT_CRITICAL()                portEXIT_CRITICAL()





/*定义任务的控制块TCB*/
typedef struct tskTaskControlBlock
 {
    volatile StackType_t *pxTopOfStack;                         /* 栈顶 */

    ListItem_t xStateListItem;                                  /* 任务节点 */

    StackType_t *pxStack;                                       /* 任务栈起始地址 */
    
    char pcTaskName[ configMAX_TASK_NAME_LEN ];                 /* 任务名称，字符串形式 */
     
    TickType_t xTicksToDelay;                                   /* 用于延时 */
     
     UBaseType_t uxPriority;
}tskTCB;
typedef tskTCB TCB_t;


void vTaskDelay( const TickType_t xTicksToDelay );
void vTaskSwitchContext( void );
void prvInitialiseTaskLists( void );
BaseType_t xTaskIncrementTick( void );
TaskHandle_t xTaskCreateStatic( TaskFunction_t pxTaskCode,
                                    const char * const pcName, 
                                    const uint32_t ulStackDepth, 
                                    void * const pvParameters, 
                                    UBaseType_t uxPriority,
                                    StackType_t * const puxStackBuffer, 
                                    TCB_t * const pxTaskBuffer );

static void prvInitialiseNewTask(TaskFunction_t pxTaskCode,
                                const char * const pcName, 
                                const uint32_t ulStackDepth, 
                                void * const pvParameters, 
                                UBaseType_t uxPriority,
                                TaskHandle_t * const pxCreatedTask, 
                                TCB_t *pxNewTCB );
void vTaskStartScheduler( void );



#endif
