#ifndef __PROJDEFS_H
#define __PROJDEFS_H

typedef void (*TaskFunction_t)( void * );

#define pdFALSE ( ( BaseType_t ) 0 )
#define pdTRUE ( ( BaseType_t ) 1 )

#define pdPASS ( pdTRUE )
#define pdFAIL ( pdFALSE )

#endif
