#include "controller.h"
#include "utils.h"

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

    // If we have arguments to read aswell
    if(request_char_num !=0)
    {
        command_string = malloc(request_char_num);
        if(read(sock, request, sizeof(request_char_num)) < 0)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
        
    }
    char* response_message = NULL;
    int number_of_char_response = 0;
    switch (cmd) 
    {
        case ISSUE_JOB: ;
            // Insert the job in the queue
            char* request_argv[3];
            server_issueJob(server, request_argv ,request_argc, sock);
            break;
        case SET_CONCURRENCY:
            server_setConcurrency(server, 3);
            break;
        case STOP:
            server_stop(server, "hahahaha");
            break;
        case POLL:;
            server_poll(server, "hahhaha");
            break;
        case EXIT:
            response_message = "jobExecutorServer terminated\n";
            number_of_char_response = strlen(response_message) + 1;
            break;
        default:
            // That doesn't seem right
            response_message = "Unexpected Error\n";
            number_of_char_response = strlen(response_message) + 1;            
    }





}

int controller_wrapper()
{

}
