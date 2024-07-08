#include "list.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "projdefs.h"
#include "portmacro.h"
#include "port.h"



#define prvAddTaskToReadyList( pxTCB )                               \
    taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );              \
    vListInsertEnd( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), \
                    &( ( pxTCB )->xStateListItem ) );


	/*定义延时列表
	定义两条，当第一条溢出的时候使用第二条
	*/
static List_t xDelayedTaskList1; 
static List_t xDelayedTaskList2; 
static List_t * volatile pxDelayedTaskList; 
static List_t * volatile pxOverflowDelayedTaskList; 


static volatile BaseType_t xNumOfOverflows = ( BaseType_t ) 0;

static volatile TickType_t xNextTaskUnblockTime = ( TickType_t ) 0U; /* Initialised to portMAX_DELAY before the scheduler starts.*/


	/*在就绪列表中查找最高优先级任务
    1、通用方法
    2、处理器架构优化后*/

/*通用方法*/
#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )
    /* uxTopReadyPriority 存的是就绪任务的最高优先级 */
    #define taskRECORD_READY_PRIORITY( uxPriority )                                          \
    {                                                                                        \
        if( ( uxPriority ) > uxTopReadyPriority )                                            \
        {                                                                                    \
            uxTopReadyPriority = ( uxPriority );                                             \
        }                                                                                    \
    }  /* taskRECORD_READY_PRIORITY */          
                
    #define taskSELECT_HIGHEST_PRIORITY_TASK()                                               \
    {                                                                                        \
        UBaseType_t uxTopPriority = uxTopReadyPriority;                                      \
        /* 寻找包含就绪任务的最高优先级的队列 */                                             \
        while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxTopPriority ] ) ) )                \
        {                                                                                    \
            --uxTopPriority;                                                                 \
        }                                                                                    \
        /* 获取优先级最高的就绪任务的 TCB，然后更新到 pxCurrentTCB */\
        listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB, &(pxReadyTasksLists[ uxTopPriority ]));    \
        /* 更新 uxTopReadyPriority */                                                        \
        uxTopReadyPriority = uxTopPriority;                                                  \
    } /* taskSELECT_HIGHEST_PRIORITY_TASK */
    
    
    
    /* 这两个宏定义只有在选择优化方法时才用，这里定义为空 */
	#if 1 /* 本章的实现方法 */
	#define taskRESET_READY_PRIORITY( uxPriority )																\
	{																											\
		if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ ( uxPriority ) ] ) ) == ( UBaseType_t ) 0 )			\
		{																										\
			portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) );									\
		}																										\
	}
	#else
		#define taskRESET_READY_PRIORITY( uxPriority )															\
		{																										\
			portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) );									\
		}
		#endif
    
    /* 查找最高优先级的就绪任务：根据处理器架构优化后的方法 */
#else /* configUSE_PORT_OPTIMISED_TASK_SELECTION */ 

    #define taskRECORD_READY_PRIORITY( uxPriority )     portRECORD_READY_PRIORITY( uxPriority, uxTopReadyPriority ) 
    
    
    
    #define taskSELECT_HIGHEST_PRIORITY_TASK()                                                  \
    {                                                                                           \
        UBaseType_t uxTopPriority;                                                              \
                                                                                                \
        /* Find the highest priority list that contains ready tasks. */                         \
        portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority );                          \
        listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );   \
    } /* taskSELECT_HIGHEST_PRIORITY_TASK() */
  #if 1
      #define taskRESET_READY_PRIORITY( uxPriority )                                            \
      {                                                                                         \
         if(listCURRENT_LIST_LENGTH(&(pxReadyTasksLists[( uxPriority)]))==(UBaseType_t)0)       \
        {                                                                                       \
            portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) );                 \
        }                                                                                       \
      }
  #else
      #define taskRESET_READY_PRIORITY( uxPriority )                                            \
      {                                                                                         \
        portRESET_READY_PRIORITY((uxPriority ), (uxTopReadyPriority));                          \
      }
#endif
    
