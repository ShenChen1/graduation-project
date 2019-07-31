#include "ftp.h"
#include "SIM900A.h"
#include "video.h"
#include "gpio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

struct info{
	char *phone;
	char *url;
	char *user;
	char *code;
}user_info;


static int video_num = 0;
static int pic_num[VIDEO_MAX] = {0};
static pthread_mutex_t video_mutex[VIDEO_MAX];
static pthread_mutex_t gsm_gprs_mutex = PTHREAD_MUTEX_INITIALIZER;

void *thread(void *arg)
{
	pthread_detach(pthread_self());

	printf("place %d video_mutex lock\n", (int)arg);
	pthread_mutex_lock(&video_mutex[(int)arg]);
	printf("place %d video_mutex lock ok\n", (int)arg);
	//抓图保存
	VIDEO_Streamon((int)arg);
	VIDEO_GetStream((int)arg);
	VIDEO_Streamoff((int)arg);
	char name[32];
	sprintf(name, "place%d_pic%d.jpeg", (int)arg, pic_num[(int)arg]++);
	FILE *fp=fopen(name, "w");
	if(fp == NULL)
	{
		fprintf(stderr, "fopen err\n");
		pthread_exit(0);
	}
	VIDEO_Saveimagetofile((int)arg, 0, fp);
	VIDEO_ReleaseStream((int)arg);
	fclose(fp);
	printf("place %d video_mutex unlock\n", (int)arg);
	pthread_mutex_unlock(&video_mutex[(int)arg]);
	printf("place %d video_mutex unlock ok\n", (int)arg);
	usleep(100);

	printf("place %d gsm_gprs_mutex lock\n", (int)arg);
	pthread_mutex_lock(&gsm_gprs_mutex);
	printf("place %d gsm_gprs_mutex lock ok\n", (int)arg);
	//GSM
	int res = 1;
	char buf[64];
	sprintf(buf, "%s     ", user_info.url);
	strcat(buf, name);
	while(res != 0)
	{
		//sim900a_sms_init();
		res = sim900a_sms_send(user_info.phone, buf, strlen(buf));
		sim900a_sms_uninit();
	}
	sleep(1);
	//FTP
	res = 1;
	while(res != 0)
	{
		printf("ppp-on\n");
		system("ppp-on");
		sleep(10);
		system("echo \"nameserver 221.130.252.200\" > /etc/resolv.conf");
		system("cat /etc/resolv.conf");
		int ftp_cmd_fd = -1;
		ftp_cmd_fd = ftp_connect_to_server(user_info.url, DEFAULT_FTP_PORT);
		ftp_login_server(ftp_cmd_fd, user_info.user, user_info.code);
		ftp_chdir(ftp_cmd_fd, "/");
		res = ftp_send_file_pasv(ftp_cmd_fd, TYPE_IMAGE, name, name);
		ftp_quit(ftp_cmd_fd);
		printf("ppp-off\n");
		system("ppp-off");
		sleep(3);
	}

	sim900a_sms_init();
	printf("place %d gsm_gprs_mutex unlock\n", (int)arg);
	pthread_mutex_unlock(&gsm_gprs_mutex);
	printf("place %d gsm_gprs_mutex unlock ok\n", (int)arg);


	pthread_exit(0);
	return NULL;
}


int main(int argc, char *argv[])
{
	int i;
	int gpio_fd;
	pthread_t tid;
	int status;
	char phone[32];
	char msg[256];

	if(argc != 5)
	{
		fprintf(stderr, "usage: ./main phone URL user code\n");
		exit(1);
	}
	user_info.phone = argv[1];
	user_info.url = argv[2];
	user_info.user = argv[3];
	user_info.code = argv[4];

	for(i = 0; i < VIDEO_MAX; i++)
	{
		if(VIDEO_Open(i) == -1)
			break;
		pthread_mutex_init(&video_mutex[i], NULL);
	}
	video_num = i;
	gpio_fd = gpio_init();
	sim900a_sms_init();

	while(1)
	{
		pthread_mutex_lock(&gsm_gprs_mutex);
		if(sim900a_sms_recv(&status, phone, msg) !=0)
		{
			if(status == 1 && strstr(msg, "capture") != NULL && strcmp(user_info.phone, phone) == 0)
			{
				pthread_create(&tid, NULL, thread, (void *)0);
				sleep(1);
			}
		}
		pthread_mutex_unlock(&gsm_gprs_mutex);

		gpio_read_status(gpio_fd);
		if(gpio_analyze_status() == 0)
		{
			printf("No data in 5s\n");
			continue;
		}

		for(i = 0; i < video_num; i++)
		{
			if(gpio_status[i] == '0')
			{
				printf("There is something in place %d \n", i);
				pthread_create(&tid, NULL, thread, (void *)i);
			}
		}

	}

	gpio_uninit(gpio_fd);
	for(i = 0; i < video_num; i++)
	{
		pthread_mutex_destroy(&video_mutex[i]);
		VIDEO_Close(i);
	}


	return 0;
}
