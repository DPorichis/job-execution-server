#include "utils.h"

// The routine of a controller thread
int worker(Server server);

// Wrapper function for dereferencing the argument
void * wrapper_worker(void* args);
