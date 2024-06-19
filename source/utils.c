// utils.c: Contains the implimentation of core server functions.
// All the heavy-lifting of locking logic is here

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

// Creates a server representation with the specifications given as arguments
Server server_create(int bufsize, int concurrency, int worker_num, pthread_t main)
{
    Server serv = malloc(sizeof(*serv));

    // Setting up the buffer
    serv->job_queue = malloc(sizeof(JobInstance)*bufsize);
    for(int i = 0; i < bufsize; i++)
        serv->job_queue[i] = NULL;

    serv->size = bufsize; // Size of the buffer
    serv->front = 0; // First job is at pos 0
    serv->queued = 0; // The buffer is empty
    serv->total_jobs = 1; // The ids start from 1

    serv->running_now = worker_num; // All the worker threads are running
    serv->concurrency = concurrency;

    serv->exiting = 0; // We are not exiting
    serv->alive_threads = worker_num; // Number of active threads in the process
    serv->accepting_connections = 1; // The listening socket is on

    // Initializing condition variables and the mutex
    pthread_cond_init(&serv->alert_worker, NULL);
    pthread_cond_init(&serv->alert_controller, NULL);
    pthread_cond_init(&serv->alert_exiting, NULL);
    pthread_mutex_init(&serv->mtx, NULL);

    serv->main_thread = main; // Storing main thread to signal exiting when needed

    return serv;
}

