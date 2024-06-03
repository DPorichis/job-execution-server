#include "worker.h"


int worker(Server server)
{

}


// Function for creating a worker thread, 
// the void * will be casted as a jobInstance pointer
// and worker function will be called
void * wrapper_worker(void* args)
{
    worker((Server)args);
}
