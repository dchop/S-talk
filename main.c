#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>			// for close()

#define PORT    22110

static int socketDescriptor; 

int main (int argc, char** args)
{
    struct sockaddr_in forLocalMachine;
    memset(&forLocalMachine, 0, sizeof(forLocalMachine));
    forLocalMachine.sin_family = AF_INET;
    forLocalMachine.sin_port = htons(PORT);
    forLocalMachine.sin_addr.s_addr = INADDR_ANY;

    socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);

    bind(socketDescriptor, (struct sockaddr *) &forLocalMachine, sizeof(forLocalMachine));




    printf("In main\n");
    Threads_init();

    


    Threads_shutdown();
    return 0;
}