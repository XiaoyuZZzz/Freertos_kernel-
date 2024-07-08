#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H


#define configUSE_16_BIT_TICKS 0
#define configMAX_TASK_NAME_LEN 16

#define configSUPPORT_STATIC_ALLOCATION                 1                  //���þ�̬�������Ƕ�̬����
#define configMAX_PRIORITIES                            5                 //���þ����б�Ĵ�С��Freertos��֧��256�����ȼ���

/*�޸��ж�����Ӧ*/
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
#define vPortSVCHandler SVC_Handler


#define configMAX_SYSCALL_INTERRUPT_PRIORITY            191                //���������ж����μĴ��� BASEPRI ��ֵ������λ��Ч�������Ը���basepri���ж���Ч


#define configTICK_RATE_HZ                      ( ( TickType_t ) 100 )

#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 25000000 )
	

#define configUSE_PREEMPTION  							1				//�����������ȵ�ǰ�������ȼ�����Ҫ����һ�������л�
    
/*
#define configCPU_CLOCK_HZ (( unsigned long ) 75000000)
#define configTICK_RATE_HZ (( TickType_t ) 1000)
*/

#endif /* FREERTOS_CONFIG_H */
