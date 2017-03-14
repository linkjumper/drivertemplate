#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_PATH	"/dev/gpioirq17"
int main(){
        int fd, interrupts = 0;
        fd = open(DEVICE_PATH, O_RDONLY);
	printf("fd = %d\n", fd);
	if(fd<0){
		perror(DEVICE_PATH);
		return -1;
	}
	while(1){
		read(fd, &interrupts, sizeof(interrupts));
		printf("interrupts: %d\n", interrupts);
	}
        return 0;
}

