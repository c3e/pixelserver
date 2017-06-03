
#ifndef SER
#define SER 1
int serial_fd;
#include "serial.cpp"
#endif

int serial_read(char * , size_t );
int serial_init(std::string );
int serial_write(char * ,size_t );

void set_mincount(int , int );
int set_interface_attribs(int , int );
