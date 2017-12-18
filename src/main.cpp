#include <stdexcept>
#include <iostream>
#include <memory>
#include <thread>
#include <cstdint>
#include <vector>
#include <cstring>
#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <mutex>

#include "mqtt/mqtt.cpp"
//#include "utils/utils.cpp"

#define SBUFSIZE 1728
#define rgbw u_int32_t
#define BUFSIZE 2048

#define XSTR(a) #a
#define STR(a) XSTR(a)
#define UDP_PROTOCOL 1

static volatile int running = 1;
static volatile int client_thread_count;
static volatile int server_sock;
static volatile unsigned int total;

#if SERVE_HISTOGRAM
static volatile unsigned int histogram[8][8][8];
static uint8_t* index_html = 0;
static int index_html_len = 0;
#endif

#define PIXEL_HEIGHT 100
#define PIXEL_WIDTH 100
#define CONNECTION_TIMEOUT 1;

void * handle_client(int);
void * handle_clients(unsigned long);
void * serial_background_loop();
uint8_t parse(uint8_t *, uint32_t );


int serial_fd = -1;
char dbuf[SBUFSIZE];
int SERIAL_LOOP=0;
bool volatile IsRun = true;

/*
LOGGING
*/



int set_interface_attribs(int fd, int speed)
{
	struct termios tty;

	if (tcgetattr(fd, &tty) < 0) {
			printf("Error from tcgetattr: %s\n", strerror(errno));
			return -1;
	}

	cfsetospeed(&tty, (speed_t)speed);
	cfsetispeed(&tty, (speed_t)speed);

	tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;         /* 8-bit characters */
	tty.c_cflag &= ~PARENB;     /* no parity bit */
	tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
	tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

	/* setup for non-canonical mode */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;

	/* fetch bytes as they become available */
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 1;

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
			printf("Error from tcsetattr: %s\n", strerror(errno));
			return -1;
	}
	return 0;
}

void set_mincount(int fd, int mcount)
{
	struct termios tty;

	if (tcgetattr(fd, &tty) < 0) {
			printf("Error tcgetattr: %s\n", strerror(errno));
			return;
	}

	tty.c_cc[VMIN] = mcount ? 1 : 0;
	tty.c_cc[VTIME] = 5;        /* half second timer */

	if (tcsetattr(fd, TCSANOW, &tty) < 0)
			printf("Error tcsetattr: %s\n", strerror(errno));
}

//int serial_fd = -1;

int serial_write(char * buffer,int length){
	int err;
	err = write ( serial_fd, buffer, length);
	tcdrain(serial_fd);
	//log_("Written ", std::to_string(err).c_str(), " Bytes!" );
	return err == length ? 0 : -1;
}


int serial_init(std::string portname)
{
	//char * portname = "/dev/ttyUSB0";
	int fd = -1;

	fd = open(portname.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
			log_("Error opening ", portname, strerror(errno));
			return -1;
	} else {
			log_("Acquired Serial (Socket: ",std::to_string(fd).c_str(),") on", portname.c_str());
	}
	set_interface_attribs(fd, B1000000);
	return fd;
}


int serial_read(char * buffer, int length){
	int err = 0;
	while ( err < length ){
			err = read (serial_fd, buffer, length);
	}
	return err;
}


int init_serial(std::string port){
	
	//the basic "rewrite the whole panel"-command begins with a '#', so 
	//we'll write it at the beginning of the buffer and never touch it again
	
	//Serial Initialization 

	int fd = -1;

	while (fd == -1){
		if (port.empty())
			fd = serial_init("/dev/ttyACM0");
		else
			fd = serial_init(port.c_str());
		sleep(1);
	}

	SERIAL_LOOP=1;
	
	serial_fd = fd;

	std::thread t(&serial_background_loop);

	return 0;
}

class panel {
	public:
		panel ();
		rgbw led[64] = {200};	
		panel ( uint8_t , rgbw *);
		panel (uint8_t, uint8_t, uint8_t, rgbw);
		uint8_t orientation;
		uint16_t position;
		uint8_t strip;
		void setPixel(uint32_t,uint32_t,rgbw);
		void setPixel(int,rgbw);
		rgbw getPixel(int,int);
		void fill (size_t, rgbw);
		void toString();
};

