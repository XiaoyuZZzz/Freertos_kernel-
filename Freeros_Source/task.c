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


	/*������ʱ�б�
	��������������һ�������ʱ��ʹ�õڶ���
	*/
static List_t xDelayedTaskList1; 
static List_t xDelayedTaskList2; 
static List_t * volatile pxDelayedTaskList; 
static List_t * volatile pxOverflowDelayedTaskList; 


static volatile BaseType_t xNumOfOverflows = ( BaseType_t ) 0;

static volatile TickType_t xNextTaskUnblockTime = ( TickType_t ) 0U; /* Initialised to portMAX_DELAY before the scheduler starts.*/


	/*�ھ����б��в���������ȼ�����
    1��ͨ�÷���
    2���������ܹ��Ż���*/

/*ͨ�÷���*/
#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )
    /* uxTopReadyPriority ����Ǿ��������������ȼ� */
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
        /* Ѱ�Ұ������������������ȼ��Ķ��� */                                             \
        while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxTopPriority ] ) ) )                \
        {                                                                                    \
            --uxTopPriority;                                                                 \
        }                                                                                    \
        /* ��ȡ���ȼ���ߵľ�������� TCB��Ȼ����µ� pxCurrentTCB */\
        listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB, &(pxReadyTasksLists[ uxTopPriority ]));    \
        /* ���� uxTopReadyPriority */                                                        \
        uxTopReadyPriority = uxTopPriority;                                                  \
    } /* taskSELECT_HIGHEST_PRIORITY_TASK */
    
    
    
    /* �������궨��ֻ����ѡ���Ż�����ʱ���ã����ﶨ��Ϊ�� */
	#if 1 /* ���µ�ʵ�ַ��� */
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
    
    /* ����������ȼ��ľ������񣺸��ݴ������ܹ��Ż���ķ��� */
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

	  
	  
	  /*������ʱ�б���л������������轻��*/
	  #define taskSWITCH_DELAYED_LISTS()														\
      {																							\
		List_t *pxTemp;																			\
		pxTemp = pxDelayedTaskList;																\
		pxDelayedTaskList = pxOverflowDelayedTaskList;											\
		pxOverflowDelayedTaskList = pxTemp;														\
		xNumOfOverflows++;																		\
		prvResetNextTaskUnblockTime();															\
      }


/*��������������*/
extern void prvIdleTask(void *p_arg);


static volatile UBaseType_t uxTopReadyPriority = tskIDLE_PRIORITY;
      

/*ָ��ǰ�������л��߼���Ҫ���е������������ƿ�*/
TCB_t* pxCurrentTCB;

extern  TCB_t Task1TCB;
extern  TCB_t Task2TCB;

/*����һ�������б�*/
List_t pxReadyTasksLists[ configMAX_PRIORITIES ];

/*��������*/
extern TCB_t IdleTaskTCB;
void vApplicationGetIdleTaskMemory( TCB_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize );
/*��������ľ��*/
TaskHandle_t xIdleTaskHandle;


/*�����ȫ�ֱ����ں��� vTaskStartScheduler()�е��� xPortStartScheduler()����ǰ��ʼ��*/
uint32_t xTickCount;


/* ȫ�������ʱ��*/
uint32_t uxCurrentNumberOfTasks  = 0 ;

