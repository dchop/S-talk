/**
 * s-talk.c for Assignment 3, CMPT 300 Summer 2020
 * Name: Devansh Chopra
 * Student #: 301-275-491
 */

// Header files imported
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

// Max length of buffers
#define MSG_MAX_LEN 512

// Four Threads doing the communication
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

// Mutex and conditional variable for making sure sender has sent over the input
static pthread_mutex_t waitForSender = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t waitForSenderToFinish = PTHREAD_COND_INITIALIZER;

// Mutex and conditional variable for making sure the receiver has printed to the screen
static pthread_mutex_t waitForReceiver = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t waitForReciverToPrint = PTHREAD_COND_INITIALIZER;

// Structs that keep info for the localMachine and remoteMachine
static struct sockaddr_in forRemoteMachine;
static struct addrinfo hints, *servinfo;
static struct addrinfo remote, *remoteinfo;

// Helper integers
static int socketDescriptor; 
static int status;
static int statusRemote;
static int freeCounter = 0;

// Declaring the 2 lists
static List* SendingList;
static List* ReceivingList;

// List Free Function pointer implementation
static void complexTestFreeFn(void* pItem) 
{
    pItem = NULL;
    freeCounter++;
}

// Function for setting up UDP communication
void *setupPorts(char* args[])
{
    // Setting up local port 
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    // Getting address info for the local port
    if ((status = getaddrinfo(NULL, args[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // Setting up the local socket
    socketDescriptor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    
    // Error checking for the socket
    if(socketDescriptor == -1){
        printf("Error: Unable to create socket descriptor\n");
        exit(1);
    }

    int bindCheck = bind(socketDescriptor, servinfo->ai_addr, servinfo->ai_addrlen);

    // Error checking for the binding
    if(bindCheck == -1){
        printf("Error: Unable to bind\n");
        exit(1);
    }

    // Setting up the remote port
    memset(&remote, 0, sizeof(remote));
    remote.ai_family = AF_INET;
    remote.ai_socktype = SOCK_DGRAM;
    remote.ai_flags = AI_PASSIVE;

    // Getting address info for the remote port
    if((statusRemote = getaddrinfo(args[2], args[3], &remote, &remoteinfo)) != 0){
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    return NULL;
}

// Function for thread that just waits for keyboard input and adds it to the list
void *inputFromKeyboard(void* SendingList)
{
    char keyboardInputBuffer[MSG_MAX_LEN] = "";
    int checkForExclamation = 0;
    
    while(1)
    {
        pthread_testcancel();

        while(fgets(keyboardInputBuffer, MSG_MAX_LEN, stdin) != NULL){

            int lengthOfInput = strlen(keyboardInputBuffer);

            // Checking if the input had an ! on a new line
            if((keyboardInputBuffer[lengthOfInput-2] == '!' && lengthOfInput == 2) || *keyboardInputBuffer == '!'){
                checkForExclamation = 1;
            }

            // Adding the keyboard input to the list
            pthread_mutex_lock(&sendList);
            {
                if(List_count(SendingList) >= 100){
                    printf("Error: Sending list is full\n");
                    exit(1);
                }
                List_add(SendingList, keyboardInputBuffer);
            }
            pthread_mutex_unlock(&sendList);

            // Signaling the sender as soon as the list has something to send
            if(List_count(SendingList) == 1){
                pthread_mutex_lock(&checkSendListEmpty);
                {
                    pthread_cond_signal(&emptySendingList);
                }
                pthread_mutex_unlock(&checkSendListEmpty);
            }

            // Waiting for the sender to finish sending before taking in more input
            pthread_mutex_lock(&waitForSender);
            {
                pthread_cond_wait(&waitForSenderToFinish, &waitForSender);
                memset(&keyboardInputBuffer, 0, sizeof(keyboardInputBuffer));
                fflush(stdin);
            }
            pthread_mutex_unlock(&waitForSender);

            // If an exclamation was found, then cancel the keyboardInput thread
            if (checkForExclamation == 1){
                int keyboardCancel = pthread_cancel(keyboardInput);
                if (keyboardCancel != 0){
                    printf("Error: could not cancel keyboardThread\n");
                    exit(1);
                }
            }
        }
    }
    return NULL;
}

// Function for thread that removes an item off the list and sends it over
void *inputToSend(void* SendingList)
{
    char *sendBuffer;
    int checkForExclamation = 0;

    while(1)
    {
        pthread_testcancel();

        // Waiting if the list is empty
        if(List_count(SendingList) == 0){
            pthread_mutex_lock(&checkSendListEmpty);
            {
                pthread_cond_wait(&emptySendingList, &checkSendListEmpty);
            }
            pthread_mutex_unlock(&checkSendListEmpty);
        }

        // Removing an item from the list to be send over
        pthread_mutex_lock(&sendList);
        {
            sendBuffer = List_remove(SendingList);
            int length = strlen(sendBuffer);

            if(*sendBuffer == '!' || (sendBuffer[length-2] == '!' && length == 2)){
                checkForExclamation = 1;
            }
        }
        pthread_mutex_unlock(&sendList);

        // Sending the item that was removed from the list
        if(sendto(socketDescriptor, sendBuffer, MSG_MAX_LEN, 0, remoteinfo->ai_addr, remoteinfo->ai_addrlen) == -1){
            perror("Error in sendto: ");
            exit(1);        
        }

        // Signaling the keyboardInput thread to stop waiting and continue taking more keyboard input
        pthread_mutex_lock(&waitForSender);
        {
            pthread_cond_signal(&waitForSenderToFinish);
            memset(&sendBuffer, 0, sizeof(sendBuffer));
        }
        pthread_mutex_unlock(&waitForSender);

        // If there was an exclamation then destroy the locks, conditional variables and cancll the other threads
        if(checkForExclamation == 1){
            pthread_mutex_destroy(&sendList);
            pthread_mutex_destroy(&checkSendListEmpty);
            pthread_mutex_destroy(&waitForSender);

            pthread_cond_destroy(&emptySendingList);
            pthread_cond_destroy(&waitForSenderToFinish);

            int receivedCancel = pthread_cancel(receivedInput);
            int printingCancel = pthread_cancel(printingInputToScreen);
            int sendingCancel = pthread_cancel(sendingInput);

            if (receivedCancel != 0 || printingCancel != 0 || sendingCancel != 0){
                printf("Error: could not cancel threads in Sending Function\n");
                exit(1);
            }         
        }
    }
    return NULL;
}

// Function for thread that just recevies an input from the sender and adds it to a list
void *inputReceived(void* ReceivingList)
{
    char receivedMsg[MSG_MAX_LEN];
	socklen_t fromlen = sizeof(forRemoteMachine);
    int receivedMsgLength = 0;
    int checkExclamation = 0;

    while(1)
    {
        pthread_testcancel();

        // Receiving keyboard input from the sender
        if(recvfrom(socketDescriptor, receivedMsg, MSG_MAX_LEN, 0, (struct sockaddr *)&forRemoteMachine, &fromlen) == -1){
            perror("Error in recvfrom: ");
            exit(1);  
        }

        receivedMsgLength = strlen(receivedMsg);

        // Checking if the an ! was received
        if (*receivedMsg == '!' || (receivedMsg[receivedMsgLength-2] == '!' && receivedMsgLength == 2)){
            checkExclamation = 1;
        }

        // Adding received input to the list for printing
        pthread_mutex_lock(&receiveList);
        {
            if(List_count(ReceivingList) >= 100){
                printf("Error: Receiving list is full\n");
                exit(1);
            }
            List_add(ReceivingList, receivedMsg);
        }
        pthread_mutex_unlock(&receiveList);

        // Signaling the printing thread if there is something in the list
        if(List_count(ReceivingList) == 1){
            pthread_mutex_lock(&checkReceiveListEmpty);
            {
                pthread_cond_signal(&emptyReceiveList);
            }
            pthread_mutex_unlock(&checkReceiveListEmpty);
        }

        // Wait for printing thread to print everything to the screen
        pthread_mutex_lock(&waitForReceiver);
        {
            pthread_cond_wait(&waitForReciverToPrint, &waitForReceiver);
            
        }
        pthread_mutex_unlock(&waitForReceiver);

        // If there was an ! in the received item then cancel this thread
        if (checkExclamation == 1){
            int receiveCancel = pthread_cancel(receivedInput);
            if (receiveCancel != 0){
                printf("Error: could not cancel receiveCancel\n");
                exit(1);
            }
        }
        memset(&receivedMsg, 0, sizeof(receivedMsg));
    }
    return NULL;
}

// Function for thread that just removes an item from the list and prints it to the screen
void *inputToPrint(void* ReceivingList)
{
    char *printingBuffer;
    int checkexclamation = 0;
    int length;

    while(1)
    {
        pthread_testcancel();

        // Wait if the list is empty
        if(List_count(ReceivingList) == 0){
            pthread_mutex_lock(&checkReceiveListEmpty);
            {
                pthread_cond_wait(&emptyReceiveList, &checkReceiveListEmpty);
            }
            pthread_mutex_unlock(&checkReceiveListEmpty);
        }

        // Remove an item from the list to be printed to the screen
        pthread_mutex_lock(&receiveList);
        {
            printingBuffer = List_remove(ReceivingList);
            length = strlen(printingBuffer);
            if (*printingBuffer == '!' || (printingBuffer[length-2] == '!' && length == 2)){
                checkexclamation = 1;
            }
        }
        pthread_mutex_unlock(&receiveList);

        // Print to the screen 
        if(write(STDOUT_FILENO, printingBuffer, MSG_MAX_LEN) < 0){
            printf("Error in printing to screen\n");
        }

        fflush(stdout);

        // Signal the receiver thread to receive more inputs from the sender
        pthread_mutex_lock(&waitForReceiver);
        {
            pthread_cond_signal(&waitForReciverToPrint);
        }
        pthread_mutex_unlock(&waitForReceiver);

        // If there is an ! then destroy the mutexes, conditional variables and cancel the other threads
        if (checkexclamation == 1){
            pthread_mutex_destroy(&receiveList);
            pthread_mutex_destroy(&checkReceiveListEmpty);
            pthread_mutex_destroy(&waitForReceiver);

            pthread_cond_destroy(&emptyReceiveList);
            pthread_cond_destroy(&waitForReciverToPrint);

            int sendCancel = pthread_cancel(sendingInput);
            int keyboardCancel = pthread_cancel(keyboardInput);
            int printCancel = pthread_cancel(printingInputToScreen);

            if (sendCancel != 0 || keyboardCancel != 0 || printCancel != 0){
                printf("Error: could not cancel threads in printingFunctionl\n");
                exit(1);
            }
        }
        memset(&printingBuffer, 0, sizeof(printingBuffer));
    }
    return NULL;
}

// Function for initializing lists and creating the 4 threads
void Threads_init()
{
    SendingList = List_create();
    ReceivingList = List_create();

    int keyboardThread = pthread_create(&keyboardInput, NULL, inputFromKeyboard, SendingList);
    int sendingThread = pthread_create(&sendingInput, NULL, inputToSend, SendingList);
    int receivingThread = pthread_create(&receivedInput, NULL, inputReceived, ReceivingList);
    int printingThread = pthread_create(&printingInputToScreen, NULL, inputToPrint, ReceivingList);
    
    if(keyboardThread != 0 || sendingThread != 0 || receivingThread != 0 || printingThread != 0){
        printf("Error: cannot create threads\n");
        exit(1);
    }
}

// Function for shutting down
void Threads_shutdown(void)
{
    int keyboardJoin = pthread_join(keyboardInput, NULL);
    int senderJoin = pthread_join(sendingInput, NULL);
    int receiveJoin = pthread_join(receivedInput, NULL);
    int printJoin = pthread_join(printingInputToScreen, NULL);

    if(keyboardJoin != 0 || senderJoin != 0 || receiveJoin != 0 || printJoin != 0){
        printf("Error: cannot join threads\n");
        exit(1);
    }
    
    // Freeing the 2 lists
    List_free(SendingList, complexTestFreeFn);
    List_free(ReceivingList, complexTestFreeFn);

    // Freeing address info
    freeaddrinfo(servinfo); 
    freeaddrinfo(remoteinfo);

    // Closing the Socket
    close(socketDescriptor);
}
