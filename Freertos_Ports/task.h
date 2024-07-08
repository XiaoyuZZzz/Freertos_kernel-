#ifndef __TASK_H
#define __TASK_H

#include "list.h"
#include "projdefs.h"
#include "port.h"

/*�����л�*/
#define taskYIELD() portYIELD()

/*�����������ȼ�*/
#define tskIDLE_PRIORITY ( ( UBaseType_t ) 0U )


typedef void * TaskHandle_t;

/*�����ٽ�β���������Ƕ��*/
#define taskENTER_CRITICAL() portENTER_CRITICAL()
/*�����ٽ�α�����Ƕ��*/
#define taskENTER_CRITICAL_FROM_ISR() portSET_INTERRUPT_MASK_FROM_ISR()

#define taskEXIT_CRITICAL()                portEXIT_CRITICAL()





/*��������Ŀ��ƿ�TCB*/
typedef struct tskTaskControlBlock
 {
    volatile StackType_t *pxTopOfStack;                         /* ջ�� */

    ListItem_t xStateListItem;                                  /* ����ڵ� */

    StackType_t *pxStack;                                       /* ����ջ��ʼ��ַ */
    
    char pcTaskName[ configMAX_TASK_NAME_LEN ];                 /* �������ƣ��ַ�����ʽ */
     
    TickType_t xTicksToDelay;                                   /* ������ʱ */
     
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
