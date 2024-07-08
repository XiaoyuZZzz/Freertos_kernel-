#include "portmacro.h"
#include "projdefs.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"




/* SysTick ʱ��Դѡ�� */
#ifndef configSYSTICK_CLOCK_HZ
    #define configSYSTICK_CLOCK_HZ      configCPU_CLOCK_HZ
    /* Ensure the SysTick is clocked at the same frequency as the core. */
    #define portNVIC_SYSTICK_CLK_BIT    ( 1UL << 2UL )
#else
/* The way the SysTick is clocked is not modified in case it is not the same
 * as the core. */
    #define portNVIC_SYSTICK_CLK_BIT    ( 0 )
#endif


/*�ڵ���֮ǰ���ʼ��0*/
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

#define portINITIAL_XPSR ( 0x01000000 )
#define portSTART_ADDRESS_MASK ( ( StackType_t ) 0xfffffffeUL )

/*
 * �ο����ϡ�STM32F10xxx Cortex-M3 programming manual��
 * �� Cortex-M �У��ں����� SCB �� SHPR3 �Ĵ����������� SysTick �� PendSV ���쳣���ȼ�
 * System handler priority register 3 (SCB_SHPR3) SCB_SHPR3�� 0xE000 ED20
 * Bits 31:24 PRI_15[7:0]: Priority of system handler 15, SysTick exception
 * Bits 23:16 PRI_14[7:0]: Priority of system handler 14, PendSV
 */
#define configKERNEL_INTERRUPT_PRIORITY                 255

#define portNVIC_SYSPRI2_REG (*(( volatile uint32_t *) 0xe000ed20))

#define portNVIC_PENDSV_PRI (((uint32_t) configKERNEL_INTERRUPT_PRIORITY ) << 16UL)
#define portNVIC_SYSTICK_PRI (((uint32_t) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )

/*SysTick��ʼ��*/
/* SysTick ���ƼĴ��� */ 
#define portNVIC_SYSTICK_CTRL_REG (*((volatile uint32_t *) 0xe000e010 ))
/* SysTick ��װ�ؼĴ����Ĵ��� */
#define portNVIC_SYSTICK_LOAD_REG (*((volatile uint32_t *) 0xe000e014 ))

#define portNVIC_SYSTICK_INT_BIT ( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT ( 1UL << 0UL )

#define portVECTACTIVE_MASK                   ( 0xFFUL )

static void prvTaskExitError( void )
{
    /* ����ֹͣ������ */
        for (;;);
}

StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack,
                                    TaskFunction_t pxCode,
                                    void *pvParameters )
{
    /* �쳣����ʱ���Զ����ص� CPU �Ĵ��������� */ 
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_XPSR; 
    pxTopOfStack--;
    *pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) prvTaskExitError; 
    pxTopOfStack -= 5; /* R12, R3, R2 and R1 Ĭ�ϳ�ʼ��Ϊ 0 */
    *pxTopOfStack = ( StackType_t ) pvParameters; 

    /* �쳣����ʱ���ֶ����ص� CPU �Ĵ��������� */ 
    pxTopOfStack -= 8;

    /* ����ջ��ָ�룬��ʱ pxTopOfStack ָ�����ջ */
    return pxTopOfStack; 
}


void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
    uxCriticalNesting++; 
    if ( uxCriticalNesting == 1 ) 
    {
        configASSERT( ( portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK ) == 0 );
    }
}

void vPortExitCritical( void )
{
    configASSERT( uxCriticalNesting );
    uxCriticalNesting--;

    if( uxCriticalNesting == 0 )
    {
        portENABLE_INTERRUPTS();
    }
}

void vPortSetupTimerInterrupt( void )
{
    
     /* Stop and clear the SysTick. */
     portNVIC_SYSTICK_CTRL_REG = 0UL;
    
    /* ������װ�ؼĴ�����ֵ */ 
    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    
    /* ����ϵͳ��ʱ����ʱ�ӵ����ں�ʱ��       ʹ�� SysTick ��ʱ���ж�      ʹ�� SysTick ��ʱ�� */
    portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
}



/*SysTick�жϺ�������С��һ��ʱ�䵥λ����һ���ж����ھ���һ��tick*/
void xPortSysTickHandler( void )
{
    /* ���ж� */
    vPortRaiseBASEPRI(); 
    
    if(xTaskIncrementTick() != pdFALSE)
	{
		taskYIELD();
	}
     
    
    /* ���ж� */
    vPortClearBASEPRIFromISR(); 
}


/*�����л��ĵط�*/
__asm void xPortPendSVHandler( void )
{
    extern pxCurrentTCB; 
    extern vTaskSwitchContext; 
    
    PRESERVE8 
    
    mrs r0, psp 
    isb
    
    ldr r3, =pxCurrentTCB 
    ldr r2, [r3] 
    
    stmdb r0!, {r4-r11} 
    str r0, [r2] 
    
    stmdb sp!, {r3, r14} 
    mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY 
    msr basepri, r0 
    dsb
    isb
    bl vTaskSwitchContext 
    mov r0, #0 
    msr basepri, r0
    ldmia sp!, {r3, r14} 
    
    ldr r1, [r3] 
    ldr r0, [r1] 
    ldmia r0!, {r4-r11} 
    msr psp, r0 
    isb
    bx r14 
    nop
}

__asm void vPortSVCHandler( void )
{
    extern pxCurrentTCB; 

    PRESERVE8

    ldr r3, =pxCurrentTCB 
    ldr r1, [r3] 
    ldr r0, [r1] 
    ldmia r0!, {r4-r11} 
    msr psp, r0 
    isb
    mov r0, #0 
    msr basepri, r0 
    orr r14, #0xd 

    bx r14 
}

/*
* �ο����ϡ�STM32F10xxx Cortex-M3 programming manual�� 4.4.3��
* �� Cortex-M �У��ں����� SCB �ĵ�ַ��ΧΪ�� 0xE000ED00-0xE000ED3F
* 0xE000ED008 Ϊ SCB ������ SCB_VTOR ����Ĵ����ĵ�ַ�������ŵ������������ʼ��ַ���� MSP �ĵ�ַ
*/
__asm void prvStartFirstTask(void)
{
    PRESERVE8 

    /* �� Cortex-M �У� 0xE000ED08 �� SCB_VTOR ����Ĵ����ĵ�ַ��
    �����ŵ������������ʼ��ַ���� MSP �ĵ�ַ */
    ldr r0, =0xE000ED08 
    ldr r0, [r0] 
    ldr r0, [r0] 

    /* ��������ջָ�� msp ��ֵ */
    msr msp, r0 

    /* ʹ��ȫ���ж� */ 
    cpsie i
    cpsie f
    dsb
    isb

    /* ���� SVC ȥ������һ������ */
    svc 0 
    nop
    nop
 }



BaseType_t xPortStartScheduler( void )
{
    
    /* ���� PendSV �� SysTick ���ж����ȼ�Ϊ��� */ 
    portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;

    /* ��ʼ�� SysTick */
    vPortSetupTimerInterrupt();
    
   /* Initialise the critical nesting count ready for the first task. */
    uxCriticalNesting = 0;
    
    /* ������һ�����񣬲��ٷ��� */
    prvStartFirstTask(); 

    /* ��Ӧ�����е����� */
    return 0;
}
