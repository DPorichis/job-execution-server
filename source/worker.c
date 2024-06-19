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
    
        // Get a job from the buffer
        JobInstance to_execute = server_getJob(server);
        
        if(to_execute == NULL) // NULL indicates server termination
        {
            // Exit as a thread
            pthread_cond_broadcast(&server->alert_controller);
            return 0;
        }

        // Execute the job that was assigned
        
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
            // Write out the starting tag
            char buffer[256];
            int used = sprintf(buffer, "-----%s output start-----\n", to_execute->jobID);
            if (write(output_file, buffer, used) < 0)
            {
                perror("output manipulation error");
                exit(EXIT_FAILURE);
            }

            // Over-ride the stdout to the file
            dup2(output_file, 1);
            
            // Execute the command specified by the job
            if(execvp(to_execute->job_argv[0], &to_execute->job_argv[0]) == -1)
            {
                perror("execve");
            }
            exit(1);
        }

        // Only the parent process will reach this part of the code
        int status;
        // Wait for the job to finish
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
        
        // Writting the end tag at the end of the file
        lseek(output_file, 0, SEEK_END);
        char buffer[256];
        int used = sprintf(buffer, "-----%s output end-----\n", to_execute->jobID);
        if (write(output_file, buffer, used) < 0)
        {
            perror("output manipulation error");
            exit(EXIT_FAILURE);
        }

        // Read it from the start
        lseek(output_file, 0, SEEK_SET);
        int transmit_size;
        // Read a max of 256 charachters at a time
        while((transmit_size = read(output_file, buffer, 256)) > 0)
        {
            printf("Transmiting %d characters\n", transmit_size);
            // Transmit the length of the message
            if(write(to_execute->socket, &transmit_size, sizeof(int)) < 0)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }
            // Transmit the message
            if(write(to_execute->socket, buffer, transmit_size*sizeof(char)) < 0)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }
        // Send out 0 length to indicate end of message
        transmit_size = 0;
        if(write(to_execute->socket, &transmit_size, sizeof(int)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }

        if (shutdown(to_execute->socket, SHUT_WR) < 0) {
            perror("shutdown failed");
            exit(EXIT_FAILURE);
        }
        
        // Close everything and move to the next job
        close(to_execute->socket);
        close(output_file);
        unlink(name);

        // Don't forget to free up the space of the job instance  
        destroy_instance(to_execute);
    }
}


// Function for creating a worker thread, 
// the void * will be casted as a jobInstance pointer
// and worker function will be called
void * wrapper_worker(void* args)
{
    worker((Server)args);
    printf("Worker exits\n");
    fflush(stdout);
    pthread_exit(0);
}
