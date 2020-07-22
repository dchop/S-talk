/**
 * s-talk.h for Assignment 3, CMPT 300 Summer 2020
 * Name: Devansh Chopra
 * Student #: 301-275-491
 */

#ifndef _STALK_H_
#define _STALK_H_

void Threads_init();

void *setupPorts(char** args);

void *inputFromKeyboard(void* SendingList);

void *inputToSend(void* SendingList);

void *inputReceived(void* ReceivingList);

void *inputToPrint(void* ReceivingList);

void Threads_shutdown(void);

#endif