int server_issueJob(Server server, char* job_argv[], int job_argc, int job_socket)
{
    // Accessing shared data, lock the mutex
    pthread_mutex_lock(&server->mtx);

    // If there is no storage available to store the job, and we are not exiting
    // sleep untill someone alerts us.
    while(server->queued >= server->size && server->exiting == 0)
        pthread_cond_wait(&server->alert_controller, &server->mtx);
    
    char* response_message = NULL;

    if(server->exiting == 1)
    {
        // Exiting Occured, stopping
        response_message = malloc(sizeof(char) * strlen("SERVER TERMINATED BEFORE EXECUTION\n") + 1);
        strcpy(response_message, "SERVER TERMINATED BEFORE EXECUTION\n");
        // Free up the space of job as it wont be used
        for(int i = 0; i < job_argc; i++)
            free(job_argv[i]);
        free(job_argv);
    }
    else
    {    
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

        // Insert the job at the end of the buffer
        int pos = (server->front + server->queued) % server->size;
        server->job_queue[pos] = i;
        
        // Update the counters
        server->queued++;
        server->total_jobs++;

        // Reconstruct job as a string for the response message
        int job_length = 0;
        for(int j = 0; j < job_argc; j++)
            job_length += strlen(job_argv[j] + 1);

        char* job = malloc(sizeof(char) * job_length);
        
        strcpy(job,job_argv[0]);    
        for(int j = 1; j < job_argc; j++)
        {
            strcat(job, " ");
            strcat(job, job_argv[j]);
        }

        // Formating the respones
        response_message  = malloc(strlen("JOB <, > SUBMITTED\n") + strlen(i->jobID) + strlen(job));
        sprintf(response_message, "JOB <%s, %s> SUBMITTED\n", i->jobID, job);

        free(job);
    }

    
    // Delivering the response length to client
    int number_of_char_response = strlen(response_message) + 1;
    if(write(job_socket, &number_of_char_response, sizeof(int)) < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }

    // And the response itself
    if(number_of_char_response !=0)
    {
        // Send them using our private communication
        if(write(job_socket, response_message, number_of_char_response*sizeof(char)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }

    free(response_message);

    // If there is space availabe for execution, alert a worker to take the job
    if(server->running_now < server->concurrency)
        pthread_cond_signal(&server->alert_worker);

    server->alive_threads--; // Note that we are done as a thread

    // If the server is on an exiting status and we where the last thread standing,
    // alert the exiting function that we are done
    if(server->alive_threads == 0 && server->exiting == 1)
        pthread_cond_signal(&server->alert_exiting);

    // End of accessing shared data
    pthread_mutex_unlock(&server->mtx);

    return 0;

}

char* server_setConcurrency(Server server, int new_conc)
{
    // Accessing shared data, lock the mutex
    pthread_mutex_lock(&server->mtx);

    server->concurrency = new_conc; // set new concurrency

    // If we have bigger concurrency than before and there are jobs queued
    // alert all waiting workers for a chance to get a job
    if(server->running_now < server->concurrency && server->queued != 0)
        pthread_cond_broadcast(&server->alert_worker);
    
    server->alive_threads--; // Note that we are done as a thread

    // If the server is on an exiting status and we where the last thread standing,
    // alert the exiting function that we are done
    if(server->alive_threads == 0 && server->exiting == 1)
        pthread_cond_signal(&server->alert_exiting);

    // End of accessing shared data
    pthread_mutex_unlock(&server->mtx);

    // Construct and return the response
    char* conc_string = my_itoa(new_conc);
    char* response_message  = malloc(strlen("CONCURRENCY SET AT ") + strlen(conc_string) + 1);
    sprintf(response_message, "CONCURRENCY SET AT %s\n", conc_string);
    free(conc_string); 

    return response_message;
}


char* server_stop(Server server, char* id)
{
    // Accessing shared data, lock the mutex
    pthread_mutex_lock(&server->mtx);

    char* response_message = NULL;
    
    // Search all the queued jobs
    for(int i = 0; i < server->queued; i++)
    {
        int pos = (server->front + i) % server->size;
        if (server->job_queue[pos] != NULL)
        {
            // To find the requested jobID
            if (strcmp(server->job_queue[pos]->jobID, id) == 0)
            {
                // Destroy it                
                destroy_instance(server->job_queue[pos]);
                server->job_queue[pos] = NULL;

                // Reorder the positioning so it is continius again
                for(int j = i; j < server->queued - 1; j++)
                {
                    pos = (server->front + j) % server->size;
                    int mov_pos = (server->front + j + 1) % server->size;
                    printf("moving %d <- %d", pos, mov_pos);
                    fflush(stdout);
                    server->job_queue[pos] = server->job_queue[mov_pos];
                    server->job_queue[mov_pos] = NULL;
                }
                // Construct the response
                response_message = malloc(sizeof(char) * (strlen(id) + strlen("JOB <> REMOVED\n")));
                sprintf(response_message, "JOB <%s> REMOVED\n", id);
            
                server->queued--;
                break;
            }
        }
    }    
    
    // If the job wasn't found
    if(response_message == NULL)
    {
        // Construct failure response
        response_message = malloc(sizeof(char) * (strlen(id) + strlen("JOB <> NOTFOUND\n")));
        sprintf(response_message, "JOB <%s> NOTFOUND\n", id);
    }

    // If the buffer was full before this deletion, alert a waiting controller to place a job
    if(server->size - 1 == server->queued)
        pthread_cond_signal(&server->alert_controller);

    server->alive_threads--; // Note that we are done as a thread

    // If the server is on an exiting status and we where the last thread standing,
    // alert the exiting function that we are done
    if(server->alive_threads == 0 && server->exiting == 1)
        pthread_cond_signal(&server->alert_exiting);

    // End of accessing shared data
    pthread_mutex_unlock(&server->mtx);
    
    return response_message;

}

char* server_poll(Server server)
{
    // Accessing shared data, lock the mutex
    pthread_mutex_lock(&server->mtx);
    
    // Create a table to store the string tupples of the jobs
    char** poll_buffer = malloc(sizeof(char *) * server->queued);
    int total_length = 0;

    // For every job queued
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

        // Format the tupple
        char* response_message  = malloc(strlen("<, >\n") + strlen(job->jobID) + strlen(argv_str));
        sprintf(response_message, "<%s, %s>\n", job->jobID, argv_str);

        // And store it in the buffer
        poll_buffer[i] = response_message;        
        total_length += strlen(response_message);
        free(argv_str);
    }

    char* response;

    // If no jobs where found return the empty message
    if(total_length == 0)
    {
        response = malloc(sizeof(char) * (strlen("NO JOB AT QUEUE\n") + 1));
        strcpy(response, "NO JOB AT QUEUE\n");
    }
    else
    {
        // If jobs where found create a response containing all the job tupples
        response = malloc(sizeof(char) * (total_length + 1));    
        strcpy(response, poll_buffer[0]);
        free(poll_buffer[0]);
        for(int i = 1; i < server->queued; i++)
        {
            strcat(response, poll_buffer[i]);
            free(poll_buffer[i]);
        }
    }
    free(poll_buffer);
    
    server->alive_threads--; // Note that we are done as a thread

    // If the server is on an exiting status and we where the last thread standing,
    // alert the exiting function that we are done
    if(server->alive_threads == 0 && server->exiting == 1)
        pthread_cond_signal(&server->alert_exiting);

    // End of accessing shared data
    pthread_mutex_unlock(&server->mtx);

    return response;

}

char* server_exit(Server server)
{
    // Accessing shared data, lock the mutex
    pthread_mutex_lock(&server->mtx);

    if(server->exiting == 1) // If the exiting process has already started by someone else, dont do anything
    {
        char* response = malloc(sizeof(char) * (strlen("SERVER ALREADY TERMINATING\n") + 1));
        sprintf(response, "SERVER ALREADY TERMINATING\n");
        server->alive_threads--;
        pthread_mutex_unlock(&server->mtx);
        return response;
    }


    pthread_kill(server->main_thread, SIGUSR1); // Signal the mainthread to close the socket
    
    server->exiting = 1; // Indicate Exiting status

    server->alive_threads--;
    
    // Stop all queued jobs
    for(int i = 0; i < server->size; i++)
    {
        // The message that will be sent to the waiting clients
        char* response_message = "SERVER TERMINATED BEFORE EXECUTION\n";
        int number_of_char_response = strlen(response_message) + 1;

        // For every actual job
        if(server->job_queue[i] != NULL)
        {
            // Send message length to client
            int sock = server->job_queue[i]->socket;
            if(write(sock, &number_of_char_response, sizeof(int)) < 0)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }

            // And the termination message
            if(number_of_char_response !=0)
            {
                // Send them using our private communication
                if(write(sock, response_message, number_of_char_response*sizeof(char)) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }

            // Indicate end of communication
            number_of_char_response = 0;
            if(write(sock, &number_of_char_response, sizeof(int)) < 0)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }
        
            // Destroy the job
            destroy_instance(server->job_queue[i]);


            server->job_queue[i] = NULL;
        }
    }
    server->front = 0;
    server->queued = 0;
    
    // Wake-up everyone to let them know we are exiting
    pthread_cond_broadcast(&server->alert_controller);
    pthread_cond_broadcast(&server->alert_worker);

    // Waiting for all threads to stop
    while(server->alive_threads != 0 || server->accepting_connections == 1)
    {
        printf("%d threads still alive\n", server->alive_threads);
        fflush(stdout);
        pthread_cond_wait(&server->alert_exiting, &server->mtx);
    }
    
    pthread_mutex_unlock(&server->mtx);

    server_destroy(server);
    server = NULL;

    // Create the response string and return it
    char* response = malloc(sizeof(char) * (strlen("SERVER TERMINATED\n") + 1));
    sprintf(response, "SERVER TERMINATED\n");
    return response;
}


