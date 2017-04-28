

#include <stdexcept>
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <cstdint>
#include <vector>
#include <evhttp.h>


char * api( char * p, evhttp_request * req){
  printf("ohai!\n");
  return NULL;
}

int main()
{
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
  
  if( mosquitto_connect(mosq, "mqtt.chaospotts.de", 1883, 60) ){
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
    printf("nya\n");
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
