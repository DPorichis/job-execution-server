#include "utils.h"
#include "helpfunc.h"

#include <stdlib.h>
#include <string.h>


Server server_create(int bufsize, int concurrency, int worker_num)
{
    Server serv = malloc(sizeof(*serv));
    serv->front = 0;
    serv->queued = 0;
    serv->running_now = worker_num;
    serv->total_jobs = 1;
    serv->size = bufsize;
    serv->concurrency = concurrency;
    serv->exiting = 0;

    serv->job_queue = malloc(sizeof(JobInstance)*bufsize);
    for(int i = 0; i < bufsize; i++)
        serv->job_queue[i] = NULL;
    
    pthread_cond_init(&serv->alert_worker, NULL);
    pthread_cond_init(&serv->alert_controller, NULL);

    pthread_mutex_init(&serv->mtx, NULL);

    return serv;
}

char* server_issueJob(Server server, char* job_argv[], int job_argc, int job_socket)
{
    // Lock the mutex
    pthread_mutex_lock(&server->mtx);

    while(server->queued >= server->size)
        pthread_cond_wait(&server->alert_controller, &server->mtx);
    
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

    if(server->running_now < server->concurrency)
        pthread_cond_signal(&server->alert_worker);
    pthread_mutex_unlock(&server->mtx);

}

char* server_setConcurrency(Server server, int new_conc)
{
    pthread_mutex_lock(&server->mtx);

    server->concurrency = new_conc;

    if(server->running_now < server->concurrency && server->queued != 0)
        pthread_cond_signal(&server->alert_worker);
    pthread_mutex_unlock(&server->mtx);

}


char* server_stop(Server server, char* id)
{
    pthread_mutex_lock(&server->mtx);
    
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
    
    server->queued--;

    if(server->size - 1 != server->queued)
        pthread_cond_signal(&server->alert_controller);
    
    pthread_mutex_unlock(&server->mtx);
    
}

char* server_poll(Server server)
{
    pthread_mutex_lock(&server->mtx);
    
    for(int i = 0; server->queued > i; i++)
    {
        JobInstance job = server->job_queue[(server->front + i) % server->size];
        // Constract string
    }

    pthread_mutex_unlock(&server->mtx);

}

int server_exit()
{

}


void server_destroy(Server server)
{

}

JobInstance server_getJob(Server server)
{
    pthread_mutex_lock(&server->mtx);

    server->running_now--;

    while(server->queued == 0 || server->concurrency >= server->running_now)
        pthread_cond_wait(&server->alert_worker, &server->mtx);
        
    // Get your job

    server->queued--;
    server->running_now++;


    if(server->size - 1 == server->queued)
        pthread_cond_signal(&server->alert_controller); 
    pthread_mutex_unlock(&server->mtx);
}