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
#include <sys/wait.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int worker(Server server)
{
    while(1)
    {
    
        // THIS IS NOT BUSY WAITING
        JobInstance to_execute = server_getJob(server);
        
        //Do the funny with the execution
        
        pid_t childpid = fork();
        if(childpid == -1)
        {
            perror("Failed to fork");
            exit(1);
        }
        if(childpid == 0)
        {

            // Setup output redirection
            int output_file;
            char name[100];
            sprintf(name, "%d.output", getpid());
            if((output_file = creat(name , 0777))==-1)
            {
                printf("file already here\n");
                perror("creating");
                exit(1);
            }
            char buffer[256];
            int used = sprintf(buffer, "-----%s output start-----\n", to_execute->jobID);
            if (write(output_file, buffer, used) < 0)
            {
                perror("output manipulation error");
                exit(EXIT_FAILURE);
            }

            dup2(output_file, 1);
            
            // Execute the command girl
            if(execvp(to_execute->job_argv[0], &to_execute->job_argv[0]) == -1)
            {
                perror("execve");
            }
            exit(1);
        }

        // Only the parent process will reach this part of the code
        int status;
        if ((childpid = waitpid(childpid, NULL, 0)) == -1 ) {
            perror ("wait failed"); exit(2);
        }

        // Reading the output file
        int output_file;
        char name[100];
        sprintf(name, "%d.output", childpid);
        
        if((output_file = open(name, O_RDWR, 0644)) == -1)
        {
            perror("Problem in reading the outputfile");
            exit(1);
        }
        
        lseek(output_file, 0, SEEK_END);

        char buffer[256];
        int used = sprintf(buffer, "-----%s output end-----\n", to_execute->jobID);
        if (write(output_file, buffer, used) < 0)
        {
            perror("output manipulation error");
            exit(EXIT_FAILURE);
        }

        lseek(output_file, 0, SEEK_SET);
        int transmit_size;
        while((transmit_size = read(output_file, buffer, 256)) > 0)
        {
            printf("Transmiting %d characters\n", transmit_size);
            if(write(to_execute->socket, &transmit_size, sizeof(int)) < 0)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }

            if(write(to_execute->socket, buffer, transmit_size*sizeof(char)) < 0)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }
        transmit_size = 0;
        if(write(to_execute->socket, &transmit_size, sizeof(int)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }


        // char* transmit = "To etrexa jerw egw\n";
        // transmit_size = strlen(transmit) + 1;

        // printf("message size: %d \n", transmit_size);
        // fflush(stdout);

        // if(write(to_execute->socket, &transmit_size, sizeof(int)) < 0)
        // {
        //     perror("write");
        //     exit(EXIT_FAILURE);
        // }

        // if(write(to_execute->socket, transmit, transmit_size*sizeof(char)) < 0)
        // {
        //     perror("write");
        //     exit(EXIT_FAILURE);
        // }
    
    
    }
}


// Function for creating a worker thread, 
// the void * will be casted as a jobInstance pointer
// and worker function will be called
void * wrapper_worker(void* args)
{
    worker((Server)args);
}
