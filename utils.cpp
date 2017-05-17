#include <time.h>

#define rgbw uint32_t

#ifndef UTILS
#define UTILS 1

struct panel {
	uint16_t x;
	uint16_t y;
	uint16_t n;
	uint8_t  o;
	rgbw led[64];
};

struct pctx {
	panel **m;
	uint16_t x;
	uint16_t y;
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
	std::cout << "[" << difftime(time(0),tstart) << "]: " << c ;
}


#endif