#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

	  
	  
	  /*用于延时列表的切换、如果溢出就需交换*/
	  #define taskSWITCH_DELAYED_LISTS()														\
      {																							\
		List_t *pxTemp;																			\
		pxTemp = pxDelayedTaskList;																\
		pxDelayedTaskList = pxOverflowDelayedTaskList;											\
		pxOverflowDelayedTaskList = pxTemp;														\
		xNumOfOverflows++;																		\
		prvResetNextTaskUnblockTime();															\
      }


/*空闲任务函数声明*/
extern void prvIdleTask(void *p_arg);


static volatile UBaseType_t uxTopReadyPriority = tskIDLE_PRIORITY;
      

/*指向当前正在运行或者即将要运行的任务的任务控制块*/
TCB_t* pxCurrentTCB;

extern  TCB_t Task1TCB;
extern  TCB_t Task2TCB;

/*定义一个就绪列表*/
List_t pxReadyTasksLists[ configMAX_PRIORITIES ];

/*空闲任务*/
extern TCB_t IdleTaskTCB;
void vApplicationGetIdleTaskMemory( TCB_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize );
/*空闲任务的句柄*/
TaskHandle_t xIdleTaskHandle;


/*定义的全局变量在函数 vTaskStartScheduler()中调用 xPortStartScheduler()函数前初始化*/
uint32_t xTickCount;


/* 全局任务计时器*/
uint32_t uxCurrentNumberOfTasks  = 0 ;

/*插入延时列表*/
static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait )
{
    TickType_t xTimeToWake;             /*temp*/
    
    const TickType_t xConstTickCount = xTickCount;                  /*TickCount  temp */
    
    /*移除*/
    if ( uxListRemove( &( pxCurrentTCB->xStateListItem ) ) == ( UBaseType_t ) 0)
    {
        /*把任务优先级位图中对应的位清除*/
        portRESET_READY_PRIORITY( pxCurrentTCB->uxPriority,uxTopReadyPriority );
		
		
    }
	/*计算一下延时到期时，系统时基计数器是多少*/
	xTimeToWake = xConstTickCount + xTicksToWait;
	
	/*到期的时候设置为节点的排序值*/
	listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ),
							xTimeToWake );
	
	/*如果溢出那么就需要指向第二个延时列表*/
	if ( xTimeToWake < xConstTickCount )
	{
		vListInsert( pxOverflowDelayedTaskList,&( pxCurrentTCB->xStateListItem ) );
	}
	else				//如果没有溢出，那么还是指向第一个延时列表
	{
		vListInsert( pxDelayedTaskList,&( pxCurrentTCB->xStateListItem ) );
		
		
		
		/*更新下一个任务解锁时刻变量xNextTaskUnblockTime */
		xNextTaskUnblockTime  = xTimeToWake;
	}

}


/*Delay函数*/
void vTaskDelay( const TickType_t xTicksToDelay )
{
    TCB_t *pxTCB = NULL;

    /* 获取当前任务的 TCB */
    pxTCB = pxCurrentTCB; 

    /*将任务插入到延时列表中*/
    prvAddCurrentTaskToDelayedList( xTicksToDelay );
    

    /* 任务切换 */
    taskYIELD(); 
}

