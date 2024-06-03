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

Server serv = NULL;

int main(int argc, char* argv[])
{

    int port, sock, newsock;
    struct sockaddr_in server, client;
    socklen_t clientlen;

    struct sockaddr *serverptr = (struct sockaddr *)&server;
    struct sockaddr *clientptr = (struct sockaddr *)&client;
    struct hostent *rem;

    if (argc != 4)
    {
        perror("Wrong Use");
        exit(EXIT_FAILURE);
    }

    port = my_atoi(argv[1]);
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    serv = server_create(my_atoi(argv[2]), 1);

    int threadpoolsize = my_atoi(argv[3]);
    for(int i = 0; i < threadpoolsize; i++)
    {
        pthread_t thr;
        int err;
        if(err = pthread_create(&thr, NULL, wrapper_worker, NULL))
        {
            fprintf(stderr, "pthread create");
            exit(1);
        }
    }


    if(bind(sock, serverptr, sizeof(server)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if(listen(sock, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Now listening for connections to port %d\n", port);

    while(1)
    {
        if ((newsock = accept(sock, clientptr, &clientlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("Connection Accepted\n");
        
        pthread_t thr;
        int err, status;

        ControllerArgs args = malloc(sizeof(*args));
        args->server = serv;
        args->sock = newsock;

        if(err = pthread_create(&thr, NULL, wrapper_controller, args))
        {
            fprintf(stderr, "pthread create");
            exit(1);
        }

    }

}
