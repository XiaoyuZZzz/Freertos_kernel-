#ifndef PORTMACRO_H
#define PORTMACRO_H

#include "stdint.h"
#include "stddef.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"




    /* 数据类型重定义 */
    #define portCHAR char
    #define portFLOAT float
    #define portDOUBLE double
    #define portLONG long
    #define portSHORT short
    #define portSTACK_TYPE uint32_t
    #define portBASE_TYPE long

    typedef portSTACK_TYPE StackType_t;
    typedef long BaseType_t;
    typedef unsigned long UBaseType_t;

    #if( configUSE_16_BIT_TICKS == 1 )
    typedef uint16_t TickType_t;
    #define portMAX_DELAY ( TickType_t ) 0xffff
    #else
    typedef uint32_t TickType_t;
    #define portMAX_DELAY ( TickType_t ) 0xffffffffUL
    #endif
    #define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
    
    #define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )
    
    #define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31UL - ( uint32_t ) __clz( ( uxReadyPriorities ) ) )
    
    #define configUSE_PORT_OPTIMISED_TASK_SELECTION     1
    



 /*进入临界段不保护不可嵌套*/
#define portENTER_CRITICAL() vPortEnterCritical()
#define portEXIT_CRITICAL()  vPortExitCritical()

/* 不带中断保护的开中断函数 */
#define portENABLE_INTERRUPTS() vPortSetBASEPRI( 0 )
    /* 带中断保护的开中断函数 */
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x) vPortSetBASEPRI(x)

/* 不带返回值的关中断函数，不能嵌套，不能在中断里面使用 */ 
 #define portDISABLE_INTERRUPTS() vPortRaiseBASEPRI()
/* 带返回值的关中断函数，能嵌套，能在中断里面使用 */ 
#define portSET_INTERRUPT_MASK_FROM_ISR() ulPortRaiseBASEPRI()
    


/* 中断控制状态寄存器： 0xe000ed04
 * Bit 28 PENDSVSET: PendSV 悬起位
*/
    #define portNVIC_INT_CTRL_REG (*(( volatile uint32_t *) 0xe000ed04))
    #define portNVIC_PENDSVSET_BIT ( 1UL << 28UL )
    
    #define portSY_FULL_READ_WRITE ( 15 )


    #define portYIELD()                                                             \
    {                                                                               \
        /* Set a PendSV to request a context switch. */                             \
        portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;                             \
                                                                                    \
        /* Barriers are normally not required but do ensure the code is completely  \
         * within the specified behaviour for the architecture. */                  \
        __dsb( portSY_FULL_READ_WRITE );                                            \
        __isb( portSY_FULL_READ_WRITE );                                            \
    }
    
    #ifndef portFORCE_INLINE
        #define portFORCE_INLINE    __forceinline
    #endif
    
   static portFORCE_INLINE void vPortSetBASEPRI( uint32_t ulBASEPRI )
    {
        __asm
        {
            /* Barrier instructions are not used as this function is only used to
             * lower the BASEPRI value. */
            /* *INDENT-OFF* */
            msr basepri, ulBASEPRI
            /* *INDENT-ON* */
        }
    }
    
    /*关闭中断*/
    static portFORCE_INLINE void vPortRaiseBASEPRI( void )
    {
        uint32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

        __asm
        {
            /* Set BASEPRI to the max syscall priority to effect a critical
             * section. */
    /* *INDENT-OFF* */
            msr basepri, ulNewBASEPRI
            dsb
            isb
    /* *INDENT-ON* */
        }
     }
    
     static portFORCE_INLINE void vPortClearBASEPRIFromISR( void )
    {
        __asm
        {
            /* Set BASEPRI to 0 so no interrupts are masked.  This function is only
             * used to lower the mask in an interrupt, so memory barriers are not
             * used. */
    /* *INDENT-OFF* */
            msr basepri, # 0
    /* *INDENT-ON* */
        }
    }
    
     
     static portFORCE_INLINE uint32_t ulPortRaiseBASEPRI( void )
    {
        uint32_t ulReturn, ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

        __asm
        {
            /* Set BASEPRI to the max syscall priority to effect a critical
             * section. */
/* *INDENT-OFF* */
            mrs ulReturn, basepri
            msr basepri, ulNewBASEPRI
            dsb
            isb
/* *INDENT-ON* */
        }

        return ulReturn;
    }
    
    /*************************************************************关闭中断end****************************************************/

 #endif /* PORTMACRO_H */
 
 