static void prvResetNextTaskUnblockTime( void )
{
	TCB_t *pxTCB;

	if ( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
	{
		/* 当前延时列表为空，则设置 xNextTaskUnblockTime 等于最大值 */
		xNextTaskUnblockTime = portMAX_DELAY;
	}
	else
	{
		/* 当前列表不为空，则有任务在延时，则获取当前列表下第一个节点的排序值
		然后将该节点的排序值更新到 xNextTaskUnblockTime */
		( pxTCB ) = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
		xNextTaskUnblockTime = listGET_LIST_ITEM_VALUE( &( ( pxTCB )->xStateListItem ) );
	}
}


/*更新系统时钟的函数*/
BaseType_t xTaskIncrementTick( void )
{
    TCB_t *pxTCB = NULL;
	TickType_t xItemValue;
    BaseType_t xSwitchRequired = pdFALSE;						//用来存储函数返回值
    
    /* 更新系统时基计数器 xTickCount */
    const TickType_t xConstTickCount = xTickCount + 1;
    xTickCount = xConstTickCount;
	
	
	/*如果溢出，那么就需要切换延时列表*/
	if(xConstTickCount == ( TickType_t ) 0U )
	{
		taskSWITCH_DELAYED_LISTS();							//切换延时列表
	}
	
	
	
	/*最近的任务的延时是否到期*/
	if(	xConstTickCount >= xNextTaskUnblockTime )
	{
		for(;;)
		{
			if(listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE)				//如果延时列表为空。那么设置xNextTaskUnblockTime 设为最大值
			{
				xNextTaskUnblockTime = portMAX_DELAY;
				break;
			}
			else
			{
				/*取出第一个延时函数的排序辅助值*/
				pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
				xItemValue = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) );
				
				
				/*如果延时列表中所有延时到期的函数移除才跳出for循环*/
				if(	xConstTickCount < xItemValue)
				{
					xNextTaskUnblockTime = xItemValue;
					break;
				}
				
				/* 将任务从延时列表移除，消除等待状态	*/
				( void ) uxListRemove( &( pxTCB->xStateListItem ) );
				
				/*把上面从延时列表移除的任务添加到就绪列表中*/
				prvAddTaskToReadyList( pxTCB ); 
				
				
				#if (configUSE_PREEMPTION == 1)
				{
					if ( pxTCB->uxPriority >= pxCurrentTCB->uxPriority )
					{
						xSwitchRequired = pdTRUE;
					}
				}
				#endif
				
	
			}
		}
	}
	
	
	#if ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) ) 
	{
		if ( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ pxCurrentTCB->uxPriority ] ) )> ( UBaseType_t ) 1 )
		{
			xSwitchRequired = pdTRUE;
		}
	}
	#endif /* ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) ) */
	
   
 }

/*手动打开任务切换的方式，如果只需要仿真，那么就定义1，开启下面的函数进行仿真测试
    该函数主要是为了进行任务的切换
    下面函数添加了空闲任务，进行任务一，任务二，空闲任务的切换
    使用第一个函数时需要将任务调度函数中的空闲函数屏蔽，只需要手动选择第一个任务再开启任务调度*/
#if 1
void vTaskSwitchContext( void )
{
   /* 获取优先级最高的就绪任务的 TCB，然后更新到 pxCurrentTCB */
    taskSELECT_HIGHEST_PRIORITY_TASK();
}
#else 

void vTaskSwitchContext( void )
{
        /* 如果当前任务是空闲任务，那么就去尝试执行任务 1 或者任务 2，
           看看他们的延时时间是否结束，如果任务的延时时间均没有到期，
           那就返回继续执行空闲任务 */
    if ( pxCurrentTCB == &IdleTaskTCB ) 
    {
        if (Task1TCB.xTicksToDelay == 0)
        {
        pxCurrentTCB =&Task1TCB;
        }
        else if (Task2TCB.xTicksToDelay == 0)
        {
            pxCurrentTCB =&Task2TCB;
        }
        else
        {
            return;                             /* 任务延时均没有到期则返回，继续执行空闲任务 */
        }
    }
    else /* 当前任务不是空闲任务则会执行到这里 */
    {
        /*如果当前任务是任务 1 或者任务 2 的话，检查下另外一个任务,
        如果另外的任务不在延时中，就切换到该任务
        否则，判断下当前任务是否应该进入延时状态，
        如果是的话，就切换到空闲任务。否则就不进行任何切换 */
        if (pxCurrentTCB == &Task1TCB)
        {
            if (Task2TCB.xTicksToDelay == 0)
            {
            pxCurrentTCB =&Task2TCB;
            }
            else if (pxCurrentTCB->xTicksToDelay != 0)
            {
                pxCurrentTCB = &IdleTaskTCB;
            }
            else
            {
                return; /* 返回，不进行切换，因为两个任务都处于延时中 */
            }
        }
        else if (pxCurrentTCB == &Task2TCB)
        {
            if (Task1TCB.xTicksToDelay == 0)
            {
                pxCurrentTCB =&Task1TCB;
            }
            else if (pxCurrentTCB->xTicksToDelay != 0)
            {
                pxCurrentTCB = &IdleTaskTCB;
            }
            else
            {
                return; /* 返回，不进行切换，因为两个任务都处于延时中 */
            }
        }
    }
}

