#pragma once
#include <pthread.h>
#include <netinet/in.h>

// Basic enumeration used to pass the command type in request
enum command{
    ISSUE_JOB,
    SET_CONCURRENCY,
    STOP,
    POLL,
    EXIT
};

// A minimalistic outward-facing representation of a job
struct job_tupple
{
    char* jobID; // contains the Job_XX string
    char* job; // contains the argv of the job in a single line
};

typedef struct job_tupple* JobTupple;

// A conprehensive inward-facing representation of a job
struct job_instance
{
    char* jobID; // contains the Job_XX string
    char** job_argv; // contains the argv table of the job
    int argc; // contains the number of arguments
    int socket; // contains the socket of the client
};

typedef struct job_instance* JobInstance;


// A total representation of server
struct server
{
    // Buffer Info (LOCK REQUIRED)
    JobInstance* job_queue;
    int concurrency;
    int running_now;
    int front;
    int size;
    int queued;
    int total_jobs;
    int exiting;

    // Sync Control
    pthread_mutex_t mtx;
    pthread_cond_t alert_worker;
    pthread_cond_t alert_controller;

    // Connection Info (LOCK IS NOT REQUIRED)
    pthread_t main_thread;
    int port, sock;
    struct sockaddr_in serv, client;
    socklen_t clientlen;

};

typedef struct server* Server;

Server server_create(int bufsize, int concurrency, int worker_num, pthread_t main);

char* server_issueJob(Server server, char* job_argv[], int job_argc, int job_socket);

char* server_setConcurrency(Server server, int new_conc);

char* server_stop(Server server, char* id);

char* server_poll(Server server);

char* server_exit(Server server);

JobInstance server_getJob(Server server);