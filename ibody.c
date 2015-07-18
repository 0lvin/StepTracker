#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#define PACKET_SIZE 17

unsigned char check_code(const char* buffer, size_t size) {
    unsigned int crc = 0;
    int i = 0;
    for (i = 0; i < size; i++) {
	crc = (crc + buffer[i]) % 0x100;
    }
    return crc & 0xff;
}

void dump_hex(char* buffer, size_t size) {
    int i = 0;
    for(i=0; i < size; i++) {
	printf(" %02x", buffer[i] & 0xff);
    }
}

void dump_buffer(char* buffer) {
    dump_hex(buffer, PACKET_SIZE);
    char code = check_code(buffer, PACKET_SIZE-1) & 0xff;
    char current = buffer[PACKET_SIZE-1] & 0xff;
    if (current  != code) {
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
	printf("->");
	dump_buffer(buffer);
	res = write(tty, buffer, PACKET_SIZE);
	if (res < 0) {
	    printf("Can't write = %d\n", res);
	    return res;
	}
    }
    if (in_buffer) {
	printf("<-");
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
	if (out_buffer && memcmp(out_buffer, buffer, 2) != 0) {
	    printf("Different operations\n");
	}
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

unsigned char convert_time(unsigned char orig) {
    orig = orig % 100;
    return ((orig % 0xa) & 0x0f)  + (((orig / 0xa) << 4) & 0xf0);
}

void set_status(char* buffer) {
    memset(buffer, 0, PACKET_SIZE-1);
    buffer[0x00] = 0x5a;
    buffer[0x01] = 0x42;
}

void set_log_start(char* buffer) {
    memset(buffer, 0, PACKET_SIZE-1);
    buffer[0x00] = 0x5a;
    buffer[0x01] = 0x46;
}

void set_dump_start(char* buffer, char day_back) {
    memset(buffer, 0, PACKET_SIZE-1);
    buffer[0x00] = 0x5a;
    buffer[0x01] = 0x43;
    /*
     * 0 - today
     * 1 - yesterday
     */
    buffer[0x02] = day_back;
}

void set_reset(char* buffer) {
    memset(buffer, 0, PACKET_SIZE-1);
    buffer[0x00] = 0x5a;
    buffer[0x01] = 0x2e;
}

void set_time(char* buffer) {
    struct tm time_struct;
    time_t curr_time;
    time(&curr_time);
    if (((time_t) -1) == curr_time) {
	printf("Can't get time\n");
	return;
    }
    localtime_r(&curr_time, &time_struct);
    memset(buffer, 0, PACKET_SIZE-1);
    buffer[0x00] = 0x5a;
    buffer[0x01] = 0x01;
    //The number of years since 1900.
    buffer[0x02] = convert_time(time_struct.tm_year - 100);
    //The number of months since January, in the range 0 to 11.
    buffer[0x03] = convert_time(time_struct.tm_mon + 1);
    buffer[0x04] = convert_time(time_struct.tm_mday);
    buffer[0x05] = convert_time(time_struct.tm_hour);
    buffer[0x06] = convert_time(time_struct.tm_min);
    buffer[0x07] = convert_time(time_struct.tm_sec);
}

/*
 * time -> 1:
 * 5a011507 11170905 00000000 00000000 ad
 * 5a011507 11170909 00000000 00000000 b1
 * 5a011507 11171217 00000000 00000000 c8
 * 5a011507 11171226 00000000 00000000 d7
 * 5a011507 16081455 00000000 00000000 fe
 * 5a011507 16081510 00000000 00000000 ba
 * 5a011507 16081546 00000000 00000000 f0
 * reset -> 0:
 * 5a2e0000 00000000 00000000 00000000 88
 * status -> 1:
 * 5a420000 00000000 00000000 00000000 9c
 * run always before 43? -> 1:
 * 5a460000 00000000 00000000 00000000 a0
 * run sometime after 43? -> 1:
 * 5a040100 00000000 00000000 00000000 5f
 * run always after 46? -> 96:
 * 5a430000 00000000 00000000 00000000 9d
*/
int main() {
    int res = 0;

    int tty = open("/dev/ttyUSB0", O_RDWR);
    if (!tty) {
	printf("Can't open\n");
	return res;
    }

    set_speed(tty);

    char buffer[PACKET_SIZE];
    printf("tracker id:\n");
    set_status(buffer);
    res = write_buffer(tty, buffer, buffer);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }
    printf("-- id will be: ");
    dump_hex(buffer+0x07, 0x06);
    printf("\n-- Other values unknown for now.\n");

    printf("something before dump:\n");
    set_log_start(buffer);
    res = write_buffer(tty, buffer, buffer);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }
    printf("-- values unknown for now.\n");
    printf("dump?:\n");
    set_dump_start(buffer, 1);
    res = write_buffer(tty, buffer, NULL);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }
    int k;
    for (k=0; k < 96; k ++) {
	res = write_buffer(tty, NULL, buffer);
	if (res < PACKET_SIZE) {
	    printf("Can't write = %d\n", res);
	    return res;
	} else {
	    if (buffer[0] != 0x5a || buffer[1] != 0x43) {
		printf("-- wrong opperation code\n");
	    }
	    if ((unsigned char)buffer[2] == (unsigned char)0xf0) {
		char year = buffer[3];
		char month = buffer[4];
		char day = buffer[5];
		char hour = buffer[6] / 4;
		char minutes = (buffer[6] * 15) % 60;
		printf("-- %02x.%02x.%02x %02d:%02d :>", year, month, day, hour, minutes);
		dump_hex(buffer+0x07, 0x07);
		printf("\n");
	    } else if (buffer[2] != 0x00) {
		printf("-- unknow value\n");
	    }
	}
    }

    //already tested, use only undestructive operation
    return 0;
    printf("time:\n");
    set_time(buffer);
    res = write_buffer(tty, buffer, buffer);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }

    printf("reset:\n");
    set_reset(buffer);
    res = write_buffer(tty, buffer, NULL);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }

    // wait for wake up
    sleep(1);
    printf("tracker id:\n");
    set_status(buffer);
    res = write_buffer(tty, buffer, buffer);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }
    return 0;
}
