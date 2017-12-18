//#include "utils/utils.cpp"
//#include "../../libs/mosquitto/lib/mosquitto.h"
#include <mosquitto.h>
#include <string.h>
#include <unistd.h>
#include "utils/utils.cpp"

void message_parse ( char * buffer){
	//parse length
	char * c  = buffer;
	while (*c != '\0')
		c++;
	long length = (int)(c-buffer);
	if (length > 1){
		if (strncmp(buffer, "ON", 2) == 0)
			//on signal
			{}
			
	}
	if (length > 2){
		if (strncmp(buffer, "OFF", 3) == 0)
			//off signal
			{}
	}

}

void on_connect(struct mosquitto *mosq, void *userdata, int result){
	if(!result){
		mosquitto_subscribe(mosq, NULL, "/foobar/elab/deckenkontrolle", 2);
		log_("MQ: Connected to channel deckenkontrolle!\n");
	}else{
		log_("MQ: Connect failed!\n");
	}
}

void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message){
	if ( message->payloadlen)
		message_parse((char *)message->payload);
	else
		message_parse((char *)message->topic);
}

void init_mosquitto() {
	// MQTT initialization
	struct mosquitto * mosq ;
	mosquitto_lib_init();
	mosq = mosquitto_new(NULL, true, NULL);
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);
	
	if( mosquitto_connect(mosq, "mqtt.foobar.local", 1883, 60) ){
		log_("Unable to connect to Server\n");
	} else {
		mosquitto_loop_start(mosq);
	}
}
