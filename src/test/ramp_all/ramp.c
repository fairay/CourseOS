/*!
 * @file ramp.c
 *
 * @brief Output Test
 *
 * @author Blake Bourque
 *
 * @date 4/8/13
 *
 * Attempts to toggle all available GPIO pins
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>


#include "modgpio.h"

#define NUM_PINS 21
static int pins[NUM_PINS] = {
	2,
	3,
	4,
	7,
	8,
	9,
	10,
	11,
	14,
	15,
	17,
	18,
	22,
	23,
	24,
	25,
	27,
	28,
	29,
	30,
	31,
};

 /*!
 * \brief Toggles pins to make a nice effect
 *
 * Opens gpio file descriptor, reserves pins, sets as output, toggle
 *
 * \param[in]			argc		unused
 * \param[in]			argv		unused
 *
 * \returns	 			0 unless an error occurs
 */
int main(int argc, char * argv[])
{
	int i;
	int fd;
	int ret;
	int t;
	struct gpio_data_mode mydmstruct;

	fd = open("/dev/rpigpio", O_RDONLY); ///hmm, if read only howcome write works?
	if (!fd) {
		perror("open(O_RDONLY)");
		return errno;
	}
	//reserve pins
	for (i=0; i<NUM_PINS; i++) {
		t=pins[i];
		ret = ioctl(fd, GPIO_REQUEST, &t);
		if (ret < 0)
			perror("ioctl");
		else
			printf("Reserved pin %d\n", t);
	}
	//set output
	for (i=0; i<NUM_PINS; i++) {
		t=pins[i];
		mydmstruct.pin = t;
		mydmstruct.data = MODE_OUTPUT;

		ret = ioctl(fd, GPIO_MODE, &mydmstruct);
		if (ret < 0)
			perror("ioctl");
		else
			printf("Set pin %d as OUTPUT\n", t);
	}
	sleep(3);
	//toggle pins
	for (i=0; i<NUM_PINS; i++) {
		t=pins[i];
		ret = ioctl(fd, GPIO_TOGGLE, &t);
		if (ret < 0)
			perror("ioctl");
		else
			printf("Toggled pin %d to %d\n", pins[i], t);
		sleep(1);
	}
	sleep(15);
	return 0;
}

