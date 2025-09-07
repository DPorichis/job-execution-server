// jobCommander.c : Contains the implementation of the client
// NOTE: The majority of code is reused from project 1

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

    // Reject basic mistakes in syntax
    // And assign the corresponding code of the command

    if(argc < 4)
    {
        printf("Wrong arguments, use: ./jobCommander <server_address> <port> <action>\n");
        return -2;
    }

    enum command cmd;
    if(strcmp(argv[3], "issueJob") == 0)
    {
        if(argc == 4)
        {
            printf("Missing command after issueJob actions.\n Use: ./jobCommander <server_address> <port> issueJob <command> [args...]\n");
            return -2;
        }
        cmd = ISSUE_JOB;
    }
    else if(strcmp(argv[3], "setConcurrency") == 0)
    {
        if(argc != 5)
        {
            printf("Wrong usage of setConcurrency. Use: ./jobCommander <server_address> <port> setConcurrency <number>\n");
            return -2;
        }
        cmd = SET_CONCURRENCY;  
    }
    else if(strcmp(argv[3], "stop") == 0) 
    {
        if(argc != 5)
        {
            printf("Wrong usage of stop. Use: ./jobCommander <server_address> <port> stop <job_id>\n");
            return -2;
        }
        cmd = STOP;
    }
    else if(strcmp(argv[3], "poll")== 0)
    {
        if(argc != 4)
        {
            printf("Wrong usage of poll. Use: ./jobCommander <server_address> <port> poll\n");
            return -2;
        }
        cmd = POLL;
    }
    else if(strcmp(argv[3], "exit")== 0)
    {
        if(argc != 4)
        {
            printf("Wrong usage of exit. Use: ./jobCommander <server_address> <port> exit\n");
            return -2;
        }
        cmd = EXIT;
    }
    else
    {
        printf("Command unknown, exiting...\n");
        exit(-2);
    }

    // Time to connect to the server and send our request
    
    // -- From lecture code --
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

    port = my_atoi(argv[2]); // Port specified by our user
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port);

    if(connect(sock, serverptr, sizeof(server)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

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
            copying_string += strlen(argv[a])+1;
        }

        //Number of sent arguments excluding ./jobCommander [server Name] [portNum] <Command>
        command_argc = argc - 4;

    }
    // Copy the request information to their according spots in the request table
    memcpy(request, &command_argc, sizeof(int));
    memcpy(request+1, &number_of_characters, sizeof(int));
    memcpy(request+2, &cmd, sizeof(enum command));
    
    // Send out the request
    if(write(sock, request, sizeof(request)) < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }

    // And the string representation of the rest of the arguments (if we have any)
    if(number_of_characters !=0)
    {
        if(write(sock, transmit, number_of_characters*sizeof(char)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
        free(transmit);
    }

    if (shutdown(sock, SHUT_WR) < 0) {
        perror("shutdown failed");
        exit(EXIT_FAILURE);
    }

    // Now we wait for server's response

    // Optain the length of the response
    int response_size = 0;
    if(read(sock, &response_size, sizeof(int)) < 0)
    {
        perror("problem in reading");
        exit(5);
    }
    
    // If there is a message optain it as well
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

    // For issue job commands we will need the output of the job
    if(cmd == ISSUE_JOB)
    {
        // The output will be sent in packs of 256-characters
        // As the size may be too big for one continuous message
        do
        {
            char buffer[257];
            response_size = 0;

            // Optain how many actual characters are in this pack
            if(read(sock, &response_size, sizeof(int)) < 0)
            {
                perror("problem in reading");
                exit(5);
            }
            // Place a Null byte in the buffer so we can print the contain without issues
            buffer[response_size] = '\0';
            if(response_size!=0)
            {
                // Read it and print it out
                if(read(sock, buffer, response_size) < 0)
                {
                    perror("problem in reading");
                    exit(5);
                }
                printf("%s", buffer);
            }
        // If we recieve a size of zero the message is over
        }while (response_size != 0);
        
    }

    // The communication is officially over
    // Clean and exit

    close(sock);
    return 0;
}

