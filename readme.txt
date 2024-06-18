System Programming Project 2 2024
Name: Dimitrios - Stefanos Porichis
AM: 1115202100159

Compilation: To compile this project use `make all` in the main folder, then you can execute right from there with `./bin/jobCommander ...`
    -make also compiles progDelay as ./bin/progDelay

==========Running notes=========== 
    - Issuing a job containing a program in the local directory requires ./ in front of it 
    - To run commands you need to specify the bin directory: 
    ./bin/jobCommander …, ./bin/jobExecutorServer …, ./bin/progDelay … 
    - Everything else is according to the assignment's standards.

=======Code structure notes=======
  The source code of this project is spit to 3 directories:
    -source: Contains all source code
    -headers: Contains all the header files of utils
    -bin: Contains all the executables
    -build: Contains all the object files

=====Contents of source files=====
  JobExecutorServer's:
    - jobExecutorServer.c: Contains the implementation of the server's main thread.
    - controller.c: Contains the implementation of a controller thread.
    - worker.c: Contains the implementation of a worker thread.
    - utils.c: Contains all implementations of functions regarding the use of shared variables. The majority of the locking and synchronization logic is in this file.
  JobCommander's:
    - jobCommander.c: Contains the implementation of the jobCommander.
  Other:
    - progDelay.c: Contains the implementation of a simple testing program (as it is provided in e-class)
    - helpfunc.c: Contains the implementation of helper functions, that are not that useful.

=======Implementation notes=======
  Structures of the implementation:
    - JobInstance: Contains all information needed for a job
    - Server: Represents the central structure of the server. It contains all information for the state of the program as well as the buffer and synchronization structure.
    Note: The functions for controlling these structures won't be analyzed here, refer to the header files for more details
