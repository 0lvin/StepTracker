/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
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

void set_dump_start(char* buffer, unsigned char day_back) {
    memset(buffer, 0, PACKET_SIZE-1);
    buffer[0x00] = 0x5a;
    buffer[0x01] = 0x43;
    /*
     * 0 - today
     * 1 - yesterday
     */
    buffer[0x02] = day_back;
}

void set_cleanup_day(char* buffer, unsigned char day_back) {
    memset(buffer, 0, PACKET_SIZE-1);
    buffer[0x00] = 0x5a;
    buffer[0x01] = 0x04;
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
    printf("-- id will be:");
    dump_hex(buffer+0x07, 0x06);
    printf("\n-- Other values unknown for now.\n");

    printf("something before dump:\n");
    set_log_start(buffer);
    res = write_buffer(tty, buffer, buffer);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }
    unsigned int days_in_log = (
	((unsigned char)buffer[0x02] << 24) +
	((unsigned char)buffer[0x03] << 16) +
	((unsigned char)buffer[0x04] << 8) +
	(unsigned char)buffer[0x05]
    );
    printf("-- days in log will be: %08x\n", days_in_log);
    int pos = 0;
    unsigned int check = days_in_log;
    for(pos=0; pos < 32; pos ++) {
	if (check&1) {
	    printf("-- dump for %d days back:\n", pos);
	    set_dump_start(buffer, pos);
	    res = write_buffer(tty, buffer, NULL);
	    if (res < PACKET_SIZE) {
		printf("Can't write = %d\n", res);
		return res;
	    }
	    int k;
	    float sleep_time = 0;
	    int steps_amount = 0;
	    int energy_amount = 0;
	    int full_distance = 0;
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
			int seconds = 0;
			if ((unsigned char)buffer[7] == (unsigned char)0xff) {
			    int second_interval;
			    for(second_interval=0; second_interval < 8; second_interval ++) {
				seconds = second_interval * 112.5; // 15 * 60 / 8;
				printf(
				    "-- %02x.%02x.%02x %02d:%02d:%02d :>",
				    year, month, day, hour, (int)(minutes + seconds / 60), ((int)seconds)%60
				);
				char sleep_value = buffer[0x08 + second_interval];
				char* sleep_type = "deep";
				if (sleep_value > 10) {
				    sleep_type = "does not";
				} else if (sleep_value > 5) {
				    sleep_type = "light";
				}
				if(sleep_value < 10) {
				    sleep_time += 112.5;
				}
				printf(" %s sleep (%02x)\n", sleep_type, sleep_value);
			    }
			} else if ((unsigned char)buffer[7] == (unsigned char)0x00) {
			    int energy = (unsigned char)buffer[8];
			    energy += ((unsigned char)buffer[9]) << 8;
			    energy_amount += energy;

			    int steps = (unsigned char)buffer[10];
			    steps += ((unsigned char)buffer[11]) << 8;
			    steps_amount += steps;

			    int distance = (unsigned char)buffer[12];
			    distance += ((unsigned char)buffer[13]) << 8;
			    full_distance += distance;

			    if (distance || steps || energy)
				printf(
				    "-- %02x.%02x.%02x %02d:%02d:%02d :>%5d steps, %3.3f km, %3.1f kkal\n",
				    year, month, day, hour, minutes, seconds,
				    steps, (float)distance/1000, (float)energy/10
				);
			} else {
			    printf(
				"-- %02x.%02x.%02x %02d:%02d:%02d :>",
				year, month, day, hour, minutes, seconds
			    );
			    printf("s/w?\n");
			}
		    } else if (buffer[2] == 0x00) {
			if ((unsigned char)buffer[3] == (unsigned char)0xff) {
			    printf("-- no data for such day\n");
			    break;
			} else if (buffer[3] != 0x00) {
			    printf("-- unknow value\n");
			}
		    } else {
			printf("-- unknow value\n");
		    }
		}
	    }
	    printf(
		"++ summary %d days back: %.2fh sleep, %.3f km, %.1f kkal, %d steps\n",
		pos, sleep_time/3600, (float)full_distance/1000,
		(float)energy_amount/10,  steps_amount
	    );
	}
	check = check >> 1;
    }

    //already tested, use only undestructive operation
    return 0;
    printf("cleanup 2 days back\n");
    set_cleanup_day(buffer, 2);
    res = write_buffer(tty, buffer, buffer);
    if (res < PACKET_SIZE) {
	printf("Can't write = %d\n", res);
	return res;
    }

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