void panel::setPixel (int n, rgbw r ){
	led[n] = r;
}

void panel::fill(size_t n, rgbw r){
	if (n<65)
		memset(led, r, n*sizeof(rgbw));
}

void panel::setPixel(uint32_t x,uint32_t y, rgbw r){
	switch (this->orientation) {
		case 0:
			led[y*8+x] = r;
			break;
		case 1:
			led[y*8+(y%2)*(7-x)+((y%2)^1)*(x)] = r;
			break;
	}
	//led[y*8+x] = r;
	//printf("Xor Result:%i\n", (y%2)^1);
	//led[y*8+((y%2)*(7-x))+(((y%2)^1)*(x))] = r;

}

void panel::toString(){
	for ( int i = 0 ; i < 8 ; i++ ){
		for ( int j = 0 ; j < 8 ; j++ ){
			printf("[%x],", led[8*i+j]);
		}	
		printf("\n");
	}
}

rgbw panel::getPixel(int x,int y){
	switch (orientation) {
		case 0:
			return led[y*8+x];
		
		case 1:
			return led[y*8+(y%2)*(8-x)+((y%2)^1)*(x)];
	}
	return 0;
}


panel::panel () {
	orientation = 0;
	position = 0;
	strip = 0;
	fill(64,0);
}

panel::panel (uint8_t c_strip, uint8_t c_orientation, uint8_t c_position, rgbw initial_color ){
	memset(this->led, initial_color, 64 * sizeof(rgbw));
	this->orientation = c_orientation;
	position = c_position;
	strip = c_strip;

}

panel::panel ( uint8_t c_orientation, rgbw * input){
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
	orientation = c_orientation;
}

class decke {
	public:
		decke();
		decke (int, int);
		void add ( int, int, int, rgbw, int, int);
		int draw();
		rgbw getPixel(int, int);
		int getx() { return  x;}
		int gety() { return  y;}
		panel * getPanel(uint32_t x, uint32_t y) { return matrix[x][y]; }
		std::vector<panel *>panels;
		panel * matrix[8][8];
	private:
		int x;
		int y;// = (panel **)malloc(sizeof(void *) * 64);
};

decke * g_ceil;

void * serial_background_loop(){

	log_("Background Loop Initiated\n");
	int err = 0;
	char a = 1;

	while (SERIAL_LOOP && IsRun){
		g_ceil->draw();
		err = serial_write(&a, 1);
		err = serial_write(dbuf,SBUFSIZE);
		
		if ( err == -1)
			log_("something's wrong\n");

	}
	return 0;
}

void decke::add ( int strip, int position, int orientation, rgbw initial_color, int x, int y){
	panel * p = new panel (strip, orientation, position,initial_color);
	panels.insert(panels.begin(),p);
	matrix[x][y] = p;
}

rgbw decke::getPixel(int cx, int cy ){
	return ((matrix[cx][cy]))->getPixel(cx%8, cy%8) ; 
}

decke::decke(){
	panels = std::vector<panel *>(4);
	x = 2;
	y = 2;
}

decke::decke ( int cx , int cy ){
	//from parameters
	x = cx;
	y = cy;
	panels = std::vector<panel *>(x*y);
}

int decke::draw(){
	//char * buf = dbuf; 

	/* ATM onboard
	uint8_t strip;
	uint8_t position;
	uint8_t value;

	for  ( auto p : panels){
		for ( int i = 0 ; i < 64 ; i++ ){
			strip = p->strip;
			position = p->position;
			value = p->led[i];

			for ( int n = 0; n < 32; n++){

				buf[ (position * i * 32 ) + n ] |= ( ( ( value >> (31-i)) & 1 ) << 7-strip );

			}
		}
	}*/
	for ( int i = 0; i < 9; i++){
		if (panels[i]) {
			for ( int j = 0; j < 64 ; j++){
				dbuf[((8-i)*64*3)+j*3] = (panels[i]->led[j] >> 16) & 0xFF;
				dbuf[((8-i)*64*3)+j*3+1] = panels[i]->led[j] >> 24 ;
				dbuf[((8-i)*64*3)+j*3+2] = (panels[i]->led[j] >> 8 ) & 0xFF ;
				
				//memcpy(dbuf+((8-i)*64*3)+j*3, &panels[i]->led[j], 3);
			}
		}
	}
	return 0;
}