/*������ʱ�б�*/
static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait )
{
    TickType_t xTimeToWake;             /*temp*/
    
    const TickType_t xConstTickCount = xTickCount;                  /*TickCount  temp */
    
    /*�Ƴ�*/
    if ( uxListRemove( &( pxCurrentTCB->xStateListItem ) ) == ( UBaseType_t ) 0)
    {
        /*���������ȼ�λͼ�ж�Ӧ��λ���*/
        portRESET_READY_PRIORITY( pxCurrentTCB->uxPriority,uxTopReadyPriority );
		
		
    }
	/*����һ����ʱ����ʱ��ϵͳʱ���������Ƕ���*/
	xTimeToWake = xConstTickCount + xTicksToWait;
	
	/*���ڵ�ʱ������Ϊ�ڵ������ֵ*/
	listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ),
							xTimeToWake );
	
	/*��������ô����Ҫָ��ڶ�����ʱ�б�*/
	if ( xTimeToWake < xConstTickCount )
	{
		vListInsert( pxOverflowDelayedTaskList,&( pxCurrentTCB->xStateListItem ) );
	}
	else				//���û���������ô����ָ���һ����ʱ�б�
	{
		vListInsert( pxDelayedTaskList,&( pxCurrentTCB->xStateListItem ) );
		
		
		
		/*������һ���������ʱ�̱���xNextTaskUnblockTime */
		xNextTaskUnblockTime  = xTimeToWake;
	}

}


/*Delay����*/
void vTaskDelay( const TickType_t xTicksToDelay )
{
    TCB_t *pxTCB = NULL;

    /* ��ȡ��ǰ����� TCB */
    pxTCB = pxCurrentTCB; 

    /*��������뵽��ʱ�б���*/
    prvAddCurrentTaskToDelayedList( xTicksToDelay );
    

    /* �����л� */
    taskYIELD(); 
}

static void prvResetNextTaskUnblockTime( void )
{
	TCB_t *pxTCB;

	if ( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
	{
		/* ��ǰ��ʱ�б�Ϊ�գ������� xNextTaskUnblockTime �������ֵ */
		xNextTaskUnblockTime = portMAX_DELAY;
	}
	else
	{
		/* ��ǰ�б�Ϊ�գ�������������ʱ�����ȡ��ǰ�б��µ�һ���ڵ������ֵ
		Ȼ�󽫸ýڵ������ֵ���µ� xNextTaskUnblockTime */
		( pxTCB ) = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
		xNextTaskUnblockTime = listGET_LIST_ITEM_VALUE( &( ( pxTCB )->xStateListItem ) );
	}
}


/*����ϵͳʱ�ӵĺ���*/
BaseType_t xTaskIncrementTick( void )
{
    TCB_t *pxTCB = NULL;
	TickType_t xItemValue;
    BaseType_t xSwitchRequired = pdFALSE;						//�����洢��������ֵ
    
    /* ����ϵͳʱ�������� xTickCount */
    const TickType_t xConstTickCount = xTickCount + 1;
    xTickCount = xConstTickCount;
	
	
	/*����������ô����Ҫ�л���ʱ�б�*/
	if(xConstTickCount == ( TickType_t ) 0U )
	{
		taskSWITCH_DELAYED_LISTS();							//�л���ʱ�б�
	}
	
	
	
	/*������������ʱ�Ƿ���*/
	if(	xConstTickCount >= xNextTaskUnblockTime )
	{
		for(;;)
		{
			if(listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE)				//�����ʱ�б�Ϊ�ա���ô����xNextTaskUnblockTime ��Ϊ���ֵ
			{
				xNextTaskUnblockTime = portMAX_DELAY;
				break;
			}
			else
			{
				/*ȡ����һ����ʱ������������ֵ*/
				pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
				xItemValue = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) );
				
				
				/*�����ʱ�б���������ʱ���ڵĺ����Ƴ�������forѭ��*/
				if(	xConstTickCount < xItemValue)
				{
					xNextTaskUnblockTime = xItemValue;
					break;
				}
				
				/* ���������ʱ�б��Ƴ��������ȴ�״̬	*/
				( void ) uxListRemove( &( pxTCB->xStateListItem ) );
				
				/*���������ʱ�б��Ƴ���������ӵ������б���*/
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

/*�ֶ��������л��ķ�ʽ�����ֻ��Ҫ���棬��ô�Ͷ���1����������ĺ������з������
    �ú�����Ҫ��Ϊ�˽���������л�
    ���溯������˿������񣬽�������һ�������������������л�
    ʹ�õ�һ������ʱ��Ҫ��������Ⱥ����еĿ��к������Σ�ֻ��Ҫ�ֶ�ѡ���һ�������ٿ����������*/
#if 1
void vTaskSwitchContext( void )
{
   /* ��ȡ���ȼ���ߵľ�������� TCB��Ȼ����µ� pxCurrentTCB */
    taskSELECT_HIGHEST_PRIORITY_TASK();
}
#else 

void vTaskSwitchContext( void )
{
        /* �����ǰ�����ǿ���������ô��ȥ����ִ������ 1 �������� 2��
           �������ǵ���ʱʱ���Ƿ����������������ʱʱ���û�е��ڣ�
           �Ǿͷ��ؼ���ִ�п������� */
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
            return;                             /* ������ʱ��û�е����򷵻أ�����ִ�п������� */
        }
    }
    else /* ��ǰ�����ǿ����������ִ�е����� */
    {
        /*�����ǰ���������� 1 �������� 2 �Ļ������������һ������,
        ����������������ʱ�У����л���������
        �����ж��µ�ǰ�����Ƿ�Ӧ�ý�����ʱ״̬��
        ����ǵĻ������л����������񡣷���Ͳ������κ��л� */
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
                return; /* ���أ��������л�����Ϊ�������񶼴�����ʱ�� */
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
                return; /* ���أ��������л�����Ϊ�������񶼴�����ʱ�� */
            }
        }
    }
}

