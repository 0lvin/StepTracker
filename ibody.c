#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#define PACKET_SIZE 17

int main() {
    char status[] = {
	0x5A, 0x42, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x9C
    };
    int res = 0;

    int tty = open("/dev/ttyUSB0", O_RDWR | O_NONBLOCK);
    if (!tty) {
	printf("Can't open\n");
	return res;
    }

    if (1) {
	struct termios tio;
	memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag=0;
	tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
	tio.c_lflag=0;
	tio.c_cc[VMIN]=1;
	tio.c_cc[VTIME]=5;

	cfsetospeed(&tio, B9600);            // 115200 baud
	cfsetispeed(&tio, B9600);            // 115200 baud

	tcsetattr(tty, TCSANOW,&tio);
    }
    res = write(tty, status, PACKET_SIZE);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }
    char buffer[16];
    for(;;) {
	sleep(1);
	memset(buffer, 0, 16);
	res = read(tty, buffer, 16);
	printf("s=%d", res);
	int i = 0;
	for(i=0; i < res; i++) {
	    printf(" %02x", buffer[i] & 0xff);
	}
	printf("\n");
    }
    return 0;
}
