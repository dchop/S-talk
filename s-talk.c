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