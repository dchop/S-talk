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

static List* SendingList;
static List* ReceivingList;

// List Free Function pointer implementation
static void complexTestFreeFn(void* pItem) 
{
    pItem = NULL;
    freeCounter++;
}

char **setupPorts(char** args)
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

}

// Function for thread that just waits for keyboard input and adds it to the list
void *inputFromKeyboard(void* SendingList)
{
    char keyboardInputBuffer[512] = "";
    int checkForExclamation = 0;
    
    while(1)
    {
        pthread_testcancel();

        while(fgets(keyboardInputBuffer, 512, stdin) != NULL){

            int lengthOfInput = strlen(keyboardInputBuffer);

            // Checking if the input had an ! on a new line
            if((keyboardInputBuffer[lengthOfInput-2] == '!' && lengthOfInput == 2) || *keyboardInputBuffer == '!'){
                checkForExclamation = 1;
            }

            // Adding the keyboard input to the list
            pthread_mutex_lock(&sendList);
            {
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
                sleep(1);
                pthread_cancel(keyboardInput);
            }
        }

        if(read(STDIN_FILENO, keyboardInputBuffer, 512) < 0){
            printf("Error in keyboard input\n");
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
        if(sendto(socketDescriptor, sendBuffer, 512, 0, remoteinfo->ai_addr, remoteinfo->ai_addrlen) == -1){
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

            sleep(1);

            pthread_cancel(receivedInput);
            pthread_cancel(printingInputToScreen);
            pthread_cancel(sendingInput);
        }
    }
    return NULL;
}


// Function for thread that just recevies an input from the sender and adds it to a list
void *inputReceived(void* ReceivingList)
{
    char receivedMsg[512];
	socklen_t fromlen = sizeof(forRemoteMachine);
    int receivedMsgLength = 0;
    int checkExclamation =0;

    while(1)
    {
        pthread_testcancel();

        // Receiving keyboard input from the sender
        if(recvfrom(socketDescriptor, receivedMsg, 512, 0, (struct sockaddr *)&forRemoteMachine, &fromlen) == -1){
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
            sleep(1);
            pthread_cancel(receivedInput);
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
        if(write(STDOUT_FILENO, printingBuffer, 512) < 0){
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

            sleep(1);

            pthread_cancel(sendingInput);
            pthread_cancel(keyboardInput);
            pthread_cancel(printingInputToScreen);
        }
        
        memset(&printingBuffer, 0, sizeof(printingBuffer));
    }
    return NULL;
}

void Threads_init()
{
    SendingList = List_create();
    ReceivingList = List_create();

    pthread_create(&keyboardInput, NULL, inputFromKeyboard, SendingList);
    pthread_create(&sendingInput, NULL, inputToSend, SendingList);
    pthread_create(&receivedInput, NULL, inputReceived, ReceivingList);
    pthread_create(&printingInputToScreen, NULL, inputToPrint, ReceivingList);
}

void Threads_shutdown(void)
{
    // printf("Shutting down\n");
    pthread_join(keyboardInput, NULL);
    pthread_join(sendingInput, NULL);
    pthread_join(receivedInput, NULL);
    pthread_join(printingInputToScreen, NULL);

    // Freeing the 2 lists
    List_free(SendingList, complexTestFreeFn);
    List_free(ReceivingList, complexTestFreeFn);

    // Freeing address info
    freeaddrinfo(servinfo); 
    freeaddrinfo(remoteinfo);

    // Closing the Socket
    close(socketDescriptor);
}
















































































// #include <pthread.h>
// #include <signal.h>
// #include <string.h>			// for strncmp()
// #include <unistd.h>			// for close()
// #include <stdio.h>
// #include <stdlib.h>
// #include <netdb.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include "s-talk.h"
// #include "list.h"

// static pthread_t keyboardInput;
// static pthread_t sendingInput;
// static pthread_t receivedInput;
// static pthread_t printingInputToScreen;

// // Sending Mutexes
// static pthread_mutex_t sendList = PTHREAD_MUTEX_INITIALIZER;
// static pthread_mutex_t checkSendListEmpty = PTHREAD_MUTEX_INITIALIZER;

// // Receiving Mutexes
// static pthread_mutex_t receiveList = PTHREAD_MUTEX_INITIALIZER;
// static pthread_mutex_t checkReceiveListEmpty = PTHREAD_MUTEX_INITIALIZER;

// // Conditional variable for checking if sending list is empty
// static pthread_cond_t emptySendingList = PTHREAD_COND_INITIALIZER;
// static pthread_cond_t emptyReceiveList = PTHREAD_COND_INITIALIZER;

// // Making sure there is only one item in Sender List
// static pthread_mutex_t waitForSender = PTHREAD_MUTEX_INITIALIZER;
// static pthread_cond_t waitForSenderToFinish = PTHREAD_COND_INITIALIZER;

// // Making sure there is only one item in the Receiver List
// static pthread_mutex_t waitForReceiver = PTHREAD_MUTEX_INITIALIZER;
// static pthread_cond_t waitForReciverToPrint = PTHREAD_COND_INITIALIZER;


// struct sockaddr_in forLocalMachine;
// struct sockaddr_in forRemoteMachine;
// static int socketDescriptor; 

// char remotePortString[10];

// int status;
// int statusRemote;
// int checker = 0;

// struct addrinfo hints, *servinfo;
// struct addrinfo remote, *remoteinfo;

// char s[INET6_ADDRSTRLEN];

// char **setupPorts(char** args)
// {

//     int localPort = atoi(args[1]);
//     int remoteMachine = atoi(args[2]);
//     int remotePort = atoi(args[3]);
    

//     // Setting up local port 
//     memset(&hints, 0, sizeof(hints));
//     hints.ai_family = AF_INET;
//     hints.ai_socktype = SOCK_DGRAM;
//     hints.ai_flags = AI_PASSIVE;
//     // forLocalMachine.sin_port = htons(localPort);
//     // forLocalMachine.sin_addr.s_addr = INADDR_ANY;
//     if ((status = getaddrinfo(NULL, args[1], &hints, &servinfo)) != 0) {
//         fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
//         exit(1);
//     }

//     socketDescriptor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

//     bind(socketDescriptor, servinfo->ai_addr, servinfo->ai_addrlen);

//     memset(&remote, 0, sizeof(remote));
//     remote.ai_family = AF_INET;
//     remote.ai_socktype = SOCK_DGRAM;
//     remote.ai_flags = AI_PASSIVE;

//     if((statusRemote = getaddrinfo(args[2], args[3], &remote, &remoteinfo)) != 0){
//         fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
//         return NULL;
//         //exit(1);
//     }

//     printf("HI\n");
//     // Bind Socket

//     printf("local port is: %d\n", localPort);
//     printf("remote port is: %d\n", remotePort);

//     // Setting up remote port
//     // strcpy(remotePortString, remotePort);
//     // strcpy(s, remoteMachine);
// }

// void Threads_init()
// {
//     List* SendingList = List_create();
//     List* ReceivingList = List_create();

//     pthread_create(&keyboardInput, NULL, inputFromKeyboard, SendingList);
//     pthread_create(&sendingInput, NULL, inputToSend, SendingList);
//     pthread_create(&receivedInput, NULL, inputReceived, ReceivingList);
//     pthread_create(&printingInputToScreen, NULL, inputToPrint, ReceivingList);
// }

// void inputFromKeyboard(List* SendingList)
// {
//     char readBuffer[512];
//     int checkForExclamatiion = 0;
//     printf("inputFromKeyboard\n");
    
//     while(1)
//     {
//         pthread_testcancel();

//         fgets(readBuffer, 512, stdin);
//         // if(fgets(readBuffer, 512, stdin) == NULL){
//         //     continue;
//         // }
//         int lengthOfInput = strlen(readBuffer);

//         if(readBuffer[lengthOfInput-2] == '!'){
//             checkForExclamatiion = 1;
//             printf("check is: %d\n", checkForExclamatiion);
//         }

//         pthread_mutex_lock(&sendList);
//         {
//             List_add(SendingList, readBuffer);
//             int k = List_count(SendingList);
//             printf("k is: %d\n", k);
//             printf("input is: %s\n", readBuffer);
//         }
//         pthread_mutex_unlock(&sendList);

//         if(List_count(SendingList) == 1){
//             pthread_mutex_lock(&checkSendListEmpty);
//             {
//                 printf("NOW SENDING\n");
//                 pthread_cond_signal(&emptySendingList);
//             }
//             pthread_mutex_unlock(&checkSendListEmpty);
//         }


//         pthread_mutex_lock(&waitForSender);
//         {
//             pthread_cond_wait(&waitForSenderToFinish, &waitForSender);
//             memset(&readBuffer, 0, sizeof(readBuffer));
//             fflush(stdin);
//         }
//         pthread_mutex_unlock(&waitForSender);

//         if (checkForExclamatiion == 1){
//             // pthread_cancel(keyboardInput);
//             break;
            
//         }

//     }
//     pthread_exit(NULL);
//     // pthread_cancel(keyboardInput);
//     // pthread_exit(0);
//     // close(socketDescriptor);
//     // sendto(socketDescriptor, readBuffer, 512, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));

// }

// void *inputToSend(List* SendingList)
// {
//     printf("inputToSend\n");

//     char *sendBuffer;
//     int checkForExclamatiion = 0;

//     while(1)
//     {
//         pthread_testcancel();
//         if(List_count(SendingList) == 0){
//             pthread_mutex_lock(&checkSendListEmpty);
//             {
//                 printf("WAITING\n");
//                 pthread_cond_wait(&emptySendingList, &checkSendListEmpty);
//             }
//             pthread_mutex_unlock(&checkSendListEmpty);
//         }
//         printf("GOT IT\n");
//         pthread_mutex_lock(&sendList);
//         {
//             int k = List_count(SendingList);
//             printf("count is: %d\n", k);
//             sendBuffer = List_remove(SendingList);
//             if(*sendBuffer == '!'){
//                 printf("About to break\n");
//                 checkForExclamatiion = 1;
//             }
//         }
//         pthread_mutex_unlock(&sendList);

//         printf("input was: %s\n", sendBuffer);
//         if(sendto(socketDescriptor, sendBuffer, 512, 0, remoteinfo -> ai_addr, remoteinfo->ai_addrlen) == -1){
//                           perror("talker: sendto");
//                           exit(1);        
//         }


        
//         pthread_mutex_lock(&waitForSender);
//         {
//             pthread_cond_signal(&waitForSenderToFinish);
//             memset(&sendBuffer, 0, sizeof(sendBuffer));
//         }
//         pthread_mutex_unlock(&waitForSender);
//         // printf("Value is: %s\n", sendBuffer);
//         // break;
//         if(checkForExclamatiion == 1){
//             printf("ABOUT TO break in sending\n");
//             pthread_cancel(receivedInput);
//             pthread_cancel(printingInputToScreen);
//             pthread_exit(NULL);
//             // pthread_cancel(sendingInput);
//         }
//     }

//     // pthread_mutex_destroy(&sendList);
//     // pthread_mutex_destroy(&checkSendListEmpty);
//     // pthread_mutex_destroy(&waitForSender);

//     // pthread_cond_destroy(&emptySendingList);
//     // pthread_cond_destroy(&waitForSenderToFinish);

//     // pthread_cancel(keyboardInput);
//     // pthread_cancel(sendingInput);
//     // pthread_cancel(printingInputToScreen);
//     // pthread_cancel(receivedInput);
//     // close(socketDescriptor);
//     // pthread_exit(0);
// }

// void inputReceived(List* ReceivingList)
// {
//     char msg[512];
// 	socklen_t fromlen = sizeof(forRemoteMachine);
//     int exclamationLength;
//     printf("inputReceived\n");
//     while(1)
//     {
//         pthread_testcancel();
//         recvfrom(socketDescriptor, msg, 512, 0, (struct sockaddr *)&forRemoteMachine, &fromlen);
//         // printf("message is %s\n", msg);
//         // pthread_mutex_lock(&removeFromReceivingList);
//         // {
//         exclamationLength = strlen(msg);
//         pthread_mutex_lock(&receiveList);
//         {
//             List_add(ReceivingList, msg);
//         }
//         pthread_mutex_unlock(&receiveList);

//         if(List_count(ReceivingList) == 1){
//             pthread_mutex_lock(&checkReceiveListEmpty);
//             {
//                 pthread_cond_signal(&emptyReceiveList);
//             }
//             pthread_mutex_unlock(&checkReceiveListEmpty);
//         }


//         pthread_mutex_lock(&waitForReceiver);
//         {
//             pthread_cond_wait(&waitForReciverToPrint, &waitForReceiver);
            
//         }
//         pthread_mutex_unlock(&waitForReceiver);
        
//         if (msg[exclamationLength-2] == '!'){
//             printf("about to break in receive\n");
//             checker = 1;
//             // pthread_cancel(receivedInput);
//             pthread_exit(NULL);
//             // break;
           
//             // pthread_join(receivedInput, NULL);
//             // close(socketDescriptor);
//             // pthread_exit(0);
//             // break;
//         }
//         memset(&msg, 0, sizeof(msg));
//         // break;
//         // }
//         // pthread_mutex_unlock(&removeFromReceivingList);
//     }
     
//     // pthread_cancel(receivedInput);
//     // sleep(1);
//     // pthread_exit(0);
// }

// void inputToPrint(List* ReceivingList)
// {
//     char *readBuffer1;
//     while(1)
//     {
//         pthread_testcancel();
//         if(List_count(ReceivingList) == 0){
//             pthread_mutex_lock(&checkReceiveListEmpty);
//             {
//                 pthread_cond_wait(&emptyReceiveList, &checkReceiveListEmpty);
//             }
//             pthread_mutex_unlock(&checkReceiveListEmpty);
//         }
//         // if(List_count(ReceivingList)> 0){
//         // pthread_mutex_lock(&removeFromReceivingList);
//         // {
//         pthread_mutex_lock(&receiveList);
//         {
//             int k = List_count(ReceivingList);
//             // printf("count is: %d\n", k);
//             readBuffer1 = List_remove(ReceivingList);
//         }
//         pthread_mutex_unlock(&receiveList);

//         printf("Message is: %s\n", readBuffer1);

//         pthread_mutex_lock(&waitForReceiver);
//         {
//             pthread_cond_signal(&waitForReciverToPrint);
            
//         }
//         pthread_mutex_unlock(&waitForReceiver);

//         if (*readBuffer1 == '!'){
//             printf("print break\n");
//             // pthread_cancel(printingInputToScreen);
//             pthread_cancel(sendingInput);
//             pthread_cancel(keyboardInput);
//             pthread_exit(NULL);

//         }
//         memset(&readBuffer1, 0, sizeof(readBuffer1));
//     }


//     // pthread_mutex_destroy(&receiveList);
//     // pthread_mutex_destroy(&checkReceiveListEmpty);
//     // pthread_mutex_destroy(&waitForReceiver);

//     // pthread_cond_destroy(&emptyReceiveList);
//     // pthread_cond_destroy(&waitForReciverToPrint);

//     // pthread_cancel(printingInputToScreen);
//     // pthread_cancel(receivedInput);
//     // pthread_cancel(sendingInput);
//     // pthread_cancel(keyboardInput);

//     // pthread_exit(0);

// }

// void Threads_shutdown(void)
// {
//     printf("Shutting down\n");
//     pthread_join(keyboardInput, NULL);
//     pthread_join(sendingInput, NULL);
//     pthread_join(receivedInput, NULL);
//     pthread_join(printingInputToScreen, NULL);
// }

// void Clean_up_sendingStuff(){
    
// }