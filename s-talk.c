#include <pthread.h>
#include <signal.h>
#include <string.h>			// for strncmp()
#include <unistd.h>			// for close()
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netdb.h>
#include "s-talk.h"
#include "list.h"

static pthread_t keyboardInput;
static pthread_t sendingInput;
static pthread_t receivedInput;
static pthread_t printingInputToScreen;

struct sockaddr_in forLocalMachine;
struct sockaddr_in forRemoteMachine;
static int socketDescriptor; 

int setupPorts(localPort, remotePort)
{
    // Setting up local port 
    memset(&forLocalMachine, 0, sizeof(forLocalMachine));
    forLocalMachine.sin_family = AF_INET;
    forLocalMachine.sin_port = htons(localPort);
    forLocalMachine.sin_addr.s_addr = INADDR_ANY;

    socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);

    // Bind Socket
    bind(socketDescriptor, (struct sockaddr *) &forLocalMachine, sizeof(forLocalMachine));
    
    printf("local port is: %d\n", localPort);
    printf("remote port is: %d\n", remotePort);

    memset(&forRemoteMachine, 0, sizeof(forRemoteMachine));
    forRemoteMachine.sin_family = AF_INET;
    forLocalMachine.sin_port = htons(remotePort);

}


void Threads_init()
{
    List* SendingList = List_create();
    List* ReceivingList = List_create();

    pthread_create(&keyboardInput, NULL, inputFromKeyboard, SendingList);
    pthread_create(&sendingInput, NULL, inputToSend, SendingList);
    pthread_create(&receivedInput, NULL, inputReceived, ReceivingList);
    pthread_create(&printingInputToScreen, NULL, inputToPrint, ReceivingList);
}

void *inputFromKeyboard(void *unused)
{
    char *readBuffer[512];
    printf("inputFromKeyboard\n");
    fgets(readBuffer, 512, stdin);
    sendto(socketDescriptor, readBuffer, 512, 0, (struct sockaddr *)&forRemoteMachine, sizeof(struct sockaddr_in));

}

void *inputToSend(void *unused)
{
    printf("inputToSend\n");
}

void *inputReceived(void *unused)
{
    char msg[256];
	socklen_t fromlen = sizeof(forRemoteMachine);
    printf("inputReceived\n");
    recvfrom(socketDescriptor, msg, 256, 0, (struct sockaddr *)&forRemoteMachine, &fromlen);
    printf("message is %s\n", msg);

}

void *inputToPrint(void *unused)
{

}

void Threads_shutdown(void)
{
    pthread_join(keyboardInput, NULL);
    pthread_join(sendingInput, NULL);
    pthread_join(receivedInput, NULL);
    pthread_join(printingInputToScreen, NULL);
}