void server_destroy(Server server)
{
    // Free up the job_queue
    free(server->job_queue);

    // Destroy the conditions and mutex
    pthread_cond_destroy(&server->alert_worker);
    pthread_cond_destroy(&server->alert_controller);
    pthread_cond_destroy(&server->alert_exiting);
    pthread_mutex_destroy(&server->mtx);

    // Free the struct itself
    free(server);

}

JobInstance server_getJob(Server server)
{
    // Accessing shared data, lock the mutex
    pthread_mutex_lock(&server->mtx);

    // If we are searching for a job, we are not a running worker
    server->running_now--;

    // If a job is available and we have permission to take it OR we are exiting
    while((server->queued == 0 || server->concurrency <= server->running_now) && server->exiting == 0)
        pthread_cond_wait(&server->alert_worker, &server->mtx);
    
    // If server is exiting is time to terminate
    if(server->exiting == 1)
    {
        server->alive_threads--; // Note that we are done as a thread

        // If the server is on an exiting status and we where the last thread standing,
        // alert the exiting function that we are done

        if(server->alive_threads == 0)
            pthread_cond_signal(&server->alert_exiting);
        
        // End of accessing shared data
        pthread_mutex_unlock(&server->mtx);
        return NULL;
    }

    // Get the first job available
    JobInstance job = server->job_queue[server->front];

    // Mark the spot as empty and shift the front
    server->job_queue[server->front] =  NULL;
    server->front = (server->front + 1) % server->size;
    server->queued--;

    // Mark that a job is now running
    server->running_now++;

    // If the buffer was full before, let a waiting controller know that he can store a job
    if(server->size - 1 == server->queued)
        pthread_cond_signal(&server->alert_controller);
    pthread_mutex_unlock(&server->mtx);

    return job;
}



void destroy_instance(JobInstance job)
{
    free(job->jobID);
    for(int i = 0; i < job->argc; i++)
        free(job->job_argv[i]);
    free(job->job_argv);
    free(job);
}