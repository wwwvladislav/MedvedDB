#include "mdv_threads.h"
#include <unistd.h>


void mdv_usleep(size_t msec)
{
    usleep(msec * 1000);
}