#endif



/*就绪列表的初始化*/
void prvInitialiseTaskLists( void )
{
     UBaseType_t uxPriority;

    for( uxPriority = ( UBaseType_t ) 0U; uxPriority < ( UBaseType_t ) configMAX_PRIORITIES; uxPriority++ )
    {
        vListInitialise( &( pxReadyTasksLists[ uxPriority ] ) );
    }
    
    
    /*初始化延时列表*/
    vListInitialise(&xDelayedTaskList1);
    vListInitialise(&xDelayedTaskList2);
    /*分别指向第一第二条延时列表*/
    pxDelayedTaskList = &xDelayedTaskList1;
    pxOverflowDelayedTaskList = &xDelayedTaskList2;
}



static void prvInitialiseNewTask(TaskFunction_t pxTaskCode,
                                const char * const pcName, 
                                const uint32_t ulStackDepth, 
                                void * const pvParameters, 
                                UBaseType_t uxPriority,
                                TaskHandle_t * const pxCreatedTask, 
                                TCB_t *pxNewTCB ) 
{
    StackType_t *pxTopOfStack;
    UBaseType_t x;
    
    /* 获取栈顶地址 */ 
    pxTopOfStack = pxNewTCB->pxStack + ( ulStackDepth - ( uint32_t ) 1 );
    /* 向下做 8 字节对齐 */ 
    pxTopOfStack = ( StackType_t * ) \
    ( ( ( uint32_t ) pxTopOfStack ) & ( ~( ( uint32_t ) 0x0007 ) ) );
    
    /* 将任务的名字存储在 TCB 中 */ 
    for ( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
    {
        pxNewTCB->pcTaskName[ x ] = pcName[ x ];
    
    if ( pcName[ x ] == 0x00 )
    {
        break;
    }
    }
    /* 任务名字的长度不能超过 configMAX_TASK_NAME_LEN */ 
    pxNewTCB->pcTaskName[ configMAX_TASK_NAME_LEN - 1 ] = '\0';
    
    /* 初始化 TCB 中的 xStateListItem 节点 */ 
    vListInitialiseItem( &( pxNewTCB->xStateListItem ) );
    /* 设置 xStateListItem 节点的拥有者 */ 
    listSET_LIST_ITEM_OWNER( &( pxNewTCB->xStateListItem ), pxNewTCB );
    
    /* 初始化优先级 */
    if ( uxPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
    {
        uxPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
    }
    pxNewTCB->uxPriority = uxPriority;   
    

    /* 初始化任务栈 */ 
    pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack,
                                                    pxTaskCode,
                                                    pvParameters );
    
    
    /* 让任务句柄指向任务控制块 */ 
    if ( ( void * ) pxCreatedTask != NULL )
    {
        *pxCreatedTask = ( TaskHandle_t ) pxNewTCB;
    }
}




static void prvAddNewTaskToReadyList( TCB_t *pxNewTCB )
{
    /* 进入临界段 */
    taskENTER_CRITICAL();
    {
        /* 全局任务计时器加一操作 */
        uxCurrentNumberOfTasks++; 

        /* 如果 pxCurrentTCB 为空，则将 pxCurrentTCB 指向新创建的任务 */
        if ( pxCurrentTCB == NULL ) 
        {
            pxCurrentTCB = pxNewTCB;

            /* 如果是第一次创建任务，则需要初始化任务相关的列表 */
            if ( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 ) 
            {
                /* 初始化任务相关的列表 */
                prvInitialiseTaskLists();
            }
        }
        else /* 如果 pxCurrentTCB 不为空，
        则根据任务的优先级将 pxCurrentTCB 指向最高优先级任务的 TCB */
        {
            if ( pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority )
            {
            pxCurrentTCB = pxNewTCB;
            }
        }
    
        /* 将任务添加到就绪列表 */
        prvAddTaskToReadyList( pxNewTCB );
    
        }
        /* 退出临界段 */
        taskEXIT_CRITICAL();
}




#if( configSUPPORT_STATIC_ALLOCATION == 1 )
                    /*任务创建函数*/
TaskHandle_t xTaskCreateStatic( TaskFunction_t pxTaskCode,
                                    const char * const pcName, 
                                    const uint32_t ulStackDepth, 
                                    void * const pvParameters, 
                                    UBaseType_t uxPriority,
                                    StackType_t * const puxStackBuffer, 
                                    TCB_t * const pxTaskBuffer ) 
{
    TCB_t *pxNewTCB;
    TaskHandle_t xReturn; 
    
    if ( ( pxTaskBuffer != NULL ) && ( puxStackBuffer != NULL ) )
    {
        pxNewTCB = ( TCB_t * ) pxTaskBuffer;
        pxNewTCB->pxStack = ( StackType_t * ) puxStackBuffer;
    
        /* 创建新的任务 */ 
        prvInitialiseNewTask( pxTaskCode,
                            pcName,
                            ulStackDepth,
                            pvParameters,
                            uxPriority,
                            &xReturn,
                            pxNewTCB);
        
        /* 将任务添加到就绪列表 */ 
        prvAddNewTaskToReadyList( pxNewTCB );
    }
    else
    {
        xReturn = NULL;
    }

    /* 返回任务句柄，如果任务创建成功，此时 xReturn 应该指向任务控制块 */
    return xReturn; 
}

portCHAR flag4;
    
/*空闲任务*/
void prvIdleTask(void *p_arg)
{
    while(1)
    {
    }
}



void vTaskStartScheduler( void )
{
    
    /*空闲任务*/
    TCB_t *pxIdleTaskTCBBuffer = NULL; /* 用于指向空闲任务控制块 */
    StackType_t *pxIdleTaskStackBuffer = NULL; /* 用于空闲任务栈起始地址 */
    uint32_t ulIdleTaskStackSize;
    
    /* 获取空闲任务的内存：任务栈和任务 TCB */
    vApplicationGetIdleTaskMemory( &pxIdleTaskTCBBuffer,
                                   &pxIdleTaskStackBuffer,
                                   &ulIdleTaskStackSize );
    
    
    /* 创建空闲任务 */ 
    xIdleTaskHandle = xTaskCreateStatic( (TaskFunction_t)prvIdleTask, /* 任务入口 */
                                         (char *)"IDLE", /* 任务名称，字符串形式 */
                                         (uint32_t)ulIdleTaskStackSize , /* 任务栈大小，单位为字 */
                                         (void *) NULL, /* 任务形参 */
                                         (UBaseType_t) tskIDLE_PRIORITY,       /*优先级*/
                                         (StackType_t *)pxIdleTaskStackBuffer, /* 任务栈起始地址 */
                                         (TCB_t *)pxIdleTaskTCBBuffer ); /* 任务控制块 */
            

      
			/* 手动将任务添加到就绪列表  手动指定第一个运行的任务*/
     /*
     vListInsertEnd( &( pxReadyTasksLists[0] ),
                     &( ((TCB_t *)pxIdleTaskTCBBuffer)->xStateListItem ) );
    
     pxCurrentTCB = &Task1TCB;
      */
       

       /*表示下一个任务需要解锁的时间，当xNextTaskUnblockTime的值和系统时基计数器xTickCount相等的时候表示任务延时已经到期需要将任务就绪*/                                  
       xNextTaskUnblockTime = portMAX_DELAY;
       
    
      /*初始化xTickCount*/
      xTickCount = ( TickType_t ) 0U;
	  
     /* 启动调度器 */
     if ( xPortStartScheduler() != pdFALSE )
     {
         /* 调度器启动成功，则不会返回，即不会来到这里 */ 
     }
 }



#endif /* configSUPPORT_STATIC_ALLOCATION */



