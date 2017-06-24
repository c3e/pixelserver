#include <stdexcept>
#include <iostream>
#include <memory>
#include <string.h>
#include <thread>
#include <cstdint>
#include <vector>


//#include "../libs/gason.h"
//#include "serial_control.cpp"
#include <pixelflut/pixelflut.cpp>
//#include <../libs/mosquitto.h>
#include <utils/utils.cpp>
#include <http/http_api.cpp>
#include <mqtt/mqtt.cpp>
//#include "serial.h"

rgbw merge_pixel( rgbw a, rgbw b){

}

void xresize(uint32_t **f, int x){
// complex, use imagemagick
}

void yresize(uint32_t **f, int y){
//complex, see top
}

bool config (std::string path){

	return false; 
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
	printf ("Usage:\n\t-c: layout Config File path\n\t-a: Interface for http api (default 127.0.0.1)\n\t-p: http port\n\t-P: pixelflut port\n\t-x: panel x dimension (default 2)\n\t-y: panel y dimension (default 2)\n\t-d: Debug without Serial\n\t-S: Serial Port (default /dev/ttyACM0)\n\t-h: help message\n");
}


int main(int argc, char *argv[])
{
	uint16_t pixelflut_port = 4444;
	uint16_t httpapi_port = 5555;
	uint16_t ceil_x = 2;
	uint16_t ceil_y = 2;
	std::string path;
	std::string saddr;
	std::string serial_port;
	int opt;
	serial_switch = true;
	while ((opt = getopt(argc, argv, "p:P:a:dhS:x:y:c:")) != -1) {
       	switch (opt) {
       	case 'p':
           httpapi_port = (uint16_t)atoi(optarg);
           break;
       	case 'P':
           pixelflut_port = (uint16_t)atoi(optarg);
           break;
        case 'y':
           ceil_y = (uint16_t)atoi(optarg);
           break;
        case 'x':
           ceil_x = (uint16_t)atoi(optarg);
           break;
        case 'c':
        	if ( optarg != NULL)
        		path = std::string(optarg);
        	break;
       	case 'a':
        	if ( optarg != NULL)
        		saddr = std::string(optarg);
        	break;
        case 'S':
        	if (optarg != NULL)
        		serial_port = std::string(optarg);
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

   	decke d(ceil_x,ceil_y);
   	
   	if ( config (path) ){
   		//
   	}
   	// default values
   	// size of ceiling should be determined by reading a saved config file
   	// or interaactively laying out the ceiling configuration
	init_log();
	init_mosquitto_threaded();
	if (serial_switch)
		init_serial_w(serial_port);
	
	//init_pixelflut(1000,1000,pixelflut_port, d);
	server_start(pixelflut_port,d);
	init_http(saddr,httpapi_port);

	log("Press Enter to quit.\n");
    std::cin.get();
    IsRun = false;
	//sleep(1000000);

	 
  //stop_pixelflut();
  return 0;
}
