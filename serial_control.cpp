#include "serial.cpp"
#include "utils.cpp"

//#define rgbw uint32_t

char dbuf[12800*4];

inline int serial_writeb (char *b, size_t length){
	memcpy(dbuf,b,length);
	return 0;
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

  int err = serial_writeb(buffer, size);
  printf("Written %i Bytes, with %i used! Highest Byte Access: %i\n", size, len, 32*offset+31+1);
  return err;
}

int setPixel(uint16_t x, uint16_t y, rgbw color){
	// based on Panel
}

void * serial_background_loop(void *b){

	while (true){
		serial_write(dbuf,8193);
		//add mechanism to compensate timing 
		//optimize for 60fps here
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


inline int send_serial_frame(rgbw * buffer, size_t length){
	buffer[0] = 0x000000FF & '#';
	buffer[length-1] = 0xFF000000 & '\n';
	return serial_writeb((char*)buffer, length);
}