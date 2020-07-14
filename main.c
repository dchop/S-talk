#include <pthread.h>
#include <signal.h>
#include <string.h>			// for strncmp()
#include <unistd.h>			// for close()
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netdb.h>
#include "list.h"

static pthread_t keyboardInput;
static pthread_t sendingInput;
static pthread_t receivedInput;
static pthread_t printingInputToScreen;

// Sending Mutexes
static pthread_mutex_t sendList = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t checkSendListEmpty = PTHREAD_MUTEX_INITIALIZER;

// Receiving Mutexes
static pthread_mutex_t receiveList = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t checkReceiveListEmpty = PTHREAD_MUTEX_INITIALIZER;

// Conditional variable for checking if sending list is empty
static pthread_cond_t emptySendingList = PTHREAD_COND_INITIALIZER;
static pthread_cond_t emptyReceiveList = PTHREAD_COND_INITIALIZER;

// Making sure there is only one item in Sender List
static pthread_mutex_t waitForSender = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t waitForSenderToFinish = PTHREAD_COND_INITIALIZER;

// Making sure there is only one item in the Receiver List
static pthread_mutex_t waitForReceiver = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t waitForReciverToPrint = PTHREAD_COND_INITIALIZER;


struct sockaddr_in forLocalMachine;
struct sockaddr_in forRemoteMachine;
static int socketDescriptor; 

static int status;
static int statusRemote;
static int freeCounter = 0;

static struct addrinfo hints, *servinfo;
static struct addrinfo remote, *remoteinfo;

static void complexTestFreeFn(void* pItem) 
{
    freeCounter++;
}

void *inputFromKeyboard(void* SendingList)
{
    char readBuffer[512];
    int checkForExclamatiion = 0;
    // printf("inputFromKeyboard\n");
    
    while(1)
    {
        pthread_testcancel();

        while(read(STDIN_FILENO, readBuffer, 512) > 0){

            int lengthOfInput = strlen(readBuffer);

            if(readBuffer[lengthOfInput-2] == '!' || readBuffer[lengthOfInput-1] == '!' || *readBuffer == '!'){
                checkForExclamatiion = 1;
                // printf("check is: %d\n", checkForExclamatiion);
            }

            pthread_mutex_lock(&sendList);
            {
                List_add(SendingList, readBuffer);
                // int k = List_count(SendingList);
                // printf("k is: %d\n", k);
                // printf("input is: %s\n", readBuffer);
            }
            pthread_mutex_unlock(&sendList);

            if(List_count(SendingList) == 1){
                pthread_mutex_lock(&checkSendListEmpty);
                {
                    // printf("NOW SENDING\n");
                    pthread_cond_signal(&emptySendingList);
                }
                pthread_mutex_unlock(&checkSendListEmpty);
            }

            pthread_mutex_lock(&waitForSender);
            {
                pthread_cond_wait(&waitForSenderToFinish, &waitForSender);
                memset(&readBuffer, 0, sizeof(readBuffer));
                fflush(stdin);
            }
            pthread_mutex_unlock(&waitForSender);

            if (checkForExclamatiion == 1){
                pthread_cancel(keyboardInput);
            }
        }
        if(read(STDIN_FILENO, readBuffer, 512) < 0){
            printf("Error in keyboard input\n");
        }
    }
}

void *inputToSend(void* SendingList)
{
    // printf("inputToSend\n");

    char *sendBuffer;
    int checkForExclamatiion = 0;

    while(1)
    {
        pthread_testcancel();
        if(List_count(SendingList) == 0){
            pthread_mutex_lock(&checkSendListEmpty);
            {
                // printf("WAITING\n");
                pthread_cond_wait(&emptySendingList, &checkSendListEmpty);
            }
            pthread_mutex_unlock(&checkSendListEmpty);
        }
        // printf("GOT IT\n");
        pthread_mutex_lock(&sendList);
        {
            // int k = List_count(SendingList);
            // printf("count is: %d\n", k);
            sendBuffer = List_remove(SendingList);
            int length = strlen(sendBuffer);
            if(*sendBuffer == '!' || sendBuffer[length-1] == '!'){
                // printf("About to break\n");
                checkForExclamatiion = 1;
            }
        }
        pthread_mutex_unlock(&sendList);

        // printf("input was: %s\n", sendBuffer);
        if(sendto(socketDescriptor, sendBuffer, 512, 0, remoteinfo -> ai_addr, remoteinfo->ai_addrlen) == -1){
            perror("Error in sendto: ");
            exit(1);        
        }

        pthread_mutex_lock(&waitForSender);
        {
            pthread_cond_signal(&waitForSenderToFinish);
            memset(&sendBuffer, 0, sizeof(sendBuffer));
        }
        pthread_mutex_unlock(&waitForSender);

        if(checkForExclamatiion == 1){
            // printf("ABOUT TO break in sending\n");

            pthread_mutex_destroy(&sendList);
            pthread_mutex_destroy(&checkSendListEmpty);
            pthread_mutex_destroy(&waitForSender);

            pthread_cond_destroy(&emptySendingList);
            pthread_cond_destroy(&waitForSenderToFinish);

            pthread_cancel(receivedInput);
            pthread_cancel(printingInputToScreen);
            pthread_cancel(sendingInput);
        }
    }
}

