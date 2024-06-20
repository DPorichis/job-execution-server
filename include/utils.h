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
    // Buffer Info
    JobInstance* job_queue; // Circular buffer
    int front; // Front of the buffer
    int size; // Size of the buffer
    int queued; // Number of jobs stored in buffer
    
    // Server's State
    int exiting; // Exiting flag
    int alive_threads; // Number of threads that COULD ask to access the shared data
    int accepting_connections; // Flag for whether or not main accepts new connections
    int concurrency; // Concurrency Level
    int running_now; // Workers executing jobs
    int total_jobs; // Job counter (Used for ID creation)
    
    // Sync Control
    pthread_mutex_t mtx;
    pthread_cond_t alert_worker;
    pthread_cond_t alert_controller;
    pthread_cond_t alert_exiting;

    // Connection Info
    pthread_t main_thread;
    int port, sock;
    struct sockaddr_in serv, client;
    socklen_t clientlen;

};

typedef struct server* Server;

// Creates a server representation with the specifications given as arguments
Server server_create(int bufsize, int concurrency, int worker_num, pthread_t main);

// Tries to insert the job described by the arguments to the buffer
// The function will communicate the outcome of the insertion to the client
// NOTE: This function includes locking and waiting logic
int server_issueJob(Server server, char* job_argv[], int job_argc, int job_socket);

// Sets the server's concurrency to new_conc, alerting workers to optain a job if "legal"
// It returns a response message for client (needs deallocation after transmition)
// NOTE: This function includes locking logic
char* server_setConcurrency(Server server, int new_conc);

// Searches the job buffer for a job with matching ID, destroying it if found. 
// It returns a response message for client (needs deallocation after transmition)
// NOTE: This function includes locking logic
char* server_stop(Server server, char* id);

// Constructs a response message for client, listing all the jobs queued in the buffer
// (needs deallocation after transmition)
// NOTE: This function includes locking logic
char* server_poll(Server server);

// Initializes the exiting routine. 
// It signals the server to stop accepting connections, clears the buffer and waits
// for ALL threads to finish their exiting routine.
// It returns a response message for client (needs deallocation after transmition)
// NOTE: This function includes locking logic
char* server_exit(Server server);

// Tries to optain a job from the buffer, and returns it.
// If exiting occurs the function will return NULL
// NOTE: This function includes locking and waiting logic
JobInstance server_getJob(Server server);

// Destroys the server struct, freeing up all the space
// NOTE: Make SURE that the mutex is UNLOCKED and NO THREAD will try to access the server
// BEFORE calling this command
void server_destroy(Server server);

// Frees up all space occupied by job
void destroy_instance(JobInstance job);