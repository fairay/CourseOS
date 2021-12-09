/*!
 * @file gpioclient.c
 *
 * @brief Client app for user testing
 *
 * @author Blake Bourque
 *
 * @date 4/8/13
 *
 * reads commands from user and calls corresponding ioctl and prints the result.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdint.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "modgpio.h"

#define RELAIS_PIN	17

int main(int argc, char * argv[])
{
	int fd;
	int ret;
	int v=0;
	int pin = 17;
	char* buf = NULL;
	char* found = NULL;
	struct gpio_data_write mydwstruct;
	struct gpio_data_mode mydmstruct;

	using_history();

	fd = open("/dev/rpigpio", O_RDWR);
	if(!fd) {
		perror("open(O_RDONLY)");
		return errno;
	}

	printf("Commands:\n");
	printf("r <pin> - \t Reserve pin\n");
	printf("f <pin> - \t Free pin\n");
	printf("m <pin> <mode> - Set pin IO mode (0 - input, 1 - output)\n");
	printf("\n");
	printf("g <pin> - \t Get value from pin (r <pin>)\n");
	printf("w <pin> <val> -  Write value to pin\n");
	printf("t <pin> - \t Toggle pin value\n");
	printf("\n");
	printf("q  - \t\t Quit\n");
	printf("\n");

	while (1) {
		if (buf != NULL)
			free(buf);

		buf = readline ("> ");
		if (!buf) { 
			break;
		}
		add_history(buf);

		//accept commands without a space between ctrl char and value
		v=atoi(&buf[2]);

		// Action based on input
		switch (buf[0])
		{
		case 'g':
		case 'G':
			pin = v;
			ret = ioctl(fd, GPIO_READ, &pin);
			if (ret < 0) {
				perror("ioctl");
			}
			printf("Pin %d value=%d\n", v, pin);
			break;

		case 'w':
		case 'W':
			found = strstr(&buf[3], " ");
			if (found == NULL) {
				printf("Missing 2nd parameter. Usage \"w <pin> <val>\"\n");
				continue;
			}

			mydwstruct.pin = v;
			mydwstruct.data = atoi(found);
			printf("Pin %d value=%d\n", pin, v);

			ret = ioctl(fd, GPIO_WRITE, (unsigned long)&mydwstruct);
			printf("%d\n", errno);
			if (ret < 0)
				perror("ioctl");
			else
				printf("Wrote %d to pin %d\n",mydwstruct.data, mydwstruct.pin);
			break;

		case 'r':
		case 'R':
			pin = v;
			ret = ioctl(fd, GPIO_REQUEST, &pin);
			if (ret < 0)
				perror("ioctl");
			else
				printf("Reserved pin %d\n", pin);
			break;
		
		case 'f':
		case 'F':
			pin = v;
			ret = ioctl(fd, GPIO_FREE, &pin);
			if (ret < 0)
				perror("ioctl");
			else
				printf("Freed pin %d\n", pin);
			break;
		
		case 't':
		case 'T':
			pin = v;
			ret = ioctl(fd, GPIO_TOGGLE, &pin);
			if (ret < 0)
				perror("ioctl");
			else
				printf("Toggled pin %d to %d\n", v, pin);
			break;
		
		case 'm':
		case 'M':
			found = strstr(&buf[3], " ");
			if (found == NULL) {
				printf("Missing 2nd parameter. Usage \"m <val> <pin>\"\n");
				continue;
			}

			mydmstruct.pin = v;
			mydmstruct.data = atoi(found)?MODE_OUTPUT:MODE_INPUT;

			ret = ioctl(fd, GPIO_MODE, &mydmstruct);
			if (ret < 0)
				perror("ioctl");
			else
				printf("Set pin %d as %s \n", mydmstruct.pin, mydmstruct.data?"OUTPUT":"INPUT");
			break;
		
		case 'q':
		case 'Q':
			break;
		default:
			printf("Unknown Command \n");;
		}

		if ((buf[0]=='q')||(buf[0]=='Q')) {
			break;
		}
	}
	clear_history();
	printf("\n");
	close(fd);
	return 0;
}
