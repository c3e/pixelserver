#include "serial.cpp"
#include "utils.cpp"

#ifndef SC
#define SC 1
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

int init_serial_w(){
	//Serial Initialization
	while (serial_fd == -1){
		//log("");
		serial_init("/dev/ttyACM0");
		sleep(1);
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

class pan {
	public:
		pan ();
		rgbw led[64];	
		pan ( uint8_t , rgbw *);
		pan (uint8_t, uint8_t);
		uint8_t o;
		uint16_t n;
		void setPixel(int,int,rgbw);
		void setPixel(int,rgbw);
		void fill (size_t, rgbw);
};

void pan::setPixel (int n, rgbw r ){
	led[n] = r;
}

void pan::fill(size_t n, rgbw r){
	if (n<65)
		memcpy(led, &r, n);
}

void pan::setPixel(int x,int y, rgbw r){
	switch (o) {
		case '0':
			led[y*8+x] = r;
		case '1':
			led[y*8+(y%2)*(8-x)+((y%2)^1)*(x)] = r;
	}
}

pan::pan () {
	o = 0;
	n = 0;
	fill(64,0);
}

pan::pan (uint8_t orientation, uint8_t position ){
	memcpy(led, 0, 64 *sizeof(rgbw));
	o = orientation;
	n = position;
}

pan::pan ( uint8_t orientation, rgbw * input){
	switch (orientation) {
		case '0':
			memcpy(led, input, sizeof (rgbw)*64 );
			break;	
		case '1':
			for (uint8_t i = 0; i< 8 ; i++){
				if ( i%2 )
					for ( uint8_t j = 7; j>=8; j--)
						 led[i*8+j] = input[i*8];
				else 
					memcpy( &led[i*8], &input[i*8], sizeof(rgbw)*8 );
			}
			break;
	}
	o = orientation;
}

class decke {
	public:
		decke (char *);
		decke (int, int);
		void setPixel(uint16_t,uint16_t,rgbw);
		int draw();
	private:
		int x;
		int y;
		std::vector<pan> panels;
};

void decke::setPixel(uint16_t cx, uint16_t cy, rgbw r){
	panels[cy*x+cx].setPixel(cx%64,cy%64,r);
	draw();
}

decke::decke ( int cx , int cy ){
	//from parameters
	x = cx;
	y = cy;
	panels = std::vector<pan>(x*y);
}

decke::decke ( char * ){
	//from json
}

int decke::draw(){
	char * buf = dbuf; 
	for (int b = 0; b<8; b++ ){
		for ( int i = 0; i < x; i++){
			for (int k = 0; k<64; k++){
				for (int j = 0; j < y;j++){
					buf[(i+y*j+k)*4] |= panels[j*y+i].led[k]   & (0x1 >> b);
					buf[(i+y*j+k)*4+1] |= panels[j*y+i].led[k] & (0x100 >> b);
					buf[(i+y*j+k)*4+2] |= panels[j*y+i].led[k] & (0x10000 >> b);
					buf[(i+y*j+k)*4+3] |= panels[j*y+i].led[k] & (0x1000000 >> b);
				}
			}
		}
	}
	
}
#endif