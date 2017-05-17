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
#define rgbw uint32_t

rgbw drawBuffer[256*8+2];
bool MQTT_STARTED = false;



//
// called by api 
// issued from a web request
// update 1 pixel with panel id and x,y coord on panel
//  


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

// 
// http api
// 


//
// handler function for mqtt
// called on MQTT connect event
// subscribes to channel
//

void on_connect(struct mosquitto *mosq, void *userdata, int result){
	if(!result){
		mosquitto_subscribe(mosq, NULL, "deckenkontrolle", 2);
		log("Connected to channel deckenkontrolle!\n");
	}else{
		fprintf(stderr, "Connect failed!\n");
	}
}

//
// parse json from mqtt channel
// calls send_panel
//

void json_parse ( char * buffer){
	char * end = buffer + strlen(buffer);
	JsonValue value;
	//JsonValue *n;
	JsonAllocator allocator;

	uint8_t status = jsonParse(buffer, &end, &value, allocator);	
	if (status != JSON_OK && value.getTag() != JSON_OBJECT) {
    	fprintf(stderr, "%s at %zd\n", jsonStrError(status), end - buffer);
	}

	uint32_t store[16];
	uint8_t index[16];
	uint8_t x,y;
	uint8_t count = 0;
	for (auto i : value) {
		if ( *(i -> key) == 'x' )
			x = i -> value.toNumber();
    	if ( *(i -> key) == 'y' )
			y = i -> value.toNumber();
		if ( *(i -> key) == 'p' ){
			//n = i -> value.toNode();
			// only works with modified gason library
			for ( auto j : i -> value ){
				try {

					store[atoi(j->key)] = (uint32_t) atoi((char *)&j->value.fval);
				}
				catch ( ... ) {}
				index[count++] = atoi(j->key); 
			}
    	}
	}

	send_panel( y*8+x, store );
}

// 
// handler function for mqtt
//

void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message){
	
	if ( message->payloadlen)
		json_parse((char *)userdata);
}

void init_mosquitto() {
		// MQTT initialization
	struct mosquitto * mosq ;
	mosquitto_lib_init();
	mosq = mosquitto_new(NULL, true, NULL);
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);
	
	if( mosquitto_connect(mosq, "mqtt.chaospott.de", 1883, 60) ){
		log("Unable to connect to MQTT f00\n");
	} else {
		MQTT_STARTED = true;
		mosquitto_loop_start(mosq);
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