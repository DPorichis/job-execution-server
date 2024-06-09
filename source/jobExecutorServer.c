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
#include <pthread.h>
#include <string.h>


#include "helpfunc.h"
#include "utils.h"
#include "controller.h"
#include "worker.h"

Server server = NULL;

void handle_exiting(int sig);

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        perror("Wrong Use");
        exit(EXIT_FAILURE);
    }

    // Setting up the termination signal
    static struct sigaction act_child_term;
    act_child_term.sa_handler = handle_exiting;
    sigfillset(&(act_child_term.sa_mask));
    sigaction(SIGUSR1, &act_child_term, NULL);

    int buffer_size = my_atoi(argv[2]);
    int threadpoolsize = my_atoi(argv[3]);
    server = server_create(buffer_size, 1, threadpoolsize, pthread_self());

    struct sockaddr *serverptr = (struct sockaddr *)&server->serv;
    struct sockaddr *clientptr = (struct sockaddr *)&server->client;
    struct hostent *rem;

    server->port = my_atoi(argv[1]);
    if ((server->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server->serv.sin_family = AF_INET;
    server->serv.sin_addr.s_addr = htonl(INADDR_ANY);
    server->serv.sin_port = htons(server->port);

    for(int i = 0; i < threadpoolsize; i++)
    {
        pthread_t thr;
        int err;
        if(err = pthread_create(&thr, NULL, wrapper_worker, server))
        {
            fprintf(stderr, "pthread create");
            exit(1);
        }
        pthread_detach(thr);
    }


    if(bind(server->sock, serverptr, sizeof(*serverptr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if(listen(server->sock, 10) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }


    printf("Now listening for connections to port %d\n", server->port);
    int newsock;

    // Creating a signal set that includes only SIGCHLD
    // to activate while we are in critical parts
    sigset_t ignore_child;
    sigemptyset(&ignore_child);
    sigaddset(&ignore_child, SIGUSR1);

    while(1)
    {
        sigprocmask(SIG_UNBLOCK, &ignore_child, NULL);

        if ((newsock = accept(server->sock, clientptr, &server->clientlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("Connection Accepted\n");

        
        
        pthread_t thr;
        int err, status;

        ControllerArgs args = malloc(sizeof(*args));
        args->server = server;
        args->sock = newsock;

        //DO NOT INTERRUPT WITH SIGNALS WHILE LOCK IS ON
        sigprocmask(SIG_BLOCK, &ignore_child, NULL);

        pthread_mutex_lock(&server->mtx);

        if(err = pthread_create(&thr, NULL, wrapper_controller, args))
        {
            fprintf(stderr, "pthread create");
            exit(1);
        }
        // The worker threads are stand-alone
        pthread_detach(thr);

        server->alive_threads++;

        pthread_mutex_unlock(&server->mtx);

    }

}




void handle_exiting(int sig)
{
    pthread_mutex_lock(&server->mtx);

    // Stop all queued jobs
    for(int i = 0; i < server->size; i++)
    {
        char* response_message = "SERVER TERMINATED BEFORE EXECUTION\n";
        int number_of_char_response = strlen(response_message) + 1;

        if(server->job_queue[i] != NULL)
        {
            int sock = server->job_queue[i]->socket;
            if(write(sock, &number_of_char_response, sizeof(int)) < 0)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }

            // If we have arguments to send aswell
            if(number_of_char_response !=0)
            {
                // Send them using our private communication
                if(write(sock, response_message, number_of_char_response*sizeof(char)) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }

            number_of_char_response = 0;

            if(write(sock, &number_of_char_response, sizeof(int)) < 0)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }
        
            // Destroy the job
            
            server->job_queue[i] = NULL;
        }
    }
    server->front = 0;
    server->queued = 0;

    printf("Letting all paused threads know we are exiting\n");
    fflush(stdout);
    
    // Wake-up everyone to let thme know we are exiting
    pthread_cond_broadcast(&server->alert_controller);
    pthread_cond_broadcast(&server->alert_worker);

    while(server->alive_threads != 0)
    {
        printf("%d threads still alive\n", server->alive_threads);
        fflush(stdout);
        pthread_cond_wait(&server->alert_exiting, &server->mtx);
    }
    // Everyone left, lets destroy that thing

    close(server->sock);

    pthread_mutex_unlock(&server->mtx);

    printf("main thread exiting\n");
    fflush(stdout);

    server_destroy(server);
    server = NULL;
    exit(0);

}