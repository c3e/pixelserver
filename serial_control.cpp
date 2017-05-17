#include "serial.cpp"
#include "utils.cpp"

#define rgbw uint32_t

char dbuf[12800*4];

void * serial_background_loop(void *b){

	while (true){
		serial_write(dbuf,8193);
	}
}

void init_serial_w(){
	//Serial Initialization
	while (serial_fd == -1){
		log("");
		serial_init("/dev/ttyACM0");
		sleep(10);
	}

}

void init_serial_background_loop(){
	pthread_t t;
	pthread_create(&t, NULL, serial_background_loop,NULL);
}

inline int serial_writeb (char *b, size_t length){
	memcpy(dbuf,b,length);
	return 0;
}


inline int send_serial_frame(rgbw * buffer, size_t length){
	buffer[0] = 0x000000FF & '#';
	buffer[length-1] = 0xFF000000 & '\n';
	return serial_writeb((char*)buffer, length);
}