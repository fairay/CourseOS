/*!
 * @file forktest.c
 *
 * @brief Locking Test
 *
 * @author Blake Bourque
 *
 * @date 4/8/13
 *
 * Uses two children to test locking in the GPIO kernel module by requesting the same pins.
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
#include <sys/wait.h>

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

void forkChildren(int nChildren, int fd);
void child(int fd);

 /*!
 * \brief Initiates child creation
 *
 * Initiates child creation, then waits for them to die.
 *
 * \param[in]			argc		unused
 * \param[in]			argv		unused
 *
 * \returns	 			0 unless an error occurs
 */
int main(int argc, char *argv[])
{
	int fd, status=0;
	pid_t wpid;

	fd = open("/dev/rpigpio", O_RDONLY); ///hmm, if read only howcome write works?
	if (!fd) {
		perror("open(O_RDONLY)");
		return errno;
	}
	forkChildren(2, fd);
	while ((wpid = wait(&status)) > 0) 	//wait for children to die.
    {
        printf("Exit status of %d was %d (%s)\n", (int)wpid, status,
               (status > 0) ? "accept" : "reject");
    }
	return 0;
}

 /*!
 * \brief Spawns Children
 *
 * Forks the process the number of times specified in the nChildren parameter
 *
 * \param[in]			nChildren	number of children to spawn
 * \param[in]			fd			file descriptor for ioctl
 *
 * \returns	 			void
 */
void forkChildren(int nChildren, int fd) {
	int i;
	pid_t pid;
	for (i = 1; i <= nChildren; i++) {
		pid = fork();
		if (pid == 0) {
			child(fd);
			break;
		}
	}
}
/*!
 * \brief Child Code
 *
 * Sequentially requests pins for the mazimum number of collisions,
 * toggles the pins 4 times
 *
 * \param[in]			fd		file descriptor for ioctl
 *
 * \returns	 			void
 */
void child(int fd) {
	int i, t, ret, j;
	//reserve pins
	for (i=0; i<NUM_PINS; i++) {
		t=pins[i];
		ret = ioctl(fd, GPIO_REQUEST, &t);
		if (ret < 0)
			perror("ioctl");
		else
			printf("Reserved pin %d\n", t);
	}
	//set as output
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
	for (j=0; j<4; j++) {
		//toggle pins
		for (i=0; i<NUM_PINS; i++) {
			t=pins[i];
			ret = ioctl(fd, GPIO_TOGGLE, &t);
			if (ret < 0)
				perror("ioctl");
			else
				printf("Client %d toggled pin %d to %d\n", getpid(), pins[i], t);
		}
		sleep(1);
	}
}