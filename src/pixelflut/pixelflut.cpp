//#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "../serial/serial_control.cpp"

#define BUFSIZE 2048

#define XSTR(a) #a
#define STR(a) XSTR(a)
#define UDP_PROTOCOL 1

static int bytesPerPixel = 4;
//static uint8_t* pixels;
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
int PORT;
#define CONNECTION_TIMEOUT 1;

decke *d;

void * handle_client(void *);
void * handle_clients(void *);

char * itoa(int n, char *s)
{
   int i, j, l, sign;
   char c;
   if ((sign = n) < 0)  /* record sign */
      n = -n;          /* make n positive */
   i = 0;
   do {       /* generate digits in reverse order */
      s[i++] = n % 10 + '0';   /* get next digit */
   } while ((n /= 10) > 0);     /* delete it */
   if (sign < 0)
      s[i++] = '-';
   s[i] = '\0';
   l = i;
   for (i = 0, j = l-1; i<j; i++, j--) {
      c = s[i];
      s[i] = s[j];
      s[j] = c;
   }
   return s + l;
}

void * handle_client(void *s){
   client_thread_count++;
   int sock = *(int*)s;
   char buf[BUFSIZE];
   int read_size, read_pos = 0;
   uint32_t x,y,c;
   int32_t offset_x = 0, offset_y = 0;
   while(running && (read_size = recv(sock , buf + read_pos, sizeof(buf) - read_pos , 0)) > 0){
      read_pos += read_size;
      int found;
      do{
         found = 0;
         for (int i = 0; i < read_pos; i++){
            if (buf[i] == '\n'){
               buf[i] = 0;
               if(!strncmp(buf, "PX ", 3)){ // ...don't ask :D...
                  char *pos1 = buf + 3;
                  x = strtoul(buf + 3, &pos1, 10);
                  if(buf != pos1){
                     pos1++;
                     char *pos2 = pos1;
                     y = strtoul(pos1, &pos2, 10);
                     if(pos1 != pos2){
                        x += offset_x;
                        y += offset_y;

                        pos2++;
                        pos1 = pos2;
                        c = strtoul(pos2, &pos1, 16);
                        if(pos2 != pos1){
                           int codelen = pos1 - pos2;
                           //set_pixel(x, y, r, g, b, a);
                           //printf("Update: %lu\n",c);
                           d->setPixel(x,y,c);
                        }
                        else if((x < PIXEL_WIDTH) && (y < PIXEL_HEIGHT)){
                           char colorout[30];
                           //snprintf(colorout, sizeof(colorout), "PX %d %d %06x\n",x,y, pixels[y * PIXEL_WIDTH + x] & 0xffffff);
                           send(sock, colorout, sizeof(colorout) - 1, MSG_DONTWAIT | MSG_NOSIGNAL);
                        }
                     }
                  }
               } else if(!strncmp(buf, "OFFSET ", 7)){
                   int32_t x, y;
                   if (sscanf(buf + 7, "%i %i", &x,&y) == 2){
                       offset_x = x;
                       offset_y = y;
                   }
               } else if(!strncmp(buf, "SIZE", 4)){
                  static const char out[] = "SIZE " STR(PIXEL_WIDTH) " " STR(PIXEL_HEIGHT) "\n";
                  send(sock, out, sizeof(out) - 1, MSG_DONTWAIT | MSG_NOSIGNAL);
               }
               else if(!strncmp(buf, "CONNECTIONS", 11)){
                  char out[32];
                  sprintf(out, "CONNECTIONS %d\n", client_thread_count);
                  send(sock, out, strlen(out), MSG_DONTWAIT | MSG_NOSIGNAL);
               }
               else if(!strncmp(buf, "HELP", 4)){
                  static const char out[] =
                     "send pixel: 'PX {x} {y} {GG or RRGGBB or RRGGBBAA as HEX}\\n'; "
                     "request pixel: 'PX {x} {y}\\n'; "
                     "request resolution: 'SIZE\\n'; "
                     "request client connection count: 'CONNECTIONS\\n'; "
                     "request this help message: 'HELP\\n';\n";
                  send(sock, out, sizeof(out) - 1, MSG_DONTWAIT | MSG_NOSIGNAL);
               }
               else{
                  /*printf("BULLSHIT[%i]: ", i);
                  int j;
                  for (j = 0; j < i; j++)
                     printf("%c", buf[j]);
                  printf("\n");*/
                  break;
               }
               int offset = i + 1;
               int count = read_pos - offset;
               if (count > 0)
                  memmove(buf, buf + offset, count); // TODO: ring buffer?
               read_pos -= offset;
               found = 1;
               break;
            }
         }
         if (sizeof(buf) - read_pos == 0){ // received only garbage for a whole buffer. start over!
            //buf[sizeof(buf) - 1] = 0;
            //printf("BULLSHIT BUFFER: %s\n", buf);
            read_pos = 0;
         }
      }while (found);
   }

   close(sock);
   //printf("Client disconnected\n");
   fflush(stdout);
   client_thread_count--;
   return 0;
}

