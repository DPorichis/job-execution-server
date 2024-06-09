#include "utils.h"
#include "helpfunc.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>

Server server_create(int bufsize, int concurrency, int worker_num, pthread_t main)
{
    Server serv = malloc(sizeof(*serv));
    serv->front = 0;
    serv->queued = 0;
    serv->running_now = worker_num;
    serv->total_jobs = 1;
    serv->size = bufsize;
    serv->concurrency = concurrency;
    serv->exiting = 0;
    serv->main_thread = 0;
    serv->alive_threads = worker_num;

    serv->job_queue = malloc(sizeof(JobInstance)*bufsize);
    for(int i = 0; i < bufsize; i++)
        serv->job_queue[i] = NULL;
    
    pthread_cond_init(&serv->alert_worker, NULL);
    pthread_cond_init(&serv->alert_controller, NULL);
    pthread_cond_init(&serv->alert_exiting, NULL);
    pthread_mutex_init(&serv->mtx, NULL);

    serv->main_thread = main;

    return serv;
}

char* server_issueJob(Server server, char* job_argv[], int job_argc, int job_socket)
{
    // Lock the mutex
    pthread_mutex_lock(&server->mtx);

    while(server->queued >= server->size && server->exiting == 0)
        pthread_cond_wait(&server->alert_controller, &server->mtx);
    
    char* response_message = NULL; 
    if(server->exiting == 1)
    {
        // Exiting Occured, stopping
        response_message = malloc(sizeof(char) * strlen("SERVER TERMINATED BEFORE EXECUTION\n") + 1);
        strcpy(response_message, "SERVER TERMINATED BEFORE EXECUTION\n");
    }
    else
    {
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

        int job_length = 0;
        // Reconstruct job as a string
        for(int j = 0; j < job_argc; j++)
            job_length += strlen(job_argv[j] + 1);

        char* job = malloc(sizeof(char) * job_length);
        
        strcpy(job,job_argv[0]);    
        for(int j = 1; j < job_argc; j++)
        {
            strcat(job, " ");
            strcat(job, job_argv[j]);
        }


        response_message  = malloc(strlen("JOB <, > SUBMITTED\n") + strlen(i->jobID) + strlen(job));
        sprintf(response_message, "JOB <%s, %s> SUBMITTED\n", i->jobID, job);

        printf("%d / %d", server->running_now, server->concurrency);
        fflush(stdout);

    }

    

    int number_of_char_response = strlen(response_message) + 1;

    if(write(job_socket, &number_of_char_response, sizeof(int)) < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }

    // If we have arguments to send aswell
    if(number_of_char_response !=0)
    {
        // Send them using our private communication
        if(write(job_socket, response_message, number_of_char_response*sizeof(char)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }

    if(server->running_now < server->concurrency)
        pthread_cond_signal(&server->alert_worker);

    server->alive_threads--;
    if(server->alive_threads == 0 && server->exiting == 1)
        pthread_cond_signal(&server->alert_exiting);

    pthread_mutex_unlock(&server->mtx);

    return response_message;

}

char* server_setConcurrency(Server server, int new_conc)
{
    pthread_mutex_lock(&server->mtx);

    server->concurrency = new_conc;

    if(server->running_now < server->concurrency && server->queued != 0)
        pthread_cond_signal(&server->alert_worker);
    pthread_mutex_unlock(&server->mtx);

    char* conc_string = my_itoa(new_conc);
    char* response_message  = malloc(strlen("CONCURRENCY SET AT ") + strlen(conc_string) + 1);

    sprintf(response_message, "CONCURRENCY SET AT %s\n", conc_string);

    server->alive_threads--;
    if(server->alive_threads == 0 && server->exiting == 1)
        pthread_cond_signal(&server->alert_exiting);

    return response_message;
}


char* server_stop(Server server, char* id)
{
    pthread_mutex_lock(&server->mtx);

    char* response_message = NULL;
    
    for(int i = 0; i < server->queued; i++)
    {
        int pos = (server->front + i) % server->size;
        
        if (server->job_queue[pos] != NULL)
        {
            if (strcmp(server->job_queue[i]->jobID, id) == 0)
            {
                // Destroy it
                server->job_queue[pos] = NULL;
                // Reorder the positioning so it is continius again
                for(int j = i; j < server->queued; j++)
                {
                    pos = (server->front + j) % server->size;
                    int mov_pos = (server->front + j + 1) % server->size;
                    server->job_queue[pos] = server->job_queue[mov_pos];
                    server->job_queue[mov_pos] = NULL;
                }
                response_message = malloc(sizeof(char) * (strlen(id) + strlen("JOB <> REMOVED\n")));
                sprintf(response_message, "JOB <%s> REMOVED\n", id);
            
                server->queued--;
                break;
            }
        }
    }    
    
    if(response_message == NULL)
    {
        response_message = malloc(sizeof(char) * (strlen(id) + strlen("JOB <> NOTFOUND\n")));
        sprintf(response_message, "JOB <%s> NOTFOUND\n", id);
    }

    if(server->size - 1 != server->queued)
        pthread_cond_signal(&server->alert_controller);

    server->alive_threads--;
    if(server->alive_threads == 0 && server->exiting == 1)
        pthread_cond_signal(&server->alert_exiting);
    
    pthread_mutex_unlock(&server->mtx);
    
    return response_message;

}

char* server_poll(Server server)
{
    pthread_mutex_lock(&server->mtx);
    
    char** poll_buffer = malloc(sizeof(char *) * server->queued);
    int total_length = 0;

    printf("check\n");
    fflush(stdout);

    for(int i = 0; i < server->queued; i++)
    {
        JobInstance job = server->job_queue[(server->front + i) % server->size];
        
        int job_length = 0;
        // Reconstruct job as a string
        for(int j = 0; j < job->argc; j++)
            job_length += strlen(job->job_argv[j]) + 1;

        char* argv_str = malloc(sizeof(char) * job_length);
    
        strcpy(argv_str,job->job_argv[0]);    
        for(int j = 1; j < job->argc; j++)
        {
            strcat(argv_str, " ");
            strcat(argv_str, job->job_argv[j]);
        }


        char* response_message  = malloc(strlen("<, >\n") + strlen(job->jobID) + strlen(argv_str));
        sprintf(response_message, "<%s, %s>\n", job->jobID, argv_str);

        poll_buffer[i] = response_message;        
        total_length += strlen(response_message);
        free(argv_str);
    }

    printf("check\n");
    fflush(stdout);

    char* response;

    if(total_length == 0)
    {
        response = malloc(sizeof(char) * (strlen("NO JOB AT QUEUE\n") + 1));
        strcpy(response, "NO JOB AT QUEUE\n");
    }
    else
    {
        response = malloc(sizeof(char) * (total_length + 1));    
        strcpy(response, poll_buffer[0]);
        for(int i = 1; i < server->queued; i++)
            strcat(response, poll_buffer[i]);
    }

    printf("check\n");
    fflush(stdout);

    server->alive_threads--;
    if(server->alive_threads == 0 && server->exiting == 1)
        pthread_cond_signal(&server->alert_exiting);

    pthread_mutex_unlock(&server->mtx);


    return response;

}

char* server_exit(Server server)
{
    pthread_mutex_lock(&server->mtx);

    pthread_kill(server->main_thread, SIGUSR1);
    // Indicate Exiting status
    server->exiting = 1;

    server->alive_threads--;
    pthread_mutex_unlock(&server->mtx);

    char* response = malloc(sizeof(char) * (strlen("SERVER TERMINATED\n") + 1));
    sprintf(response, "SERVER TERMINATED\n");
    return response;
}


void server_destroy(Server server)
{
    free(server->job_queue);

    pthread_cond_destroy(&server->alert_worker);
    pthread_cond_destroy(&server->alert_controller);
    pthread_cond_destroy(&server->alert_exiting);
    pthread_mutex_destroy(&server->mtx);

    free(server);

}

JobInstance server_getJob(Server server)
{
    printf("Checking for a job...\n");
    fflush(stdout);

    pthread_mutex_lock(&server->mtx);

    server->running_now--;

    printf("locked the mutex with running threads %d...\n", server->alive_threads);
    fflush(stdout);

    while((server->queued == 0 || server->concurrency <= server->running_now) && server->exiting == 0)
        pthread_cond_wait(&server->alert_worker, &server->mtx);
    
    if(server->exiting == 1)
    {
        server->alive_threads--;
        printf("Alive threads: %d\n", server->alive_threads);
        fflush(stdout);
        if(server->alive_threads == 0)
            pthread_cond_signal(&server->alert_exiting);
        pthread_mutex_unlock(&server->mtx);
        return NULL;
    }

    JobInstance job = server->job_queue[server->front];

    server->job_queue[server->front] =  NULL;
    server->front = (server->front + 1) % server->size;

    server->queued--;
    server->running_now++;


    if(server->size - 1 == server->queued)
        pthread_cond_signal(&server->alert_controller); 
    pthread_mutex_unlock(&server->mtx);

    return job;
}