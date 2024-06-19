System Programming Project 2 2024
Name: Dimitrios - Stefanos Porichis
AM: 1115202100159

Compilation: To compile this project use `make all` in the main folder, then you can execute right from there with `./bin/jobCommander ...`
	-make also compiles progDelay as `./bin/progDelay`

==========Running notes=========== 
	- Issuing a job containing a program in the local directory requires ./ in front of it 
    - To run commands you need to specify the bin directory: 
    	./bin/jobCommander …, ./bin/jobExecutorServer …, ./bin/progDelay … 

=======Code structure notes=======
  The source code of this project is spit to 4 directories:
	-source: Contains all source code
    -headers: Contains all the header files of utils
    -bin: Contains all the executables
    -build: Contains all the object files

=====Contents of source files=====
  JobExecutorServer's:
    - jobExecutorServer.c: Contains the implementation of the server's main thread.
    - controller.c: Contains the implementation of a controller thread.
    - worker.c: Contains the implementation of a worker thread.
    - utils.c: Contains all implementations of functions regarding the use of shared variables. 
      (The majority of the locking and synchronization logic is in this file.)
  JobCommander's:
    - jobCommander.c: Contains the implementation of the jobCommander.
  Other:
    - progDelay.c: Contains the implementation of a simple testing program (as provided in e-class)
    - helpfunc.c: Contains the implementation of helper functions, that are not that useful.

=======Implementation notes=======
Structures of the implementation:
    - JobInstance: Contains all information needed for a job
    - Server: Represents the central structure of the server. It contains all information for the state of the program as well as the buffer and synchronization structure.  

Main thread's behavior:
	By calling jobExecutorServer with the according arguments, the server will initialize its server structure, create its worker threads, and wait for connections in the port specified. Every new connection will be assigned to its controller thread, as described by the assignment.
    The main thread will accept connections in a loop until it receives a termination signal (SIGUSR1). At that point, the termination signal handler will be activated and will proceed to close the listening socket and exit the main thread. 
    Note: The exit of the main thread won't terminate the server process, as other threads will be active. Also, note that all created threads are detached so doesn't need to wait for them.

Controller thread’s behavior:
    Whenever a new connection is established, the main thread locks the main structure and creates a new controller thread, updating the number of threads now alive. Controller threads take the socket and the server structure as arguments and perform the communication protocol listed below. After receiving the request, controllers perform one of the actions described in [Actions on Shared Variables] and continue with the communication protocol for sending the response. When this is done, the controller thread exits.

Worker thread's behavior:
	The worker threads are created at the server's initialization and live in a constant loop of executing jobs or sleeping while waiting for one. When a job is received [Look at Actions on Shared Variables for info on how that is implemented] the worker thread creates a child process that will execute the job, as described by the assignment. After the execution, the worker thread communicates the result to the waiting client, according to the [Communication protocol] below. When that is done the worker goes back to asking for a job.
	A worker thread exits when it receives a job equal to NULL. 

[Communication protocol]
    The client (JobCommander) will send a request tuple, consisting of 3 int values:
        - Number of arguments of command
        - Total character length of the command's arguments
        - Command identification (ENUM value)
    After that, the client will send out the arguments, if there are any, and wait for a response from the server.
	
	The assigned controller thread will read the request tuple and arguments, if they exist, performing the command described.

    For all commands except issueJob, the controller will send the response back to the client, first by sending out the length of the response and then the response itself. After that the communication is officially over, the socket is closed by both parties, jobCommander exits and the controller thread dies.

    If the command is issueJob, the controller thread is only responsible for transmitting the message of submission, leaving the socket connected for the worker thread to transmit the output of the job.

    Transmission of output is sent in packets of 256 bytes, as it is read from a file and its length could be too large to be sent as one. Before each packet is sent an integer containing its length is transmitted. The end of the communication is signaled when this length integer is set to zero.

[Actions on Shared Variables]
Buffer Actions and synchronization
	This implementation achieves a safe usage of the shared buffer using:
        - 1 mutex for accessing the shared variables
        - 3 condition variables:
            + alertControllers: To signal waiting controllers that space is available if they are waiting to store a job
            + alertWorkers: To signal waiting workers that there is a job available for execution, and concurrency allows the execution
            + alertExiting: To signal the controller thread responsible for exiting that ALL threads are done using the shared variables.
        - Multiple counters and variables for storing metadata.

Here is a short sum-up of how each action on the buffer is performed:
    - IssueJob: The function locks the mutex and checks for available storage, waiting under the alertControllers condition variable if no space is available. Afterward, it creates a job instance for the described job and stores it in the circular buffer, updating the according counters. Before leaving, it checks if a worker thread could execute the job issued, signaling one if so. After that, the mutex is unlocked, and the submission message is sent to the client
	
	- SetConcurrency: The function locks the mutex changes the concurrency to the desired value and signals workers to execute a job, if possible with the new concurrency level. After that, the mutex is unlocked and the response message is constructed
	
	- Poll: The function locks the mutex to ensure the state of the buffer stays the same, and goes over all queued jobs, constructing a representation string for each. It then unlocks the mutex, constructs a continuous string containing all the representations stored earlier.
	
	- Stop: The function locks the mutex, and goes over all queued jobs checking if the ID matches. If the job is found, it destroys it and shifts the remaining jobs left to fill the gap. The mutex is unlocked and the relevant message is constructed and returned.
	Note: The client waiting for the job's execution WON'T receive any message.
	
	- GetJob: Get job is used by the worker thread. It locks the mutex and checks if a job is available and concurrency allows the execution of it. If not, the worker sleeps under the alertWorker condition variable. If the worker can get a job, it does so from the front of the circular buffer by taking the pointer of the job instance, removing it from the circular buffer, and updating the variables accordingly. If the buffer was full, the function signals a waiting controller to store its job in the space created. After that the mutex is unlocked and the job is returned. (This return of a job is what was described before as "the worker receives a job")
	
	- Exit: The exiting function locks the mutex and sends a signal to the main thread to stop accepting new connections. Then it cleans up the buffer, destroying every job queued and sending the termination response to all clients waiting. Next, it proceeds to wake up all threads, for them to start their exiting process. To free up the server structure space, the function must wait for ALL threads to guarantee that they won't touch the server again. That is done through the alive_threads variable, which keeps track of all threads created. As we can not tell with certainty that the main thread will stop accepting new connections before other threads exit, we also check that accepting_connections is set to 0 before deallocating the memory. After the deallocation, the response message is constructed.
	Note: The server will eventually exit after the termination of the final thread. That may not be the controller thread that received the exiting command, as alive_threads only indicates the number of threads that COULD access the shared structure, and NOT the actual threads active.

[Final Notes Regarding the parallelism of the implementation]
	- This implementation allows executions of jobs and communications with clients to run in parallel.
	- It does not allow any of the [Actions on Shared Variables] to be performed simultaneously.

That's all for this README
For more information, you can take a look at the comments inside the source files.
