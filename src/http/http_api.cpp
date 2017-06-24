#include <evhttp.h>
#include <chrono>
#include <thread>
#include <utils/utils.cpp>
#include <serial/serial_control.cpp>
#include <list>
#include "../../libs/json/gason.hpp"

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
  serial_writeb(b, 4);

  return NULL;
}

// show markings for configuration 
char * show_markings ( uint16_t n){
  size_t bsize = 8182;
  char buffer[bsize+1];
  buffer[0] = '#';
  setPixelB(n*64,0xFFFFFFFF,buffer);
  setPixelB((n+1)*64-1, 0xFFFFFFFF, buffer);
  serial_writeb(buffer, bsize+1);
  return NULL;
}

char * fill(uint16_t n ){
  char b[8193];
  memset(b,0,8193);
  b[0] = '#';
  for ( int i = 0; i<n; i++){
    setPixelB(i, 0xFF000000, b+1);
  }
  serial_writeb(b,8193);
  return NULL;
}

char * nope(){
  char a;
  a = '+';
  serial_writeb(&a,1);
  return NULL;
}

char * update_pixel(uint8_t n){
  
  if ( setPixel(n, 0xFF000000) != 0){
    log ("Serial: Error\n" );
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
  serial_writeb(p, 3);
  return NULL;
}

//
// update a whole panel 
// calls send_panel 
//
char * update_panel(uint8_t id, char *buffer ){

    return NULL;
}


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
      if ( *(i->key) == 'x' ) {
        int t = i->value.toNumber();
        if ( t > mx )
          mx = t;
        a = t;
      }
      if ( *(i->key) == 'y' ) {
        int t = i -> value.toNumber();
        if ( t > my )
          my = t;
        b = t;
      }
      if ( *(i->key) == 'n' ) {
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

char * start_buffered(){
  init_serial_background_loop();
  CONSTANT_WRITE_LOOP = true;
  return NULL;
}


char * api( char * p, evhttp_request * req){
  //printf("Yo!");
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

  log("");
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
    //log("Set Reached!\n");
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
      ret = fill(atoi( args[4].c_str() ) );
    } else if ( args[3].compare("nope") == 0){
      //demo application
      ret = nope( );
    } else if ( args[3].compare("number") == 0){
      //write a number to screen
      ret = number(atoi( args[4].c_str()),atoi( args[5].c_str()),atoi( args[6].c_str()));
    } else if ( args[3].compare("buffer") == 0 ){
      ret = set_buffer(atoi( args[4].c_str()));
    } else if ( args[3].compare("threaded") == 0 ){
      ret = start_buffered();
    }



  } else if ( strncmp(args[2].c_str(), "get", 3 ) == 0){
    //fun query 
  } 
  return ret;
}

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


int http_api(uint16_t port, const char * addr){

	const char  * SrvAddress;
	char def[] = "127.0.0.1";
	SrvAddress = def;
	if (addr != NULL)
		SrvAddress = addr;
	std::uint16_t const SrvPort = port;
	int const SrvThreadCount = 4;
	try
	{

    std::exception_ptr InitExcept;
    evutil_socket_t Socket = -1;
    
    auto ThreadFunc = [&] (){
      try
      {
        std::unique_ptr<event_base, decltype(&event_base_free)> EventBase(event_base_new(), &event_base_free);
        if (!EventBase)
          log("HA: Failed to create new base_event.\n");
        
        std::unique_ptr<evhttp, decltype(&evhttp_free)> EvHttp(evhttp_new(EventBase.get()), &evhttp_free);
        if (!EvHttp)
          throw std::runtime_error("Failed to create new evhttp.");
        
        evhttp_set_gencb(EvHttp.get(), OnRequest, nullptr);
        
        if (Socket == -1)
        {
          auto *BoundSock = evhttp_bind_socket_with_handle(EvHttp.get(), SrvAddress, SrvPort);
          if (!BoundSock)
            log("HA: Failed to bind server socket.\n");
          if ((Socket = evhttp_bound_socket_get_fd(BoundSock)) == -1)
            log("HA: Failed to get server socket for next instance.\n");
        }
        else
        {
          if (evhttp_accept_socket(EvHttp.get(), Socket) == -1)
            log("HA:Failed to bind server socket for new instance.\n");
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
    //std::cout << "Press Enter to quit." << std::endl;
    //std::cin.get();
    //IsRun = false;
    while (IsRun)
      sleep (5);
    return 0;
  }
  catch (std::exception const &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  return 0;
}

struct tcpy{
  std::string host;
  uint16_t port;
};

void * http_control_thread(void *a){
  tcpy * n = (tcpy *)a; 
  http_api(n->port,n->host.c_str());
}


int init_http(std::string host, int port){
 tcpy * tmp = (tcpy *)malloc(sizeof(tcpy));
 if ( !host.empty())
  tmp->host = host;
 tmp->port = port;
 //logn("Host: ",tmp->host);
 pthread_t t;
 pthread_create(&t, NULL, http_control_thread, tmp);
}