#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H


#define configUSE_16_BIT_TICKS 0
#define configMAX_TASK_NAME_LEN 16

#define configSUPPORT_STATIC_ALLOCATION                 1                  //配置静态创建还是动态创建
#define configMAX_PRIORITIES                            5                 //配置就绪列表的大小，Freertos是支持256个优先级的

/*修改中断名响应*/
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
#define vPortSVCHandler SVC_Handler


#define configMAX_SYSCALL_INTERRUPT_PRIORITY            191                //用来配置中断屏蔽寄存器 BASEPRI 的值（高四位有效），所以高于basepri的中断无效


#define configTICK_RATE_HZ                      ( ( TickType_t ) 100 )

#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 25000000 )
	

#define configUSE_PREEMPTION  							1				//如果就绪任务比当前任务优先级高则要进行一次任务切换
    
/*
#define configCPU_CLOCK_HZ (( unsigned long ) 75000000)
#define configTICK_RATE_HZ (( TickType_t ) 1000)
*/

#endif /* FREERTOS_CONFIG_H */
