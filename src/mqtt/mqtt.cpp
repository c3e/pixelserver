#include "utils/utils.cpp"
#include "../../libs/mosquitto/lib/mosquitto.h"
//#include <mosquitto.h>

bool MQTT_STARTED = false;

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
		log("Connect failed!\n");
	}
}

//
// parse json from mqtt channel
// calls send_panel
// Why do we need this?
//

void json_parse ( char * buffer){

	/*
	
	Missing functionality

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

	*/

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

void * init_mosquitto_t(void *){
	init_mosquitto();		
}


void init_mosquitto_threaded(){
	pthread_t t;
	pthread_create(&t, NULL, init_mosquitto_t,NULL);
}
