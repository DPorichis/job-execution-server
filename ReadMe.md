# Multi-Threaded Job Execution Server  

This project focuses on implementing a **multi-threaded job execution server for Bash commands** with a corresponding JobCommander client, utilizing sockets, synchronization primitives, and a shared circular buffer to manage job submissions, concurrency control, and client-server communication in parallel.  

Through the design, the code demonstrates thread-based concurrency, safe shared memory handling with mutexes and condition variables, and a communication protocol that supports both control messages and job output streaming.  

*This project was developed as part of the [**K24 System Programming Course**](https://cgi.di.uoa.gr/~mema/courses/k24/k24.html) at the National and Kapodistrian University of Athens.*  

## Quick Start Guide  

This project consists of three main executables:  

- **jobCommander**: Client program that sends commands to the server (issueJob, poll, stop, setConcurrency, exit).  
- **jobExecutorServer**: Server program that manages incoming requests, dispatches jobs to worker threads, and maintains a synchronized job queue.  
- **progDelay**: Simple testing program.  

### Compilation  

To compile the project, use:  

```bash
make all
```

### Execution

You can run the executables from the `bin` folder with their corresponding arguments:

```
./bin/jobExecutorServer <port> <buffersize> <threadpoolsize>
./bin/jobCommander <server_address> <port> <action> [actions arguments]
./bin/progDelay <delay_seconds>
```

#### JobCommander's Possible Actions

- `issueJob [job arguments]`: Issues a job to be executed by the jobExecutorServer, described in the job arguments.
- `setConcurrency <number>`: Requests the change of the number of worker threads used by the JobExecutorServer.
- `stop <job_id>`: Requests the termination of job `job_id`.
- `poll`: Requests a list of all pending jobs of the jobExecutorServer.
- `exit`: Requests the termination of the jobExecutorServer.

## Code Structure

The source code is organized into the following directories:

- `source/`: Contains all source files.
- `headers/`: Header files for the project utilities and structures.
- `bin/`: Contains all compiled executables.
- `build/`: Contains object files.

### Contents of Source Files

#### JobExecutorServer
- `jobExecutorServer.c`: Main thread implementation of the server.
- `controller.c`: Handles per-connection controller threads.
- `worker.c`: Handles worker threads responsible for executing jobs.
- `utils.c`: Shared variables logic, synchronization, and locking.

#### JobCommander
- `jobCommander.c`: Implementation of the client.

#### Other
- `progDelay.c`: Simple delay-based test program.
- `helpfunc.c`: Miscellaneous helper functions.

## Thread Behavior

Below, we describe the behavior of the different types of threads used by the JobExecutorServer.

### Main Thread 

- Initializes the server structure and worker threads.
- Listens on the specified port for incoming connections.
- Creates a controller thread for each new client connection.
- Stops accepting connections when receiving a SIGUSR1 termination signal.
- Exits after shutting down the listening socket (but detached threads remain active).

### Controller Thread 

- Created when a new client connects.
- Reads and processes the client request according to the communication protocol.
- Executes actions on shared variables (issueJob, poll, stop, etc.).
- Sends responses back to the client.
- Terminates after completing the request.

### Worker Thread 
- Created at server startup.
- Continuously fetches jobs from the circular buffer.
- Executes jobs by forking child processes.
- Transmits job output to the client (in 256-byte packets).
- Sleeps while waiting for jobs if none are available.
- Terminates when it receives a NULL job. 

## Communication Protocol

#### Client Request

The **JobCommander** initiates communication by sending a 3-integer tuple that serves as the request header. The tuple specifies:

- **Argument count** – number of arguments provided  
- **Argument length** – total length (in characters) of all arguments  
- **Command ID** – the command identifier (enum)  

If arguments are present, they follow the header as a string.  

#### Controller Handling

The Server’s controller thread processes the request by:  

1. Reading the header and arguments  
2. Executing the specified command  
3. Sending back a response message  

The response message begins with a single integer indicating the length of the response string, followed by the response itself.  

#### Special Case: `issueJob`

For the `issueJob` command:  

- The controller first sends an acknowledgment of submission.  
- It then hands the connection over to a Worker thread.  
- The Worker executes the job and streams output back to the client in 256-byte packets.  
- Transmission ends with a zero-length packet signaling completion.  


## Synchronization
The shared buffer is protected by:
- 1 mutex for mutual exclusion.
- 3 condition variables:
    - `alertControllers`: Notify controllers when space is available in the buffer.
    - `alertWorkers`: Notify workers when jobs are available and concurrency allows execution.
    - `alertExiting`: Notify the exiting controller when all threads are done with shared variables.

Through these mechanisms, **job execution and client communication** run in **parallel** while access to the *shared buffer* operations are *serialized* to ensure thread safety.
