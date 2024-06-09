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


#include "helpfunc.h"
#include "utils.h"
#include "controller.h"
#include "worker.h"

Server server = NULL;

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        perror("Wrong Use");
        exit(EXIT_FAILURE);
    }

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

    while(1)
    {
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

        if(err = pthread_create(&thr, NULL, wrapper_controller, args))
        {
            fprintf(stderr, "pthread create");
            exit(1);
        }

    }

}
