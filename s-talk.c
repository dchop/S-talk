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

char remotePortString[10];

int status;
int statusRemote;

struct addrinfo hints, *servinfo;
struct addrinfo remote, *remoteinfo;

char s[INET6_ADDRSTRLEN];

char **setupPorts(char** args)
{

    int localPort = atoi(args[1]);
    int remoteMachine = atoi(args[2]);
    int remotePort = atoi(args[3]);
    

    // Setting up local port 
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    // forLocalMachine.sin_port = htons(localPort);
    // forLocalMachine.sin_addr.s_addr = INADDR_ANY;
    if ((status = getaddrinfo(NULL, args[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    socketDescriptor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    bind(socketDescriptor, servinfo->ai_addr, servinfo->ai_addrlen);

    memset(&remote, 0, sizeof(remote));
    remote.ai_family = AF_INET;
    remote.ai_socktype = SOCK_DGRAM;
    remote.ai_flags = AI_PASSIVE;

    if((statusRemote = getaddrinfo(args[2], args[3], &remote, &remoteinfo)) != 0){
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return NULL;
        //exit(1);
    }

    printf("HI\n");
    // Bind Socket

    printf("local port is: %d\n", localPort);
    printf("remote port is: %d\n", remotePort);

    // Setting up remote port
    // strcpy(remotePortString, remotePort);
    // strcpy(s, remoteMachine);

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
    // sendto(socketDescriptor, readBuffer, 512, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));
    if(sendto(socketDescriptor, readBuffer, 512, 0, remoteinfo -> ai_addr, remoteinfo->ai_addrlen) == -1){
                          perror("talker: sendto");
                          exit(1);
                      
              }

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