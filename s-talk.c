#include <pthread.h>
#include <signal.h>
#include <string.h>			// for strncmp()
#include <unistd.h>			// for close()
#include <stdlib.h>
#include <netdb.h>
#include "s-talk.h"

static pthread_t keyboardInput;
static pthread_t sendingInput;
static pthread_t receivedInput;
static pthread_t printingInputToScreen;

void Threads_init()
{
    
}


void Threads_shutdown(void)
{

}