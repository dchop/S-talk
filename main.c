#include <pthread.h>
#include <signal.h>
#include <string.h>	
#include <unistd.h>	
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netdb.h>
#include "list.h"
#include "s-talk.h"


int main (int argc, char** args)
{
    setupPorts(args);
    
    Threads_init();

    Threads_shutdown();

    return 0;
}