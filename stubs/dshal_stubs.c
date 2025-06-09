#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "dsTypes.h"
#include "dsDisplay.h"
#include "dsUtl.h"
#include "dsError.h"

#define RDK_DSHAL_NAME "libdshal.so" 

dsError_t dsGetDisplay(dsVideoPortType_t m_vType, int index, intptr_t *handle)
{
    return dsERR_NONE;
}