#endif



/*�����б�ĳ�ʼ��*/
void prvInitialiseTaskLists( void )
{
     UBaseType_t uxPriority;

    for( uxPriority = ( UBaseType_t ) 0U; uxPriority < ( UBaseType_t ) configMAX_PRIORITIES; uxPriority++ )
    {
        vListInitialise( &( pxReadyTasksLists[ uxPriority ] ) );
    }
    
    
    /*��ʼ����ʱ�б�*/
    vListInitialise(&xDelayedTaskList1);
    vListInitialise(&xDelayedTaskList2);
    /*�ֱ�ָ���һ�ڶ�����ʱ�б�*/
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
    
    /* ��ȡջ����ַ */ 
    pxTopOfStack = pxNewTCB->pxStack + ( ulStackDepth - ( uint32_t ) 1 );
    /* ������ 8 �ֽڶ��� */ 
    pxTopOfStack = ( StackType_t * ) \
    ( ( ( uint32_t ) pxTopOfStack ) & ( ~( ( uint32_t ) 0x0007 ) ) );
    
    /* ����������ִ洢�� TCB �� */ 
    for ( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
    {
        pxNewTCB->pcTaskName[ x ] = pcName[ x ];
    
    if ( pcName[ x ] == 0x00 )
    {
        break;
    }
    }
    /* �������ֵĳ��Ȳ��ܳ��� configMAX_TASK_NAME_LEN */ 
    pxNewTCB->pcTaskName[ configMAX_TASK_NAME_LEN - 1 ] = '\0';
    
    /* ��ʼ�� TCB �е� xStateListItem �ڵ� */ 
    vListInitialiseItem( &( pxNewTCB->xStateListItem ) );
    /* ���� xStateListItem �ڵ��ӵ���� */ 
    listSET_LIST_ITEM_OWNER( &( pxNewTCB->xStateListItem ), pxNewTCB );
    
    /* ��ʼ�����ȼ� */
    if ( uxPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
    {
        uxPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
    }
    pxNewTCB->uxPriority = uxPriority;   
    

    /* ��ʼ������ջ */ 
    pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack,
                                                    pxTaskCode,
                                                    pvParameters );
    
    
    /* ��������ָ��������ƿ� */ 
    if ( ( void * ) pxCreatedTask != NULL )
    {
        *pxCreatedTask = ( TaskHandle_t ) pxNewTCB;
    }
}




static void prvAddNewTaskToReadyList( TCB_t *pxNewTCB )
{
    /* �����ٽ�� */
    taskENTER_CRITICAL();
    {
        /* ȫ�������ʱ����һ���� */
        uxCurrentNumberOfTasks++; 

        /* ��� pxCurrentTCB Ϊ�գ��� pxCurrentTCB ָ���´��������� */
        if ( pxCurrentTCB == NULL ) 
        {
            pxCurrentTCB = pxNewTCB;

            /* ����ǵ�һ�δ�����������Ҫ��ʼ��������ص��б� */
            if ( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 ) 
            {
                /* ��ʼ��������ص��б� */
                prvInitialiseTaskLists();
            }
        }
        else /* ��� pxCurrentTCB ��Ϊ�գ�
        �������������ȼ��� pxCurrentTCB ָ��������ȼ������ TCB */
        {
            if ( pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority )
            {
            pxCurrentTCB = pxNewTCB;
            }
        }
    
        /* ��������ӵ������б� */
        prvAddTaskToReadyList( pxNewTCB );
    
        }
        /* �˳��ٽ�� */
        taskEXIT_CRITICAL();
}




#if( configSUPPORT_STATIC_ALLOCATION == 1 )
                    /*���񴴽�����*/
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
    
        /* �����µ����� */ 
        prvInitialiseNewTask( pxTaskCode,
                            pcName,
                            ulStackDepth,
                            pvParameters,
                            uxPriority,
                            &xReturn,
                            pxNewTCB);
        
        /* ��������ӵ������б� */ 
        prvAddNewTaskToReadyList( pxNewTCB );
    }
    else
    {
        xReturn = NULL;
    }

    /* ������������������񴴽��ɹ�����ʱ xReturn Ӧ��ָ��������ƿ� */
    return xReturn; 
}

