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
#include <list>


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

rgbw drawBuffer[256*8+2];

bool MQTT_STARTED = false;
screen * s;

inline int send_serial_frame(rgbw * buffer, size_t length){
	buffer[0] = 0x000000FF & '#';
	buffer[length-1] = 0xFF000000 & '\n';
	return serial_write((char*)buffer, length);
}

// Background loop
// runs after configuration
void send_serial_background_loop(){
	//fun stuff
}
void init_serial_background_loop(){
	//start fun stuffv
}

int setPixel(uint32_t num, rgbw color){
	size_t len = ((num+1)*32)+2;
	size_t size = 8193;
	char buffer[size];

	memset(buffer, 0, size);
	buffer[0] = '#';
	
	int stripLen = 256;
 
	uint8_t row = num / stripLen;
	uint32_t offset = num % stripLen;
	//printf ("Row: %i, Offset: %i\n",row,offset);

	for ( int i = 0; i < 32; i++ ){
		buffer[32*offset+1+i] = ( ( ( color >> (31-i)) & 1 ) << row );
	}
	//memset ( buffer+(row*stripLen+offset+1), )

	int err = serial_write(buffer, size);
	printf("Written %i Bytes, with %i used! Highest Byte Access: %i\n", size, len, 32*offset+31+1);
	return err;
}

void setPixelB(uint32_t num, rgbw color, char * buffer){
	size_t len = ((num+1)*32)+2;
	
	buffer[0] = '#';
	
	int stripLen = 256;
 
	uint8_t row = num / stripLen;
	uint32_t offset = num % stripLen;

	for ( int i = 0; i < 32; i++ ){
		buffer[32*offset+i] |= ( ( ( color >> (31-i)) & 1 ) << row );
	}
}

char * number(int x, int y, int c){

	char b[4];
	b[0] = '*';
	b[1] = c & 0xFF;
	b[2] = x & 0xFF;
	b[3] = y & 0xFF;
	serial_write(b, 4);

	return NULL;
}

// show markings for configuration 
char * show_markings ( uint16_t n){
	size_t bsize = 8182;
	char buffer[bsize+1];
	buffer[0] = '#';
	setPixelB(n*64,0xFFFFFFFF,buffer);
	setPixelB((n+1)*64-1, 0xFFFFFFFF, buffer);
	serial_write(buffer, bsize+1);
	return NULL;
}

char * bla(uint16_t n ){
	char b[8193];
	memset(b,0,8193);
	b[0] = '#';
	for ( int i = 0; i<n; i++){
		setPixelB(i, 0xFF000000, b+1);
	}
	serial_write(b,8193);
	return NULL;
}

char * nope(){
	char a;
	a = '+';
	serial_write(&a,1);
	return NULL;
}

//
// called by api 
// issued from a web request
// update 1 pixel with panel id and x,y coord on panel
//  


char * update_pixel(uint8_t n){
	

	if ( setPixel(n, 0xFF000000) != 0){
		printf ("Serial: Error\n" );
	} 
	
	return NULL;
}