void *inputReceived(void* ReceivingList)
{
    char msg[512];
	socklen_t fromlen = sizeof(forRemoteMachine);
    int exclamationLength = 0;
    int checkExclamation;
    // printf("inputReceived\n");
    while(1)
    {
        pthread_testcancel();
        if(recvfrom(socketDescriptor, msg, 512, 0, (struct sockaddr *)&forRemoteMachine, &fromlen) == -1){
            perror("Error in recvfrom: ");
            exit(1);  
        }
        // printf("message is %s\n", msg);
        // pthread_mutex_lock(&removeFromReceivingList);
        // {
        exclamationLength = strlen(msg);
        if (*msg == '!' || msg[exclamationLength-1] == '!'){
            checkExclamation = 1;
        }

        pthread_mutex_lock(&receiveList);
        {
            List_add(ReceivingList, msg);
        }
        pthread_mutex_unlock(&receiveList);

        if(List_count(ReceivingList) == 1){
            pthread_mutex_lock(&checkReceiveListEmpty);
            {
                pthread_cond_signal(&emptyReceiveList);
            }
            pthread_mutex_unlock(&checkReceiveListEmpty);
        }

        pthread_mutex_lock(&waitForReceiver);
        {
            pthread_cond_wait(&waitForReciverToPrint, &waitForReceiver);
            
        }
        pthread_mutex_unlock(&waitForReceiver);


        if (checkExclamation == 1){
            // printf("about to break in receive\n");
            pthread_cancel(receivedInput);
        }
        memset(&msg, 0, sizeof(msg));
    }
}

void *inputToPrint(void* ReceivingList)
{
    char *readBuffer1;
    int checkexclamation = 0;
     int length;
    while(1)
    {
        pthread_testcancel();
        if(List_count(ReceivingList) == 0){
            pthread_mutex_lock(&checkReceiveListEmpty);
            {
                pthread_cond_wait(&emptyReceiveList, &checkReceiveListEmpty);
            }
            pthread_mutex_unlock(&checkReceiveListEmpty);
        }

        pthread_mutex_lock(&receiveList);
        {
            // int k = List_count(ReceivingList);
            // printf("count is: %d\n", k);
            readBuffer1 = List_remove(ReceivingList);
            length = strlen(readBuffer1);
            if (*readBuffer1 == '!' || readBuffer1[length-1] == '!'){
                checkexclamation = 1;
            }
        }
        pthread_mutex_unlock(&receiveList);

        if(write(STDOUT_FILENO, readBuffer1, 512) < 0){
            printf("Error in printing to screen\n");
        }

        fflush(stdout);

        pthread_mutex_lock(&waitForReceiver);
        {
            pthread_cond_signal(&waitForReciverToPrint);
            
        }
        pthread_mutex_unlock(&waitForReceiver);

        if (checkexclamation == 1){
            // printf("print break\n");

            pthread_mutex_destroy(&receiveList);
            pthread_mutex_destroy(&checkReceiveListEmpty);
            pthread_mutex_destroy(&waitForReceiver);

            pthread_cond_destroy(&emptyReceiveList);
            pthread_cond_destroy(&waitForReciverToPrint);

            pthread_cancel(sendingInput);
            pthread_cancel(keyboardInput);
            pthread_cancel(printingInputToScreen);
        }
        memset(&readBuffer1, 0, sizeof(readBuffer1));
    }
}

int main (int argc, char** args)
{
    // int localPort = atoi(args[1]);
    // int remoteMachine = atoi(args[2]);
    // int remotePort = atoi(args[3]);
    int argsLength = strlen(args);

    // if(argsLength != 4){
    //     printf("length is: %d\n", argsLength);
    //     printf("Error: This program needs 4 arguments\n");
    //     exit(1);
    // }
    

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
    
    if(socketDescriptor == -1){
        printf("Error: Unable to create socket descriptor\n");
        exit(1);
    }

    int bindCheck = bind(socketDescriptor, servinfo->ai_addr, servinfo->ai_addrlen);

    if(bindCheck == -1){
        printf("Error: Unable to bind\n");
        exit(1);
    }

    memset(&remote, 0, sizeof(remote));
    remote.ai_family = AF_INET;
    remote.ai_socktype = SOCK_DGRAM;
    remote.ai_flags = AI_PASSIVE;

    if((statusRemote = getaddrinfo(args[2], args[3], &remote, &remoteinfo)) != 0){
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // Bind Socket

    // printf("local port is: %d\n", localPort);
    // printf("remote port is: %d\n", remotePort);

    // Setting up remote port
    // strcpy(remotePortString, remotePort);
    // strcpy(s, remoteMachine);


    // printf("In main\n");
    // setupPorts(args);

    freeaddrinfo(servinfo);
    freeaddrinfo(remoteinfo);

    List* SendingList = List_create();
    List* ReceivingList = List_create();

    pthread_create(&keyboardInput, NULL, inputFromKeyboard, SendingList);
    pthread_create(&sendingInput, NULL, inputToSend, SendingList);
    pthread_create(&receivedInput, NULL, inputReceived, ReceivingList);
    pthread_create(&printingInputToScreen, NULL, inputToPrint, ReceivingList);

    pthread_join(keyboardInput, NULL);
    pthread_join(sendingInput, NULL);
    pthread_join(receivedInput, NULL);
    pthread_join(printingInputToScreen, NULL);


    List_free(SendingList, complexTestFreeFn);
    List_free(ReceivingList, complexTestFreeFn);

    close(socketDescriptor);

    return 0;
}