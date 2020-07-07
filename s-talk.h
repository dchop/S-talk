#ifndef _STALK_H_
#define _STALK_H_

#include "list.h"


void Threads_init();

char **setupPorts(char** args);

void inputFromKeyboard(List* SendingList);

void *inputToSend(List* SendingList);

void inputReceived(List* ReceivingList);

void inputToPrint(List* ReceivingList);

void Threads_shutdown(void);


#endif