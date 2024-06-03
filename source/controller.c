#include "controller.h"



// Functions implementing server opperation
// not public facing thus not included in controller.h

int issueJob(char* job_argv[], int job_argc);
int setConcurrency(int new_conc);
int stop(int id);
int exit();



int controller(int sock)
{
    // Read the request from the specified socket

    int request[3];
    if(read(sock, request, sizeof(request)) < 0)
    {
        perror("write");
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
            perror("write");
            exit(EXIT_FAILURE);
        }
        
    }

    switch (cmd) 
    {
        case ISSUE_JOB: ;
            // Insert the job in the queue
            JobTupple tupple = jobQueue_insert(queue, req_argv, req_argc);
            // Convert the JobTupple in to a string in order to sent it to the commander 
            response_message  = malloc(strlen("JOB <, , >\n") + strlen(tupple->jobID) + strlen(tupple->job) + 12);
            str_malloc = 1;
            sprintf(response_message, "JOB <%s, %s, %d>\n", tupple->jobID, tupple->job, tupple->queuePosition);
            destroy_tupple(tupple);
            number_of_char_response = strlen(response_message) + 1;
            // Check if there is space so we can execute the job
            if(jobProcessor_space_available(processor) > 0)
                child_creation();
            break;
        case SET_CONCURRENCY:
            // Set the concurrency to the new value
            jobProcessor_set_concurrency(processor ,my_atoi(req_argv[0]));
            // Check if after the change there is space for executing jobs
            if(jobProcessor_space_available(processor) > 0)
                child_creation();
            break;
        case STOP:
            if(jobQueue_remove_job(queue, req_argv[0]) == 0) // Search the Queue for the job and remove it if it is there
            {
                response_message  = malloc(strlen(req_argv[0]) + strlen(" removed\n") + 1);
                str_malloc = 1;
                sprintf(response_message, "%s removed\n", req_argv[0]);    
            }
            else if(jobProcessor_delete_job(processor, req_argv[0]) == 0) // If not check the processor aswell
            {
                response_message  = malloc(strlen(req_argv[0]) + strlen(" terminated\n") + 1);
                str_malloc = 1;
                sprintf(response_message, "%s terminated\n", req_argv[0]);
            }
            else // Job not found
            {
                response_message  = malloc(strlen(req_argv[0]) + strlen(" not found\n") + 1);
                sprintf(response_message, "%s not found\n", req_argv[0]);
            }
            str_malloc = 1;
            number_of_char_response = strlen(response_message) + 1;
            break;
        case POLL_RUNNING:;
            // Ask for the table containing all job tupples in processor
            table = jobProcessor_contains(processor, &size);
            if(table == NULL)
            {
                response_message = "Empty\n";
                number_of_char_response = 7;
            }
            else
            {
                // Convert the JobTupple in to a string in order to sent it to the commander 
                int str_size = strlen("<, , >\n")*size + 1;
                for(int i = 0; i < size; i++)
                    str_size += strlen(table[i]->jobID) + strlen(table[i]->job) + 11;
                response_message = malloc(sizeof(char)*str_size);
                str_malloc = 1;
                number_of_char_response = str_size;
                int iter = 0;
                for(int i = 0; i < size; i++)
                {
                    iter += sprintf(response_message + iter, "<%s, %s, %d>\n", table[i]->jobID, table[i]->job, table[i]->queuePosition);
                    destroy_tupple(table[i]);
                }
                free(table);
            }
            break;
        case POLL_QUEUED:;
            // Same logic as before for the queued
            table = jobQueue_contains(queue, &size);
            if(table == NULL)
            {
                response_message = "Empty\n";
                number_of_char_response = 7;
            }
            else
            {
                int str_size = strlen("<, , >\n")*size + 1;
                for(int i = 0; i < size; i++)
                    str_size += strlen(table[i]->jobID) + strlen(table[i]->job) + 11;
                response_message = malloc(sizeof(char)*str_size);
                str_malloc = 1;
                number_of_char_response = str_size + 1;
                int iter = 0;
                for(int i = 0; i < size; i++)
                {
                    iter += sprintf(response_message + iter, "<%s, %s, %d>\n", table[i]->jobID, table[i]->job, table[i]->queuePosition);
                    destroy_tupple(table[i]);
                }
                free(table);
            }
            break;        
        case EXIT:
            // Delete the txt file and set the termination flag 1
            unlink("jobExecutorServer.txt");
            timetodie = 1;
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

int issueJob(char* job_argv[], int job_argc)
{

}

int setConcurrency(int new_conc)
{

}

int stop(int id)
{

}

int exit()
{

}