#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>			// for close()

#define PORT    22110

int main (int argc, char** args)
{


    printf("In main\n");
    setupPorts(args);


    Threads_init();


    Threads_shutdown();
    return 0;
}