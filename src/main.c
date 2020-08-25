/**
 * main.c for Assignment 3, CMPT 300 Summer 2020
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


int main (int argc, char* args[])
{
    // If 4 arguments are not provided, throw an error
    if(argc != 4){
        printf("Error: This program needs 4 command line arguments\n");
        exit(1);
    }

    // Sets up the sockets for communication
    setupPorts(args);

    // Creating threads and lists
    Threads_init();

    // Joining threads and closing socket
    Threads_shutdown();

    return 0;
}