void g_setPixel( uint32_t x, uint32_t y, rgbw r){
	if ( x < 25 && y < 25)
		g_ceil->getPanel(x/8,y/8)->setPixel(x%8,y%8, r);
	//g_ceil->draw();
	//g_ceil->getPanel(x/8,y/8)->toString();
}

void * handle_client_fast( int sock){
	uint8_t buf[1024];
	uint8_t offset = 0;
	int read_size = 0;
	while( (read_size = recv(sock , buf + offset, sizeof(buf) - offset , 0)) > 0){
		offset = parse(buf, read_size);
		memcpy(buf, buf+read_size-offset, offset);  	
		}
	close(sock);
	client_thread_count--; 
	return 0;
}

void * handle_clients(unsigned long port){
	 
	int client_sock;
	socklen_t addr_len;
	struct sockaddr_in addr;
	addr_len = sizeof(addr);
	struct timeval tv;

	log_("PF: Starting Pixelflut Server...\n");

	server_sock = socket(PF_INET, SOCK_STREAM, 0);

	tv.tv_sec = CONNECTION_TIMEOUT;
	tv.tv_usec = 0;

	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;

	if (server_sock == -1){
		log_("PF: socket() failed\n");
		return 0;
	}
	int one = 1;
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) < 0)
		log_("PF: setsockopt(SO_REUSEADDR) failed\n");
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(int)) < 0)
		log_("PF: setsockopt(SO_REUSEPORT) failed\n");

	int retries;
	for (retries = 0; bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1 && retries < 10; retries++){
		log_("PF: bind() failed ...retry in 5s\n");
		usleep(5000000);
	}
	if (retries == 10)
		return 0;

	if (listen(server_sock, 4) == -1){
		log_("PF: listen() failed\n");
		return 0;
	}
	log_("PF: Pixelflut Listening...\n");

	setsockopt(server_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,sizeof(struct timeval));
	setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

	while(IsRun){
		client_sock = accept(server_sock, (struct sockaddr*)&addr, &addr_len);
		if(client_sock > 0){
			std::thread t(handle_client_fast,client_sock);
		}
	}
	close(server_sock);
	return 0;
}

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

uint8_t * nextws(uint8_t* s){
	while (*s != ' ')
		s++;
	return s;
}

void * handle_udp(unsigned long p){
	int port = p;

	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s < 0){
		log_("udp socket() failed\n");
		return 0;
	}
	int one = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) < 0)
	log_("udp setsockopt(SO_REUSEADDR) failed\n");

	struct timeval tv;
	tv.tv_sec = CONNECTION_TIMEOUT;
	tv.tv_usec = 0;
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		log_("Error setting socket timeout\n");
	}

	struct sockaddr_in si_me = {0};
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (struct sockaddr*)&si_me, sizeof(si_me)) < 0){
		perror("udp bind() failed");
		return 0;
	}

	#define UDP_BUFFER_SIZE 65536

	uint8_t packet[UDP_BUFFER_SIZE];
	uint16_t x, y;
	uint32_t c;
	uint8_t *k,*l;

	log_("PF: UDP Server Initialized!\n");

	while(IsRun){
		int packet_size = recv(s, packet, UDP_BUFFER_SIZE, 0);
		log_("Packet Received! Size: ", std::to_string(packet_size).c_str());

		if ( packet_size > 0){
			k = packet+3;
		//packet_size = MIN(offset + (pixellength * MAX_PIXELS), packet_size);
			while ( k < packet+packet_size ) {
				l = nextws(k);

				x = (uint16_t) strtol( (char*)k, (char**)&l, 10) ;
				
				k = nextws(l+1);
				
				y = (uint16_t) strtol( (char*)l+1, (char**)&k, 10) ;
				
				c = (uint32_t) strtol( (char*)k+1, (char**)&k+7, 16) << 8;

				k = k+11;
				
				//printf("X:%i Y:%i Color:%u\n", x,y,c);

				g_setPixel(x,y,c);
				//write_pixel_to_screen(x, y, r, g, b, a);
			}
		}
	}
	return 0;
}

