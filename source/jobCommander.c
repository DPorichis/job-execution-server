// JobCommander code for project 2
// Dimitris-Stefanos Porichis

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


#include "utils.h"
#include "helpfunc.h"

int main(int argc, char* argv[])
{
    
    if(argc < 4)
    {
        printf("Wrong use, try again\n");
        return -2;
    }

    // Reject basic mistakes in syntax
    // And assign the corresponding code of the command
    enum command cmd;
    if(strcmp(argv[3], "issueJob") == 0)
    {
        if(argc == 4)
        {
            printf("Missing command after issueJob\n");
            return -2;
        }
        cmd = ISSUE_JOB;
    }
    else if(strcmp(argv[3], "setConcurrency") == 0)
    {
        if(argc != 5)
        {
            printf("Wrong number of arguments for setConcurrency\n");
            return -2;
        }
        cmd = SET_CONCURRENCY;  
    }
    else if(strcmp(argv[3], "stop") == 0) 
    {
        if(argc != 5)
        {
            printf("Wrong number of arguments for stop\n");
            return -2;
        }
        cmd = STOP;
    }
    else if(strcmp(argv[3], "poll")== 0)
    {
        if(argc != 4)
        {
            printf("Wrong number of arguments for poll\n");
            return -2;
        }
        cmd = POLL;
    }
    else if(strcmp(argv[3], "exit")== 0)
    {
        if(argc != 4)
        {
            printf("Wrong number of arguments for stop\n");
            return -2;
        }
        cmd = EXIT;
    }
    else
    {
        printf("Command unknown, exiting...\n");
        exit(-2);
    }

    // Connect to server

    int port, sock, i;
    char bud[256];

    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }
    if ((rem = gethostbyname(argv[1])) == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }

    port = my_atoi(argv[2]);
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port);

    printf("Connecting to %s port %d\n", argv[1], port);
    fflush(stdout);


    if(connect(sock, serverptr, sizeof(server)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    printf("Connecting to %s port %d\n", argv[1], port);
    fflush(stdout);

    // Constructing our initial request
    int request[3];
    int number_of_characters = 0;
    int command_argc = 0;
    char* transmit = NULL;
    
    // If the command requires more arguments
    if(cmd == ISSUE_JOB || cmd == SET_CONCURRENCY || cmd == STOP)
    {
        // Count the space necessary
        for(int a = 4; a < argc; a++)
        {
            number_of_characters += strlen(argv[a]) + 1;
        }
        
        // Get a big enough buffer
        transmit = malloc(sizeof(char) * number_of_characters);
        char* copying_string = transmit;
        
        //Copy the arguments inside it
        for(int a = 4; a < argc; a++) // This may be unecessary
        {
            strcpy(copying_string, argv[a]);
            printf("%s\n", copying_string);
            copying_string += strlen(argv[a])+1;
        }

        //Number of sent arguments excluding ./jobCommander [server Name] [portNum] <Command>
        command_argc = argc - 4;

    }

    memcpy(request, &command_argc, sizeof(int));
    memcpy(request+1, &number_of_characters, sizeof(int));
    memcpy(request+2, &cmd, sizeof(enum command));
    
    if(write(sock, request, sizeof(request)) < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }

    // If we have arguments to send aswell
    if(number_of_characters !=0)
    {
        // Send them using our private communication
        if(write(sock, transmit, number_of_characters*sizeof(char)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
        free(transmit);
    }

    // Wait for server's response
    int response_size = 0;
    if(read(sock, &response_size, sizeof(int)) < 0)
    {
        perror("problem in reading");
        exit(5);
    }
    
    // If server has a message to deliver
    if(response_size!=0)
    {
        // Read it and print it out
        char* response = malloc(sizeof(char)*response_size);
        if(read(sock, response, response_size) < 0)
        {
            perror("problem in reading");
            exit(5);
        }
        printf("%s", response);
        free(response);
    }    

    if(cmd == ISSUE_JOB)
    {
        // Read the result of the command aswell
        // Wait for server's response
        response_size = 0;
        if(read(sock, &response_size, sizeof(int)) < 0)
        {
            perror("problem in reading");
            exit(5);
        }
        
        // If server has a message to deliver
        if(response_size!=0)
        {
            // Read it and print it out
            char* response = malloc(sizeof(char)*response_size);
            if(read(sock, response, response_size) < 0)
            {
                perror("problem in reading");
                exit(5);
            }
            printf("%s", response);
            free(response);
        }
    
    }

    // The communication is officially over
    // Clean and exit

    close(sock);

    return 0;

}