portCHAR flag4;
    
/*��������*/
void prvIdleTask(void *p_arg)
{
    while(1)
    {
    }
}



void vTaskStartScheduler( void )
{
    
    /*��������*/
    TCB_t *pxIdleTaskTCBBuffer = NULL; /* ����ָ�����������ƿ� */
    StackType_t *pxIdleTaskStackBuffer = NULL; /* ���ڿ�������ջ��ʼ��ַ */
    uint32_t ulIdleTaskStackSize;
    
    /* ��ȡ����������ڴ棺����ջ������ TCB */
    vApplicationGetIdleTaskMemory( &pxIdleTaskTCBBuffer,
                                   &pxIdleTaskStackBuffer,
                                   &ulIdleTaskStackSize );
    
    
    /* ������������ */ 
    xIdleTaskHandle = xTaskCreateStatic( (TaskFunction_t)prvIdleTask, /* ������� */
                                         (char *)"IDLE", /* �������ƣ��ַ�����ʽ */
                                         (uint32_t)ulIdleTaskStackSize , /* ����ջ��С����λΪ�� */
                                         (void *) NULL, /* �����β� */
                                         (UBaseType_t) tskIDLE_PRIORITY,       /*���ȼ�*/
                                         (StackType_t *)pxIdleTaskStackBuffer, /* ����ջ��ʼ��ַ */
                                         (TCB_t *)pxIdleTaskTCBBuffer ); /* ������ƿ� */
            

      
			/* �ֶ���������ӵ������б�  �ֶ�ָ����һ�����е�����*/
     /*
     vListInsertEnd( &( pxReadyTasksLists[0] ),
                     &( ((TCB_t *)pxIdleTaskTCBBuffer)->xStateListItem ) );
    
     pxCurrentTCB = &Task1TCB;
      */
       

       /*��ʾ��һ��������Ҫ������ʱ�䣬��xNextTaskUnblockTime��ֵ��ϵͳʱ��������xTickCount��ȵ�ʱ���ʾ������ʱ�Ѿ�������Ҫ���������*/                                  
       xNextTaskUnblockTime = portMAX_DELAY;
       
    
      /*��ʼ��xTickCount*/
      xTickCount = ( TickType_t ) 0U;
	  
     /* ���������� */
     if ( xPortStartScheduler() != pdFALSE )
     {
         /* �����������ɹ����򲻻᷵�أ��������������� */ 
     }
 }



#endif /* configSUPPORT_STATIC_ALLOCATION */



