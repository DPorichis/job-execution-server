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

struct controller_args 
{
    Server server;
    int sock;
};

typedef struct controller_args* ControllerArgs;


int controller(Server server, int sock);

void* wrapper_controller(void * arg);

