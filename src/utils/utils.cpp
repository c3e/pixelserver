#include <iostream>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
//#include <std>
#include <cstdint>

#define rgbw u_int32_t

#ifndef UTILS
#define UTILS 1

pthread_mutex_t logm;
bool serial_switch;
bool volatile IsRun = true;

struct panel {
	u_int16_t x;
	u_int16_t y;
	u_int16_t n;
	u_int8_t  o;
	rgbw led[64];
};

struct pctx {
	panel **m;
	u_int16_t x;
	u_int16_t y;
};

//global panel configuration

pctx pnl;

/*
LOGGING
*/

time_t tstart;

void init_log(){
	tstart = time(0);
}

void log(const char * c){
	pthread_mutex_lock(&logm);
	std::cout << "[" << difftime(time(0),tstart) << "]: " << c ;
	pthread_mutex_unlock(&logm);
}
void logn(const char * c, const char * d){
	pthread_mutex_lock(&logm);
	std::cout << "[" << difftime(time(0),tstart) << "]: " << c << d << std::endl;
	pthread_mutex_unlock(&logm);
}
void logn(const char * c, const char * d, const char * e){
	pthread_mutex_lock(&logm);
	std::cout << "[" << difftime(time(0),tstart) << "]: " << c << d << e << std::endl;
	pthread_mutex_unlock(&logm);
}


#endif