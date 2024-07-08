#include "list.h"
#include "task.h"
 #include "FreeRTOS.h"

                        /*全局变量*/
/*************************************************************/
portCHAR flag1;
portCHAR flag2;
portCHAR flag3;

extern List_t pxReadyTasksLists[ configMAX_PRIORITIES ];
/*************************************************************/

/*定义任务栈的大小*/
#define TASK1_STACK_SIZE 128
StackType_t Task1Stack[TASK1_STACK_SIZE]; 

#define TASK2_STACK_SIZE 128
StackType_t Task2Stack[TASK2_STACK_SIZE];

#define TASK3_STACK_SIZE 128
StackType_t Task3Stack[TASK3_STACK_SIZE];

/*定义控制块*/
TCB_t Task1TCB;
TCB_t Task2TCB;
TCB_t Task3TCB;
/*定义了任务句柄*/
TaskHandle_t Task1_Handle;
TaskHandle_t Task2_Handle;
TaskHandle_t Task3_Handle;



/*************************************************空闲任务****************************************/
/* 定义空闲任务的栈 */
#define configMINIMAL_STACK_SIZE ( ( unsigned short ) 128 ) 
StackType_t IdleTaskStack[configMINIMAL_STACK_SIZE];
/* 定义空闲任务的任务控制块 */
TCB_t IdleTaskTCB;




/*函数声明*/
void delay (uint32_t count);
void Task1_Entry( void *p_arg );
void Task2_Entry( void *p_arg );
void Task3_Entry( void *p_arg );



int main(void)
{
    /* 初始化与任务相关的列表，如就绪列表 */
    prvInitialiseTaskLists();

    /* 任务句柄 */
    Task1_Handle = xTaskCreateStatic( (TaskFunction_t)Task1_Entry, /* 任务入口 */
                                    (char *)"Task1", /* 任务名称，字符串形式 */
                                    (uint32_t)TASK1_STACK_SIZE , /* 任务栈大小，单位为字 */
                                    (void *) NULL, /* 任务形参 */
                                    (UBaseType_t) 2,
                                    (StackType_t *)Task1Stack, /* 任务栈起始地址 */
                                    (TCB_t *)&Task1TCB ); /* 任务控制块 */
    
    
    /* 任务句柄 */
    Task2_Handle = xTaskCreateStatic( (TaskFunction_t)Task2_Entry, /* 任务入口 */
                                        (char *)"Task2", /* 任务名称，字符串形式 */
                                        (uint32_t)TASK2_STACK_SIZE , /* 任务栈大小，单位为字 */
                                        (void *) NULL, /* 任务形参 */
                                        (UBaseType_t) 2,
                                        (StackType_t *)Task2Stack, /* 任务栈起始地址 */
                                        (TCB_t *)&Task2TCB ); /* 任务控制块 */
										
										
	/* 任务句柄 */								
	Task3_Handle =	xTaskCreateStatic( (TaskFunction_t)Task3_Entry,
										(char *)"Task3",
										(uint32_t)TASK3_STACK_SIZE ,
										(void *) NULL,
										(UBaseType_t) 3,
										(StackType_t *)Task3Stack,
										(TCB_t *)&Task3TCB );
														
										
										
  
    vTaskStartScheduler();
    while(1)
    {
        /*Function  error....*/
    }
}

void delay (uint32_t count)
{
    for (; count!=0; count--);
}

/* 任务 1 */
void Task1_Entry( void *p_arg )
{
    for ( ;; )
    {
       flag1 = 1;
      // vTaskDelay(20); 
		delay(100);
       flag1 = 0;
      // vTaskDelay(20);
		delay(100);
    }
}
/*任务 2*/
void Task2_Entry( void *p_arg )
{
    for ( ;; )
    {
        flag2 = 1;
        //vTaskDelay(20);
		delay(100);
        flag2 = 0;
        //vTaskDelay(20);
		delay(100);
    }
}
/*任务 3*/
void Task3_Entry( void *p_arg )
{
    for ( ;; )
    {
        flag3 = 1;
        vTaskDelay( 1 );
        flag3 = 0;
        vTaskDelay( 1 );
    }
}




/*将pxIdleTaskTCBBuffer 和pxIdleTaskStackBuffer 
这两个接下来要作为形参传到 xTaskCreateStatic()函数的指针分别指
向空闲任务的 TCB 和栈的起始地址*/
void vApplicationGetIdleTaskMemory( TCB_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
    *ppxIdleTaskTCBBuffer=&IdleTaskTCB;
    *ppxIdleTaskStackBuffer=IdleTaskStack;
    *pulIdleTaskStackSize=configMINIMAL_STACK_SIZE;
}


