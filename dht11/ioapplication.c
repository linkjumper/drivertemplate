#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "dht11.h"

#define IOCTL_BASE 0x0100
#define IOCTL_TEST 0x0001 + IOCTL_BASE
#define IOCTL_TEMP 0x0002 + IOCTL_BASE
#define IOCTL_HUMI 0x0003 + IOCTL_BASE

struct dht11 {
	char value;
	char * str;
};


int main(){
   //char str[20];
   int fd, ret;
   struct dht11 dht;

   dht.str = malloc(20*sizeof(char));


   fd = open("/dev/dht11", O_RDWR);
   if(fd<0){
     printf("Error open fd=%d, flag=%d\n",fd, O_RDWR);
     return -1;
   }

   //test
   if((ret = ioctl(fd, IOCTL_TEST,(void*) &dht))<0){
     printf("Error ioctl ret=%d\n",ret);
     return -2;
   }

   printf("IO-String: %s\n",dht.str);

   //temp
   if((ret = ioctl(fd, IOCTL_TEMP,(void*) &dht))<0){
     printf("Error ioctl ret=%d\n",ret);
     return -3;
   }

   printf("IO-Val: %.0f\n",(float)dht.value);

   close(fd);
   free(dht.str);
   return 0;
}
