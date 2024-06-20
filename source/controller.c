// controller.c : Contains the function used by the controller
// threads (and its wrapper)

#include "controller.h"
#include "utils.h"

#include <string.h>

int controller(Server server, int sock)
{
    // Read the request from the specified socket
    int request[3];
    if(read(sock, request, sizeof(request)) < 0)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }

    int request_argc = request[0];
    int request_char_num = request[1];
    enum command cmd = (enum command)request[2];
    char * command_string = NULL;

    char** request_argv = NULL;

    printf("Request recieved\n");
    
    // If we have arguments to read aswell
    if(request_char_num !=0)
    {
        // Retrive the string representation of the job
        command_string = malloc(request_char_num);
        printf("%d\n", request_char_num);
        if(read(sock, command_string, request_char_num * sizeof(char)) < 0)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }

        // Recreate an argv table representation
        request_argv = malloc(sizeof(char*)*(request_argc + 1));
        int offset = 0;
        for(int i = 0; i < request_argc; i++)
        {
            int len_of_arg = (strlen(command_string+offset)+1);
            request_argv[i] = malloc(len_of_arg*sizeof(char));
            strcpy(request_argv[i], command_string+offset);
            offset += len_of_arg;
            printf("%s ", request_argv[i]);
        }
        request_argv[request_argc] = NULL;
        
        free(command_string);

    }


    // Depending on the type of the request, call the relevant server_*function*
    // Note: These functions contain locking logic, the caller may be suspended
    // for arbitrary time due to these locks.

    char* response_message = NULL;
    int number_of_char_response = 0;
    switch (cmd) 
    {
        case ISSUE_JOB: ;
            server_issueJob(server, request_argv ,request_argc, sock);
            break;
        case SET_CONCURRENCY:
            response_message = server_setConcurrency(server, my_atoi(request_argv[0]));
            break;
        case STOP:
            response_message = server_stop(server, request_argv[0]);
            break;
        case POLL:;
            response_message = server_poll(server);
            break;
        case EXIT:
            response_message = server_exit(server);
            break;
        default:
            // That doesn't seem right
            response_message = "Unexpected Error\n";           
    }

    // For ISSUE_JOB the server_issueJob function will communicate with the client,
    // For all the other commands, we need to send the message ourselves
    if(cmd != ISSUE_JOB)
    {
        number_of_char_response = strlen(response_message) + 1;

        // Send out the length of the response
        if(write(sock, &number_of_char_response, sizeof(int)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }

        // And the response itself
        if(number_of_char_response !=0)
        {
            if(write(sock, response_message, number_of_char_response*sizeof(char)) < 0)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }
            free(response_message);
        }

        if (shutdown(sock, SHUT_WR) < 0) {
            perror("shutdown failed");
            exit(EXIT_FAILURE);
        }

        close(sock);
        
        if(request_argv != NULL)
        {
            for(int i = 0; i < request_argc; i++)
                free(request_argv[i]);
            free(request_argv);
        }
    }

}


// Wrapper function for dereferencing the arguments.
void* wrapper_controller(void * arg)
{
    ControllerArgs cntrl = arg;
    controller(cntrl->server, cntrl->sock);
    free(arg);
    // printf("Controller Exits\n");
    // fflush(stdout);
    pthread_exit(0);
}