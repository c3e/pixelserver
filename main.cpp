#include <stdexcept>
#include <iostream>
#include <memory>
#include <chrono>
#include <string.h>
#include <thread>
#include <cstdint>
#include <vector>
#include <evhttp.h>

#include "libs/gason.h"
//#include "serial_control.cpp"
#include "pixelflut.c"
#include <mosquitto.h>
#include "utils.cpp"
#include "http_api.cpp"
#include "mqtt.c"


void xresize(uint32_t **f, int x){

}

void yresize(uint32_t **f, int x){
	
}

void frame(uint32_t **frame, int xf, int yf){
	panel **p = pnl.m;
	int x = pnl.x;
	int y = pnl.y;
	if ( xf > x*64 )
		xresize(frame, x*64);
	if ( yf > y*64 )
		xresize(frame, y*64);
	for (int i = 0; i < x*64; i++){
		for ( int j = 0; j < y*64; j++){
			uint8_t im = i%64; uint8_t jm = j%64; uint8_t id = i/64; uint8_t jd = j/64;
			switch (p[i/64][j/64].o){
				case 0: //F
					p[id][jd].led[(jm)*8+(im)] = frame[i][j];
				case 1: //S
					p[id][jd].led[jm*8 + (jm%2)*(7-im) + ((jm%2)^1)*(im)] = frame[i][j];
			}
		}
	}
}



void usage(){
	printf ("Usage:\n\t-p: http port\n\t-P: pixelflut port\n\t-d: Debug without\n");
}


int main(int argc, char *argv[])
{
	uint16_t pixelflut_port = 4444;
	uint16_t httpapi_port = 5555;
	char * addr = NULL;
	int opt;
	bool serial_switch = true;
	while ((opt = getopt(argc, argv, "p:P:adh")) != -1) {
       	switch (opt) {
       	case 'p':
           httpapi_port = (uint16_t)atoi(optarg);
           break;
       	case 'P':
           pixelflut_port = (uint16_t)atoi(optarg);
           break;
        case 'a':
        	addr = optarg;
        	break;
        case 'd':
        	serial_switch = false;
        	break;
        case 'h':
       	default: /* '?' */
           usage();
           return -1;
       	}
   }
	init_log();
	init_mosquitto();
	if (serial_switch)
		init_serial_w();
	init_pixelflut(1000,1000,pixelflut_port);
	http_api(httpapi_port, addr);



	 
  stop_pixelflut();
}