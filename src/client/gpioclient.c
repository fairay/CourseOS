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

 /*!
 * \brief Makes ioctl calls based on user input
 *
 * Users can enter any of the characters represented in the regex: /[RWCFTMQ]/i
 * Commands are in the format <ctrl char> <val1> <val2?>
 * val2 is not optional, but only used when writing or changing mode.
 *
 * \param[in]			argc		unused
 * \param[in]			argv		unused
 *
 * \returns	 			0 unless an error occurs
 */
int main(int argc, char * argv[])
{
	int fd;
	int ret;
	int v=0;
	int pin = 17;
	char* buf = NULL;
	char *found = NULL;
	struct gpio_data_write mydwstruct;
	struct gpio_data_mode mydmstruct;

	using_history();

	fd = open("/dev/rpigpio", O_RDONLY);
	if(!fd) {
		perror("open(O_RDONLY)");
		return errno;
	}

	printf("[R]ead [W]rite [C]heckout [F]ree [T]oggle [M]ode [Q]uit\n");

	while (1) {
		if (buf != NULL)
			free(buf);
		// Get new item / delete item
		buf = readline ("\e[01;34mCommand:\e[00m ");
		if (buf == NULL) {
			break;	//NULL on eof
		}
		add_history(buf);

		//accept commands without a space between ctrl char and value
		if (buf[1] == ' ')
			v=atoi(&buf[2]);
		else
			v=atoi(&buf[1]);

		// Action based on input
		if ((buf[0]=='r')||(buf[0]=='R')) {
			pin = v;
			ret = ioctl(fd, GPIO_READ, &pin);
			if (ret < 0) {
				perror("ioctl");
			}
			printf("Pin %d value=%d\n", v, pin);
		} else
		if ((buf[0]=='w')||(buf[0]=='W')) {

			if (buf[1] == ' ')
				found = strstr(&buf[3], " ");
			else
				found = strstr(&buf[2], " ");
			if (found == NULL) {
				printf("Missing 2nd parameter. Usage \"w <val> <pin>\"\n");
				continue;
			}
			pin = atoi(found);
			mydwstruct.pin = pin;
			mydwstruct.data = v;

			ret = ioctl(fd, GPIO_WRITE, (unsigned long)&mydwstruct);
			printf("%d\n", errno);
			if (ret < 0)
				perror("ioctl");
			else
				printf("Wrote %d to pin %d\n",mydwstruct.data, mydwstruct.pin);
		} else
		if ((buf[0]=='c')||(buf[0]=='C')) {
			pin = v;
			ret = ioctl(fd, GPIO_REQUEST, &pin);
			if (ret < 0)
				perror("ioctl");
			else
				printf("Reserved pin %d\n", pin);
		} else
		if ((buf[0]=='f')||(buf[0]=='F')) {
			pin = v;
			ret = ioctl(fd, GPIO_FREE, &pin);
			if (ret < 0)
				perror("ioctl");
			else
				printf("Freed pin %d\n", pin);
		} else
		if ((buf[0]=='t')||(buf[0]=='T')) {
			pin = v;
			ret = ioctl(fd, GPIO_TOGGLE, &pin);
			if (ret < 0)
				perror("ioctl");
			else
				printf("Toggled pin %d to %d\n", v, pin);
		} else
		if ((buf[0]=='m')||(buf[0]=='M')) {
			if (buf[1] == ' ')
				found = strstr(&buf[3], " ");
			else
				found = strstr(&buf[2], " ");
			if (found == NULL) {
				printf("Missing 2nd parameter. Usage \"m <val> <pin>\"\n");
				continue;
			}

			pin = atoi(found);

			mydmstruct.pin = pin;
			mydmstruct.data = v?MODE_OUTPUT:MODE_INPUT;

			ret = ioctl(fd, GPIO_MODE, &mydmstruct);
			if (ret < 0)
				perror("ioctl");
			else
				printf("Set pin %d as %s \n", mydmstruct.pin, mydmstruct.data?"OUTPUT":"INPUT");
		} else
		if ((buf[0]=='q')||(buf[0]=='Q')) {
			break;
		} else {
			printf("Unknown Command \n");
		}
	}
	clear_history();
	printf("\n");
	close(fd);
	return 0;
}
