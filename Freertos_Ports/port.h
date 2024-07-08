#ifndef __PORT_H
#define __PORT_H

#include "projdefs.h"
#include "portmacro.h"


static void prvTaskExitError( void );
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack,
                                    TaskFunction_t pxCode,
                                    void *pvParameters );
                                    
void xPortSysTickHandler( void );
void xPortPendSVHandler(void);
void vPortSVCHandler(void);
void prvStartFirstTask(void);
void vPortSetupTimerInterrupt( void );
BaseType_t xPortStartScheduler( void );



void vPortEnterCritical( void );
void vPortExitCritical( void );
#endif
