#include <time.h>

#ifndef UTILS
#define UTILS 1

/*
LOGGING
*/

time_t tstart;

void init_log(){
	tstart = time(0);
}

void log(const char * c){
	std::cout << "[" << difftime(time(0),tstart) << "]: " << c ;
}


#endif