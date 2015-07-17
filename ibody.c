#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#define PACKET_SIZE 17

unsigned char check_code(const char* buffer, size_t size) {
    unsigned int crc = 0;
    int i = 0;
    for (i = 0; i < size; i++) {
	crc = (crc + buffer[i]) % 0x100;
    }
    return crc & 0xff;
}

void dump_buffer(char* buffer) {
    int i = 0;
    for(i=0; i < PACKET_SIZE; i++) {
	printf(" %02x", buffer[i] & 0xff);
    }
    unsigned char code = check_code(buffer, PACKET_SIZE-1) & 0xff;
    if (buffer[PACKET_SIZE-1] & 0xff == code) {
	printf(" valid check code: %02x\n", code);
    }
    printf("\n");
}

int write_buffer(int tty, char* out_buffer, char* in_buffer){
    int res = -1;
    char buffer[PACKET_SIZE];
    if (out_buffer) {
	memcpy(buffer, out_buffer, PACKET_SIZE-1);
	buffer[PACKET_SIZE-1] = check_code(buffer, PACKET_SIZE-1);
	printf("->\n");
	dump_buffer(buffer);
	res = write(tty, buffer, PACKET_SIZE);
	if (res < 0) {
	    printf("Can't write = %d\n", res);
	    return res;
	}
	//sleep(1);
    }
    if (in_buffer) {
	printf("<-\n");
	memset(buffer, 0, PACKET_SIZE);
	int pos = 0;
	while (pos < PACKET_SIZE) {
	    res = read(tty, buffer + pos, PACKET_SIZE - pos);
	    if (res < 0) {
		printf("Can't read = %d\n", res);
		return res;
	    }
	    pos += res;
	}
	dump_buffer(buffer);
	memcpy(in_buffer, buffer, PACKET_SIZE);
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

/*
 * time:
 * 5a011507 11170905 00000000 00000000 ad
 * 5a011507 11170909 00000000 00000000 b1
 * 5a011507 11171217 00000000 00000000 c8
 * 5a011507 11171226 00000000 00000000 d7
 * 5a011507 16081455 00000000 00000000 fe
 * 5a011507 16081510 00000000 00000000 ba
 * 5a011507 16081546 00000000 00000000 f0
 * status:
 * 5a420000 00000000 00000000 00000000 9c
*/
int main() {
    char tracker_id[] = {
	0x5a, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    char time_set_17_09_05[] = {
	0x5a, 0x01, 0x15, 0x07, 0x11, 0x17, 0x12, 0x17,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
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
    res = write_buffer(tty, tracker_id, buffer);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }
    printf("time:\n");
    res = write_buffer(tty, time_set_17_09_05, buffer);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }
    return 0;
}
