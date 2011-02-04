
#define _POSIX_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>

int _init(void)
{
    prctl(0x59616d61, getppid(), 0, 0, 0);
    puts("eeks\n");
    return 0;
}

