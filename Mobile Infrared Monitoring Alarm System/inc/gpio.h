#ifndef GPIO_H_
#define GPIO_H_

#define GPIO_MAX 6

extern char gpio_status[GPIO_MAX];

int gpio_init();
int gpio_uninit(int fd);
int gpio_read_status(int fd);
int gpio_analyze_status();

#endif
