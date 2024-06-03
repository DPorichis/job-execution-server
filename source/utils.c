#include "utils.h"
#include "helpfunc.h"

#include <stdlib.h>
#include <string.h>


Server server_create(int bufsize, int concurrency)
{
    Server serv = malloc(sizeof(*serv));
    serv->front = 0;
    serv->queued = 0;
    serv->running_now = 0;
    serv->total_jobs = 1;
    serv->size = bufsize;
    serv->concurrency = concurrency;

    serv->job_queue = malloc(sizeof(JobInstance)*bufsize);
    for(int i = 0; i < bufsize; i++)
        serv->job_queue[i] = NULL;
    
    return serv;
}

char* server_issueJob(Server server, char* job_argv[], int job_argc, int job_socket)
{
    // Lock the mutex
    
    while(server->queued == server->size)
    {
        // Sleep again
    }
    

    // Space available, insert it at the end of the cycle
    
    // Create a new JobInstance
    JobInstance i = malloc(sizeof(*i));
    
    // Store the argvs of job
    i->job_argv = job_argv; 
    i->argc = job_argc;
    // Store the socket
    i->socket = job_socket;


    // Make the JobID string
    char* sid = my_itoa(server->total_jobs);
    i->jobID = malloc((5+strlen(sid))*sizeof(char));
    strcpy(i->jobID, "job_");
    i->jobID = strcat(i->jobID, sid);
    free(sid);


    int pos = (server->front + server->queued) % server->size;
    
    server->job_queue[pos] = i;
    
    server->queued++;
    server->total_jobs++;

    // Unlock

}

char* server_setConcurrency(Server server, int new_conc)
{
    // Lock the mutex
    server->concurrency = new_conc;
    // Unlock

}


char* server_stop(Server server, char* id)
{
    // Lock the mutex
    for(int i = 0; i < server->size; i++)
    {
        if (server->job_queue[i] != NULL)
        {
            if (strcmp(server->job_queue[i]->jobID, id) == 0)
            {
                // Destroy it
                server->job_queue[i] = NULL;
                // Reorder
                break;
            }
        }
    }    
    
    // Unlock
}

char* server_poll(Server server, char* id)
{
    // Lock the mutex
    
    for(int i = 0; server->queued > i; i++)
    {
        JobInstance job = server->job_queue[(server->front + i) % server->size];
        // Constract string
    }

    // Unlock the mutex

}

int server_exit()
{

}