char * set_buffer(int size){
	if ( size < 4 ){
		return NULL;
	}

	char b[3];
	char * p = b;
	b[0] = 'v';
	b[1] = size >> 8;
	b[2] = size && 0xFF;
	serial_write(p, 3);
	return NULL;
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

/*
{
	[
		{
			x: 0,
			y: 0,
			n: 0
		},

		{
			...
		},
	]
}

*/

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

pctx pnl;

char * readconfig( std::string input){

	// This happens when you take efficient C-code and plug it into crappy c++ syntax constraints
	// It disgusts me
	const char * end = input.c_str() + input.length();
	const char * buffer = input.c_str();
	JsonValue c;
	JsonValue *n;
	JsonAllocator allocator;

	uint8_t status = jsonParse((char*)buffer, (char**)&end, &c, allocator);	
	if (status != JSON_OK && c.getTag() != JSON_OBJECT) {
    	fprintf(stderr, "%s at %zd\n", jsonStrError(status), end - buffer);
	}
	uint16_t mx = 0;
	uint16_t my = 0;
	std::list<std::string> strs = {};
	std::list<panel> panels = {};

	uint16_t a,b;

	for (auto j : c){
		for ( auto i : j->value){
			if ( *(i->key) == 'x' )	{
				int t = i->value.toNumber();
				if ( t > mx )
					mx = t;
				a = t;
			}
			if ( *(i->key) == 'y' )	{
				int t = i -> value.toNumber();
				if ( t > my )
					my = t;
				b = t;
			}
			if ( *(i->key) == 'n' )	{
				panel p;
				p.x = a;
				p.y = b;
				p.n = i -> value.toNumber();
				panels.push_back(p);
			}
		}
	}

	panel **m = (panel **)malloc ( my * sizeof(panel *));
	for ( int i=0; i<my; i++ ){
		m[i] = (panel *)malloc ( sizeof(panel) * mx);
	}
	for ( panel p : panels){
		m[p.x][p.y].x = p.x;
		m[p.x][p.y].y = p.y;
		m[p.x][p.y].n = p.n;
	}

	pnl.m = m;
	pnl.x = mx;
	pnl.y = my;
	//write stuff to panel ctx

	return NULL;
}

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

char * api( char * p, evhttp_request * req){
	printf("Yo!");
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
		if( !args[k].empty() ){
			printf("/%s",args[k].c_str() );
		}
	}
	printf("\n");

	if ( args[1].compare("api") != 0)
		return NULL;

	uint8_t len = evbuffer_get_length(evhttp_request_get_input_buffer(req));
	struct evbuffer *in_evb = evhttp_request_get_input_buffer(req);
    char data[len+1];
	evbuffer_copyout(in_evb, data, len);

	//This is also bad!
	if ( args[2].compare("set") == 0){
		printf("Set Reached!\n");
		if        ( args[3].compare("panel") == 0 ){
			//setting a whole panel
			ret = update_panel(atoi(args[4].c_str()), data);
		} else if ( args[3].compare("pixel") == 0){
			//setting singel pixels
			ret = update_pixel(atoi( args[4].c_str()));
		} else if ( args[3].compare("config") == 0){
			//configuration process
			//light one panel up
			ret = show_markings( atoi (args[4].c_str() ) );
		} else if ( args[3].compare("setconfig") == 0){
			ret = readconfig(args[4]);
		} else if ( args[3].compare("fill") == 0){
			//write up to n leds green
			ret = bla(atoi( args[4].c_str() ) );
		} else if ( args[3].compare("nope") == 0){
			//demo application
			ret = nope( );
		} else if ( args[3].compare("number") == 0){
			//write a number to screen
			ret = number(atoi( args[4].c_str()),atoi( args[5].c_str()),atoi( args[6].c_str()));
		} else if ( args[3].compare("buffer") == 0 ){
			ret = set_buffer(atoi( args[4].c_str()));
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
		mosquitto_subscribe(mosq, NULL, "deckenkontrolle", 2);
		printf("connected to channel deckenkontrolle\n");
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

int main()
{

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

	//Serial Initialization
	while (serial_fd == -1){
		serial_init("/dev/ttyACM0");
		sleep(10);
	}

  char const SrvAddress[] = "127.0.0.1";
  std::uint16_t const SrvPort = 5555;
  int const SrvThreadCount = 4;
  try
  {
   void (*OnRequest)(evhttp_request *, void *) = [] (evhttp_request *req, void *)
  {
    char * ret;
    auto *OutBuf = evhttp_request_get_output_buffer(req);
    if (!OutBuf)
      return;
    char *path = req->uri;
    if ( ( ret = api(path, req) ) != NULL ){
      evbuffer_add(OutBuf, ret, 150);
      free(ret);
    } else {
      evbuffer_add_printf(OutBuf, "OK");
    }
    evhttp_send_reply(req, HTTP_OK, "", OutBuf);
  
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
    std::cout << "Press Enter fot quit." << std::endl;
    std::cin.get();
    IsRun = false;
  }
  catch (std::exception const &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  return 0;
}