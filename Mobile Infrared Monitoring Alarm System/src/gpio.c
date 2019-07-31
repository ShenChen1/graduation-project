#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include "gpio.h"

#define DEBUG

char gpio_status[GPIO_MAX] = {'0', '0', '0', '0', '0', '0'};
static char current_gpio_status[GPIO_MAX];

int gpio_init()
{
	int fd;
	fd = open("/dev/buttons", O_RDWR | O_NONBLOCK);
	if(fd < 0)
	{
		perror("open device buttons");
		exit(1);
	}
	return fd;
}

int gpio_uninit(int fd)
{
	close(fd);
	return 0;
}

int gpio_read_status(int fd)
{
	fd_set rfds;
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	select(fd + 1, &rfds, NULL, NULL, &tv);
	if(FD_ISSET(fd, &rfds))
	{
		if(read(fd, current_gpio_status, sizeof(current_gpio_status)) != sizeof(current_gpio_status))
		{
			perror("read buttons:");
			return -1;
		}
	}
	return 0;
}

int gpio_analyze_status()
{
	int count_of_change;
	int i;
	for(i = 0, count_of_change = 0; i < GPIO_MAX; i++)
	{
		if(current_gpio_status[i] != gpio_status[i])
		{
			gpio_status[i] = current_gpio_status[i];
			#ifdef DEBUG
			printf("%skey %d is %s\n", count_of_change? ", ": "", i+1, gpio_status[i] == '0' ? "up" :"down");
			#endif
			count_of_change++;
		}
	}
	printf("\n");
	return count_of_change;
}

/*
int main(void)
{
	int gpio_fd;
	gpio_fd = gpio_init();

	while(1)
	{
		if(gpio_read_status(gpio_fd) != 0)
			continue;

		gpio_analyze_status();
	}


	gpio_uninit(gpio_fd);
	return 0;
}
*/
