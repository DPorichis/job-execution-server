#include "worker.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int worker(Server server)
{
    while(1)
    {
        JobInstance to_execute = server_getJob(server);
        //Do the funny with the execution
    

        char* transmit = "To etrexa jerw egw\n";
        int transmit_size = strlen(transmit) + 1;

        printf("message size: %d \n", transmit_size);
        fflush(stdout);

        if(write(to_execute->socket, &transmit_size, sizeof(int)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }

        if(write(to_execute->socket, transmit, transmit_size*sizeof(char)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
    
    
    }
}


// Function for creating a worker thread, 
// the void * will be casted as a jobInstance pointer
// and worker function will be called
void * wrapper_worker(void* args)
{
    worker((Server)args);
}
