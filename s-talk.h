#ifndef _STALK_H_
#define _STALK_H_


void Threads_init();

char **setupPorts(char** args);

void *inputFromKeyboard(void *unused);

void *inputToSend(void *unused);

void *inputReceived(void *unused);

void *inputToPrint(void *unused);

void Threads_shutdown(void);


#endif