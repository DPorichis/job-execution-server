#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>

#include "utils.h"
#include "helpfunc.h"

// Structure for passing arguments to the controller thread
struct controller_args 
{
    Server server;
    int sock;
};
typedef struct controller_args* ControllerArgs;

// The routine of a controller thread
int controller(Server server, int sock);

// Wrapper function for dereferencing the argument
void* wrapper_controller(void * arg);