void * handle_clients(void * foobar){
   (void)foobar;

   pthread_t thread_id;
   int client_sock;
   socklen_t addr_len;
   struct sockaddr_in addr;
   addr_len = sizeof(addr);
   struct timeval tv;

   log("Starting Pixelflut Server...\n");

   server_sock = socket(PF_INET, SOCK_STREAM, 0);

   tv.tv_sec = CONNECTION_TIMEOUT;
   tv.tv_usec = 0;

   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_port = htons(PORT);
   addr.sin_family = AF_INET;

   if (server_sock == -1){
      perror("socket() failed");
      return 0;
   }
   int one = 1;
   if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) < 0)
      printf("setsockopt(SO_REUSEADDR) failed\n");
   if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(int)) < 0)
      printf("setsockopt(SO_REUSEPORT) failed\n");

   int retries;
   for (retries = 0; bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1 && retries < 10; retries++){
      perror("bind() failed ...retry in 5s");
      usleep(5000000);
   }
   if (retries == 10)
      return 0;

   if (listen(server_sock, 4) == -1){
      perror("listen() failed");
      return 0;
   }
   log("Pixelflut Listening...\n");

   setsockopt(server_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,sizeof(struct timeval));
   setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

   while(running){
      client_sock = accept(server_sock, (struct sockaddr*)&addr, &addr_len);
      if(client_sock > 0){
         //printf("Client %s connected\n", inet_ntoa(addr.sin_addr));
         if( pthread_create( &thread_id , NULL ,  handle_client , (void*) &client_sock) < 0)
         {
            close(client_sock);
            perror("could not create thread");
         }
      }
   }
   close(server_sock);
   return 0;
}


