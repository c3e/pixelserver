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
#include "serial.cpp"
#include <mosquitto.h>

#define rgbw uint32_t


struct tile {
	uint8_t layout;
	uint8_t spin;
	uint32_t mask;
};


class screen {
	public:
		screen(uint16_t, uint16_t, rgbw);
		screen(uint16_t, uint16_t);
		void set_pixel(uint16_t, uint16_t, rgbw);
		void configure_tiles(uint16_t, uint16_t);
		void set_tile_layout(uint8_t);
	private:
		rgbw * panel;
		tile * tiles;
		uint16_t width;
		uint16_t height;
		uint8_t tile_layout;

};
void screen::set_pixel(uint16_t x, uint16_t y, rgbw color ) {
	panel[y*width+x] = color;
}

screen::screen ( uint16_t x, uint16_t y){
	width = x;
	height = y;
	panel = (rgbw *)malloc(sizeof(rgbw)*x*y);
	configure_tiles(x,y);
}

screen::screen ( uint16_t x, uint16_t y, rgbw color){
	width = x;
	height = y;
	panel = (rgbw *)malloc(sizeof(rgbw)*x*y);
	memset(panel,color,sizeof(panel));
	configure_tiles(x,y);
}

void screen::configure_tiles(uint16_t x, uint16_t y){
	tiles = (tile *)malloc(sizeof(tile)*(x%64)*(y%64));

}

void screen::set_tile_layout(uint8_t layout){
	tile_layout = layout;
}
bool MQTT_STARTED = false;
screen * s;

inline int send_serial_frame(rgbw * buffer, size_t length){
	buffer[0] = '#';
	buffer[length-1] = '\n';
	return serial_write(buffer, length);
}

// Background loop
// runs after configuration
void send_serial_background_loop(){
	//fun stuff
}
void init_serial_background_loop(){
	//start fun stuffv
}

// show markings for configuration 
char * show_markings(uint16_t panel){
	rgbw buffer[panel*64+2];
	memset(buffer, 0, panel*64);
	buffer[panel*64-1+2] = 0x00FF0000;
	buffer[panel*64-65+2] = 0xFF000000;
	
	send_serial_frame(buffer, panel*64+2);
	return NULL;
	//show red marker add beginning and green marker at the end
}

//
// called by api 
// issued from a web request
// update 1 pixel with panel id and x,y coord on panel
//  


char * update_pixel(uint8_t id, uint8_t x, uint8_t y, char *buffer ){
	
	// values for gason library
	// Dummy
	return  NULL;
}


//
// update a whole panel 
// calls send_panel 
//
char * update_panel(uint8_t id, char *buffer ){

    return NULL;
}

//
// sends panel updates to mqtt channel
// and CAN Bus
//
inline void send_panel ( uint8_t id, uint32_t * store){

	//Dummy
}


// 
// http api
// 

char * api( char * p, evhttp_request * req){
	char * ret = NULL;

	std::string s(p);
	std::string delimiter = "/";


	//Need to find a better Solution than this!
	std::string args[10];
	uint8_t i=0;
	uint8_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos && i < 10) {
    	token = s.substr(0, pos);
    	args[i] = token;
   	 	s.erase(0, pos + delimiter.length());
   	 	i++;
	}
	if ( !s.empty() ) 
		args[i] = s;
	//Seriously! ie. Something that is actually safe!

	for ( int k = 0; k < 10 ; k++){
		printf("%i: %s\n",k,args[k].c_str() );
	}

	if ( strcmp( args[1].c_str(), "api") != 0)
		return NULL;

	uint8_t len = evbuffer_get_length(evhttp_request_get_input_buffer(req));
	struct evbuffer *in_evb = evhttp_request_get_input_buffer(req);
    char data[len+1];
	evbuffer_copyout(in_evb, data, len);

	//This is also bad!
	if ( strncmp( args[2].c_str(),"set", 3) == 0){
		if        ( args[3].compare("panel") == 0 ){
			//setting a whole panel
			ret = update_panel(atoi(args[4].c_str()), data);
		} else if ( args[3].compare("pixel") == 0){
			//setting singel pixels
			ret = update_pixel(atoi( args[4].c_str()),atoi( args[5].c_str()),atoi( args[6].c_str()), data);
		} else if ( args[3].compare("config") == 0){
			//configuration process
			ret = show_markings( atoi (args[4].c_str() ) );
		}

	} else if ( strncmp(args[2].c_str(), "get", 3 ) == 0){
		//fun query 
	} 
	return ret;
}