inline uint8_t parse(uint8_t *b, uint32_t size){
	uint16_t x, y;
	uint32_t c;
	uint8_t *k,*l;
	k = b+3;
	//packet_size = MIN(offset + (pixellength * MAX_PIXELS), packet_size);
	while ( k < b+size-14 ) {
		l = nextws(k);

		x = (uint16_t) strtol( (char*)k, (char**)&l, 10) ;
		
		k = nextws(l+1);
		
		y = (uint16_t) strtol( (char*)l+1, (char**)&k, 10) ;
		
		c = (uint32_t) strtol( (char*)k+1, (char**)&k+7, 16) << 8;

		k = k+11;
		
		//printf("X:%i Y:%i Color:%u\n", x,y,c);

		g_setPixel(x,y,c);
		//write_pixel_to_screen(x, y, r, g, b, a);
	}

	return b+size-k;

}


void usage(){
	 std::cout << "Usage:\n\t-c: layout Config File path\n\t-a: Interface for http api (default 127.0.0.1)\n\t-p: http port\n\t-P: pixelflut port\n\t-x: panel x dimension (default 2)\n\t-y: panel y dimension (default 2)\n\t-d: Debug without Serial\n\t-S: Serial Port (default /dev/ttyACM0)\n\t-h: help message\n" ;
}


int main(int argc, char *argv[])
{
	uint16_t pixelflut_port = 4444;
	uint16_t httpapi_port = 5555;
	uint16_t ceil_x = 2;
	uint16_t ceil_y = 2;
	std::string path;
	std::string saddr;
	std::string serial_port;
	int opt;
	bool serial_switch = true;
	while ((opt = getopt(argc, argv, "p:P:a:dhS:x:y:c:")) != -1) {
		switch (opt) {
			case 'p':
				 httpapi_port = (uint16_t)atoi(optarg);
				 break;
			case 'P':
				 pixelflut_port = (uint16_t)atoi(optarg);
				 break;
			case 'y':
				 ceil_y = (uint16_t)atoi(optarg);
				 break;
			case 'x':
				 ceil_x = (uint16_t)atoi(optarg);
				 break;
			case 'c':
				if ( optarg != NULL)
					path = std::string(optarg);
				break;
			case 'a':
				if ( optarg != NULL)
					saddr = std::string(optarg);
				break;
			case 'S':
				if (optarg != NULL)
					serial_port = std::string(optarg);
				break;
			case 'd':
				serial_switch = false;
				break;
			case 'h':
			default: /* '?' */
				 usage();
				 return -1;
		}
	}

		//decke d(ceil_x,ceil_y);
		
	decke ceil(ceil_x,ceil_y);

	//d.add( strip , position , orientation , initial_value , x , y);
	
	/*
	ceil.add( 0 , 0 , 0 , 0, 0, 0);
	ceil.add( 0 , 1 , 0 , 0, 1, 0);
	ceil.add( 0 , 2 , 0 , 0, 2, 0);
	ceil.add( 0 , 3 , 0 , 0, 0, 1);	
	ceil.add( 0 , 4 , 0 , 0, 1, 1);
	ceil.add( 0 , 5 , 0 , 0, 2, 1);
	ceil.add( 0 , 6 , 0 , 0, 0, 2);
	ceil.add( 0 , 7 , 0 , 0, 1, 2);
	ceil.add( 0 , 8 , 0 , 0, 2, 2);
	ceil.draw();
	*/

	int i = 0;
	for ( int x = 0; x < ceil_x; x++){
		for (int y = 0; y < ceil_y; y++){
			ceil.add(0,i,0,0,x,y);
			i++;
		}
	}

	g_ceil = &ceil;

	// default values
	// size of ceiling should be determined by reading a saved config file
	// or interaactively laying out the ceiling configuration

	init_log_();
	if (serial_switch)
		init_serial(serial_port);


	std::thread t(&handle_clients,(unsigned long)pixelflut_port);
	std::thread s(&handle_udp,(unsigned long)pixelflut_port);
	std::thread u(init_mosquitto);

	log_("Press Enter to quit.\n");
	std::cin.get();
	IsRun = false;
	log_("Stopping Pixelflut Threads...");
	t.join();
	s.join();
	log_("Stopped");
	 
		return 0;
}


