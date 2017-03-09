#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(){
   char str[20];
   int fd, ret;
   fd = open("/dev/myioctl", O_RDWR); 
   if(fd<0){
     printf("Error open fd=%d, flag=%d\n",fd, O_RDWR);
     return -1;
   }
   if((ret=ioctl(fd, 0x0001,(void*)str))<0){
     printf("Error ioctl ret=%d\n",ret);
     return -2;
   }
   printf("IO-String:%s",str);
   close(fd);
   return 0;
}
