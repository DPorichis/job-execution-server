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