#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#if UDP_PROTOCOL
void * handle_udp(void * foobar){
     (void)foobar;

     int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
     if (s < 0){
      perror("udp socket() failed");
      return 0;
    }
    int one = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) < 0)
    printf("udp setsockopt(SO_REUSEADDR) failed\n");

    struct timeval tv;
    tv.tv_sec = CONNECTION_TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
      printf("Error setting socket timeout\n");
    }

    struct sockaddr_in si_me = {0};
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr*)&si_me, sizeof(si_me)) < 0){
      perror("udp bind() failed");
      return 0;
    }

    #define UDP_BUFFER_SIZE 65507

    uint8_t packet[UDP_BUFFER_SIZE];
    ssize_t packet_size;
    uint16_t x, y;
    uint32_t c;
    uint8_t pixellength = 0, protocol, version;
    uint16_t i;
    uint8_t offset = 2;

    while(running){
      int packet_size = recv(s, packet, UDP_BUFFER_SIZE, 0);

      if (packet_size < 0){
          if(errno != EAGAIN)
              perror("udp recv() failed");
              continue;
      }
      version = packet[1];    // the version
      protocol = packet[0];
      switch (protocol) {
        // Protocol 0: xyrgb 16:16:8:8:8 specified for each pixel
        case 0:
          pixellength = 7;
          //packet_size = MIN(offset + (pixellength * MAX_PIXELS), packet_size);
          for (i=offset; i+pixellength<packet_size; i+=pixellength) {
              x = packet[i  ] | packet[i+1]<<8;
              y = packet[i+2] | packet[i+3]<<8;
              c = packet[i+4] << 24 | packet[i+5] << 16 | packet[i+6] << 8;
              d->setPixel(x,y,c);
               //write_pixel_to_screen(x, y, r, g, b, a);
          }
          break;

        // Protocol 1: xyrgba 16:16:8:8:8:8 specified for each pixel
        case 1:
          pixellength = 8;
          //packet_size = MIN(offset + (pixellength * MAX_PIXELS), packet_size);
          for (i=offset; i+pixellength<packet_size; i+=pixellength) {
            x = packet[i  ] | packet[i+1]<<8;
            y = packet[i+2] | packet[i+3]<<8;
            c = *((uint32_t *) &packet[i+4]);
            d->setPixel(x,y,c);
          }  
          break;

        // Protocol 2: xyrgb 12:12:8:8:8 specified for each pixel
        case 2:
          pixellength = 6;
          //packet_size = MIN(offset + (pixellength * MAX_PIXELS), packet_size);
          for (i=offset; i<packet_size; i+=pixellength) {
            x = packet[i  ] | (packet[i+1]&0xF0)<<4;
            y = (packet[i+1]&0x0F) | packet[i+2]<<4;
            c = packet[i+4] << 24 | packet[i+5] << 16 | packet[i+6] << 8;
            d->setPixel(x,y,c);
          }
          break;

        // Protocol 3: xyrgba 12:12:8:8:8:8 specified for each pixel
        case 3:
          pixellength = 7;
          //packet_size = MIN(offset + (pixellength * MAX_PIXELS), packet_size);
          for (i=offset; i<packet_size; i+=pixellength) {
            x = packet[i  ] | (packet[i+1]&0xF0)<<4;
            y = (packet[i+1]&0x0F) | packet[i+2]<<4;
            c = *(uint32_t *) packet+i+4;
            d->setPixel(x,y,c);
          }
          break;

        // Protocol 4: xyrgb 12:12:3:3:2 specified for each pixel
        case 4:
          pixellength = 4;
          //packet_size = MIN(offset + (pixellength * MAX_PIXELS), packet_size);
          for (i=offset; i<packet_size; i+=pixellength) {
            x = packet[i  ] | (packet[i+1]&0xF0)<<4;
            y = (packet[i+1]&0x0F) | packet[i+2]<<4;
            c = (packet[i+3] & 0xE0) << 24 | (packet[i+3]<<3 & 0xE0) << 16 | (packet[i+3]<<6 & 0xC0) << 8 ;
            d->setPixel(x,y,c);
          }
          break;

             // Protocol 5: xyrgba 12:12:2:2:2:2 specified for each pixel
        case 5:
          pixellength = 4;
          //packet_size = MIN(offset + (pixellength * MAX_PIXELS), packet_size);
          for (i=offset; i<packet_size; i+=pixellength) {
            x = packet[i  ] | (packet[i+1]&0xF0)<<4;
            y = (packet[i+1]&0x0F) | packet[i+2]<<4;
            c = (packet[i+3] & 0xC0) << 24 | (packet[i+3]<<2 & 0xC0) << 16 | (packet[i+3]<<4 & 0xC0) << 8 | packet[i+3]<<6 & 0xC0;
            d->setPixel(x,y,c);
          }
          break;

        // Error no recognied protocol defined
        default:
        break;
      }
  }
  return 0;
}

pthread_t udp_thread_id;
#endif

pthread_t thread_id;
int server_start(int port, decke &dd)
{

   d = &dd;
   PORT = port;

   if(pthread_create(&thread_id , NULL, handle_clients , NULL) < 0){
      perror("could not create tcp thread");
      running = 0;
      return 0;
   }

   if(pthread_create(&udp_thread_id , NULL, handle_udp , NULL) < 0){
      perror("could not create udp thread");
      running = 0;
      return 0;
   }

   return 0;
}

void server_stop()
{
   running = 0;
   printf("Shutting Down %d childs ...\n", client_thread_count);
   while (client_thread_count)
      usleep(100000);
   printf("Shutting Down socket ...\n");
   close(server_sock);

   printf("Joining threads ... ");
   printf("TCP ... ");
   pthread_join(thread_id, NULL);

#if UDP_PROTOCOL
   printf("UDP ... ");
   pthread_join(udp_thread_id, NULL);
#endif

   printf("done\n");
   printf("Cleaning memory\n");
   //free(pixels);

#if SERVE_HISTOGRAM
   free(index_html);
#endif
}
