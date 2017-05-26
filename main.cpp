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

rgbw merge_pixel( rgbw a, rgbw b){

}

void xresize(uint32_t **f, int x){
// complex, use imagemagick
}

void yresize(uint32_t **f, int y){
//complex, see top
}

void frame(uint32_t **frame, int xf, int yf){
	panel **p = pnl.m;
	int x = pnl.x;
	int y = pnl.y;
	uint8_t im,jm,id,jd;
	if ( xf > x*64 )
		xresize(frame, x*64);
	if ( yf > y*64 )
		xresize(frame, y*64);
	for (int i = 0; i < x*64; i++){
		for ( int j = 0; j < y*64; j++){
			im = i%64; 
			jm = j%64; 
			id = i/64; 
			jd = j/64;
			switch (p[i/64][j/64].o){
				case 0: //F
					p[id][jd].led[(jm)*8+(im)] = frame[i][j];
				case 1: //S
					p[id][jd].led[jm*8 + (jm%2)*(7-im) + ((jm%2)^1)*(im)] = frame[i][j];
			}
		}
	}
}

void setPanel(uint16_t x, uint16_t y, rgbw c){
	panel **p = pnl.m;
	switch (p[x/64][y/64].o){
		case 0: //F
			p[x%64][y%64].led[(y%64)*8+(x%64)] = c;
		case 1: //S
			p[x%64][y%64].led[y%64*8 + ((y%64)%2)*(7-x%64) + (((y%64)%2)^1)*(x%64)] = c;
	}
}

void usage(){
	printf ("Usage:\n\t-p: http port\n\t-P: pixelflut port\n\t-d: Debug without\n");
}


int main(int argc, char *argv[])
{
	uint16_t pixelflut_port = 4444;
	uint16_t httpapi_port = 5555;
	std::string saddr;
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
        	if ( optarg != NULL)
        		saddr = std::string(optarg);
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

   	decke d(2,2);
	init_log();
	init_mosquitto_threaded();
	if (serial_switch)
		init_serial_w();
	init_pixelflut(1000,1000,pixelflut_port, d);
	init_http(saddr,httpapi_port);

	log("Press Enter to quit.\n");
    std::cin.get();
    IsRun = false;
	//sleep(1000000);

	 
  stop_pixelflut();
}