//
// handler function for mqtt
// called on MQTT connect event
// subscribes to channel
//

void on_connect(struct mosquitto *mosq, void *userdata, int result){
	if(!result){
		mosquitto_subscribe(mosq, NULL, "#", 2);
		printf("connected to channel #");
	}else{
		fprintf(stderr, "Connect failed\n");
	}
}

//
// parse json from mqtt channel
// calls send_panel
//

void json_parse ( char * buffer){
	char * end = buffer + strlen(buffer);
	JsonValue value;
	JsonValue *n;
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

int main(){
	
	// HTTP api
	char const SrvAddress[] = "0.0.0.0";
	std::uint16_t const SrvPort = 5555;
	int const SrvThreadCount = 4;

	// MQTT initialization
	struct mosquitto * mosq ;
	mosquitto_lib_init();
	mosq = mosquitto_new(NULL, true, NULL);
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);
	
	if( mosquitto_connect(mosq, "mqtt.chaospott.de", 1883, 60) ){
		printf("Unable to connect to MQTT f00\n");
	} else {
		MQTT_STARTED = true;
		mosquitto_loop_start(mosq);
	}

	//
	// handler function for evhttp server
	// calls api()
	//
	void (*OnRequest)(evhttp_request *, void *) = [] (evhttp_request *req, void *)
	{
		char * ret;
		auto *OutBuf = evhttp_request_get_output_buffer(req);
		if (!OutBuf)
			return;
		char *path = req->uri;
		if ( ( ret = api(path, req) ) != NULL ){
			evbuffer_add(OutBuf, ret, 150);
		} else {
			evbuffer_add_printf(OutBuf, "Ok");
		}
		evhttp_send_reply(req, HTTP_OK, "", OutBuf);
		if ( ret != NULL)
			free(ret);
	};

	std::exception_ptr InitExcept;
	bool volatile IsRun = true;
	evutil_socket_t Socket = -1;
	auto ThreadFunc = [&] ()
	{
		try
		{
			std::unique_ptr<event_base, decltype(&event_base_free)> EventBase(event_base_new(), &event_base_free);
			if (!EventBase)
				throw std::runtime_error("Failed to create new base_event.");
			std::unique_ptr<evhttp, decltype(&evhttp_free)> EvHttp(evhttp_new(EventBase.get()), &evhttp_free);
			if (!EvHttp)
				throw std::runtime_error("Failed to create new evhttp.");
				evhttp_set_gencb(EvHttp.get(), OnRequest, nullptr);
			if (Socket == -1)
			{
				auto *BoundSock = evhttp_bind_socket_with_handle(EvHttp.get(), SrvAddress, SrvPort);
				if (!BoundSock)
					throw std::runtime_error("Failed to bind server socket.");
				if ((Socket = evhttp_bound_socket_get_fd(BoundSock)) == -1)
					throw std::runtime_error("Failed to get server socket for next instance.");
			}
			else
			{
				if (evhttp_accept_socket(EvHttp.get(), Socket) == -1)
					throw std::runtime_error("Failed to bind server socket for new instance.");
			}
			for ( ; IsRun ; )
			{
				event_base_loop(EventBase.get(), EVLOOP_NONBLOCK);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
		catch (...)
		{
			InitExcept = std::current_exception();
		}
	};
	auto ThreadDeleter = [&] (std::thread *t) { IsRun = false; t->join(); delete t; };
	typedef std::unique_ptr<std::thread, decltype(ThreadDeleter)> ThreadPtr;
	typedef std::vector<ThreadPtr> ThreadPool;
	ThreadPool Threads;
	for (int i = 0 ; i < SrvThreadCount ; ++i)
	{
		ThreadPtr Thread(new std::thread(ThreadFunc), ThreadDeleter);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		if (InitExcept != std::exception_ptr())
		{
			IsRun = false;
			std::rethrow_exception(InitExcept);
		}
		Threads.push_back(std::move(Thread));
	}
	std::cout << "Press Enter to quit." << std::endl;
	std::cin.get();
	IsRun = false;
	
	return 0;
}
