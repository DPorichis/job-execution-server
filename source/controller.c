#include "controller.h"
#include "utils.h"

#include <string.h>

// Functions implementing server opperation
// not public facing thus not included in controller.h

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
        command_string = malloc(request_char_num);
        printf("%d\n", request_char_num);
        if(read(sock, command_string, request_char_num * sizeof(char)) < 0)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }

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
        
        // for(int i = 0; i < request_argc; i++)
        //     printf("%s ", request_argv[i]);
        // printf("\n");  

    }
    char* response_message = NULL;
    int number_of_char_response = 0;
    switch (cmd) 
    {
        case ISSUE_JOB: ;
            response_message = server_issueJob(server, request_argv ,request_argc, sock);
            break;
        case SET_CONCURRENCY:
            response_message = server_setConcurrency(server, my_atoi(request_argv[0]));
            break;
        case STOP:
            response_message = server_stop(server, request_argv[0]);
            break;
        case POLL:;
            server_poll(server);
            break;
        case EXIT:
            response_message = "jobExecutorServer terminated\n";
            break;
        default:
            // That doesn't seem right
            response_message = "Unexpected Error\n";           
    }

    number_of_char_response = strlen(response_message) + 1;

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
        free(response_message);
    }

}

void* wrapper_controller(void * arg)
{
    ControllerArgs cntrl = arg;
    controller(cntrl->server, cntrl->sock);
    free(arg);
}