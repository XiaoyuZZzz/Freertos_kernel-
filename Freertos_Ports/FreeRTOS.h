#ifndef __FREERTOS_H
#define __FREERTOS_H

#include <stddef.h>

#include <stdint.h>

#include "FreeRTOSConfig.h"

#include "projdefs.h"

#ifndef configASSERT
    #define configASSERT( x )
    #define configASSERT_DEFINED    0
#else
    #define configASSERT_DEFINED    1
#endif


#define	 configUSE_TIME_SLICING		1		



#endif

