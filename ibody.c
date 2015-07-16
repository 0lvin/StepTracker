#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#define PACKET_SIZE 17

void dump_buffer(char* buffer, size_t size) {
    printf("(%zd)\t", size);
    int i = 0;
    for(i=0; i < size; i++) {
	printf(" %02x", buffer[i] & 0xff);
    }
    printf("\n");
}

int write_buffer(int tty, char* out_buffer, size_t out_size, char* in_buffer, size_t in_size){
    int res = -1;
    if (out_size) {
	printf("->\n");
	dump_buffer(out_buffer, out_size);
	res = write(tty, out_buffer, out_size);
	if (res < 0) {
	    printf("Can't write = %d\n", res);
	    return res;
	}
	//sleep(1);
    }
    if (in_size) {
	printf("<-\n");
	memset(in_buffer, 0, in_size);
	int pos = 0;
	while (pos < in_size) {
	    res = read(tty, in_buffer + pos, in_size - pos);
	    if (res < 0) {
		printf("Can't read = %d\n", res);
		return res;
	    }
	    pos += res;
	}
	dump_buffer(in_buffer, in_size);
	return pos;
    }
    return res;
}

void set_speed(int tty) {
    struct termios tio;
    memset(&tio,0,sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    // 8n1, see termios.h for more information
    tio.c_cflag=CS8|CREAD|CLOCAL;
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=5;
    // 9600 baud
    cfsetospeed(&tio, B9600);
    cfsetispeed(&tio, B9600);

    tcsetattr(tty, TCSANOW,&tio);
}

int main() {
    char tracker_id[] = {
	0x5a, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9c
    };

    char time_set_17_09_05[] = {
	0x5a, 0x01, 0x15, 0x07, 0x11, 0x17, 0x12, 0x17,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xc8
    };

    int res = 0;

    int tty = open("/dev/ttyUSB0", O_RDWR);
    if (!tty) {
	printf("Can't open\n");
	return res;
    }

    set_speed(tty);

    char buffer[PACKET_SIZE];
    printf("tracker id:\n");
    res = write_buffer(tty, tracker_id, PACKET_SIZE, buffer, PACKET_SIZE);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }
    printf("time:\n");
    res = write_buffer(tty, time_set_17_09_05, PACKET_SIZE, buffer, PACKET_SIZE);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }
    return 0;
}
