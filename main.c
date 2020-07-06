#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>			// for close()

#define PORT    22110

int main (int argc, char** args)
{
    int localPort = atoi(args[1]);
    int remotePort = atoi(args[3]);

    printf("In main\n");
    setupPorts(localPort, remotePort);


    Threads_init();


    Threads_shutdown();
    return 0;
}