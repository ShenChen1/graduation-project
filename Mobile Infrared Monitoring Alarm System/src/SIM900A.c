#include <termios.h>            /* tcgetattr, tcsetattr */
#include <stdio.h>              /* perror, printf, puts, fprintf, fputs */
#include <unistd.h>             /* read, write, close */
#include <fcntl.h>              /* open */
#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>             /* bzero, memcpy */
#include <stdlib.h>
#include <limits.h>             /* CHAR_MAX */

#include "serial.h"
#include "SIM900A.h"

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>             /* close */
#include <arpa/inet.h>

int __recv_sms()
{
	int res;
	char buf[32];
	char *p;
	res = ReadComPort(buf, sizeof(buf));
	if(res == 0)
		return -1;
	//+CMTI: "SM",3
	p = strstr(buf, "\"SM\",");
	if(p == NULL)
		return -2;
	p += 5;
	return atoi(p);
}

int __send_cmd(char *cmd, char *ack, int waittime)
{
	int res = 0;
	int i;
	char buf[512] = {0};
	sprintf(buf, "%s\r\n", cmd);
	res = WriteComPort(buf, strlen(buf) + 1);
	if(res != strlen(buf) + 1)
	{
		fprintf(stderr, "WriteComPort error\n");
		return -1;
	}
	usleep(1000);
	if(ack && waittime)
	{
		while(--waittime)
		{
			memset(buf,0,sizeof(buf));
			res = ReadComPort(buf, sizeof(buf));
			for(i = 0; i < res; i++)
			{
				if(strncmp(&buf[i], ack, strlen(ack)) == 0)
					return 0;
			}
		}
		if(waittime==0)
			res=-2;
	}

	return res;
}

int sim900a_sms_init()
{
	OpenComPort(0, 115200, 8, "1", 'N');
	if(__send_cmd("AT+CSCS=\"GSM\"","OK",50))
	{
		fprintf(stderr, "AT+CSCS=\"GSM\" error\n");
		return -1;
	}
	usleep(200);
	if(__send_cmd("AT+CMGF=1","OK",50))
	{
		fprintf(stderr, "AT+CMGF=1 error\n");
		return -2;
	}
	usleep(200);
	if(__send_cmd("AT+CNMI=2,1","OK",50))
	{
		fprintf(stderr, "AT+CMGF=1 error\n");
		return -3;
	}
	usleep(200);
	printf("SIM900A_SMS_Init OK\n");
	return 0;
}

int sim900a_sms_uninit()
{
	CloseComPort();
	return 0;
}


int sim900a_sms_send(char *phone, char *msg, int len)
{
	char buf[512] = {0};
	if(phone == NULL || msg == NULL || len == 0)
		return -1;
	sprintf(buf, "AT+CMGS=\"%s\"", phone);
	if(__send_cmd(buf, ">", 50) != 0)
	{
		fprintf(stderr, "AT+CMGS=\"%s\" error\n", phone);
		return -1;
	}
	printf("Set phone number OK\n");
	WriteComPort(msg, len);//发送内容
	if(__send_cmd("\x1A", "+CMGS:", 100) != 0)
	{
		fprintf(stderr, "+CMGS: error\n");
		return -2;
	}
	printf("SMS send OK\n");
	return 0;
}

int sim900a_sms_recv(int *status, char *phone, char *msg)
{
	char *p1, *p2;
	char buf[512] = {0};
	int res;
	int whereis;

	*status = 0;
	memset(phone, strlen(phone), 0);
	memset(msg, strlen(msg), 0);

	whereis = __recv_sms();
	if(whereis < 0)
		return -1;
	sprintf(buf, "AT+CMGR=%d\r\n", whereis);
	if(WriteComPort(buf, strlen(buf) + 1) != strlen(buf) + 1)
	{
		fprintf(stderr, "AT+CMGR=%d error\n", whereis);
		return -1;
	}

	usleep(1000);
	while(1)
	{
		res = ReadComPort(buf, sizeof(buf));
		if(res > 0)
			break;
	}

	p1 = strstr(buf, "\r\n");
	p1 += 3;
	//+CMGR: "REC UNREAD","+8613666629031","","14/02/16,16:20:58+32"
	if(strstr(p1,"+CMGR:") == NULL)
	{
		fprintf(stderr, "+CMGR: error\n");
		return -1;
	}
	if(status != NULL)
	{
		*status = (strstr(p1,"\"REC READ\"") == NULL) ? 1 : 0;
		printf("status:%d\n", *status);
	}
	if(phone != NULL)
	{
		p1 = strstr(p1,",");
		p2 = strstr(p1 + 2,"\"");
		*p2 = 0;
		memcpy(phone, p1 + 5, strlen(p1 + 5) + 1);
		printf("phone:%s\n", phone);
	}
	if(msg != NULL)
	{
		p1 = strstr(p2 + 1,"\r\n");
		p2 = strstr(p1,"OK");
		printf("%c\n",*(p1+1));
		*(p2-4) = 0;
		memcpy(msg, p1 + 1, strlen(p1 + 1) + 1);
		printf("msg:%s\n", msg);
	}
	printf("SMS receive OK\n");
	return 0;
}

/*
int main()
{
	int status;
	char phone[16];
	char msg[32];

	SIM900A_SMS_Init();
	SIM900A_SMS_Send("13173680281", "shenchen", strlen("shenchen"));
	SIM900A_SMS_Recv(5, &status, phone, msg);
	SIM900A_SMS_Uninit();
	return 0;
}
*/
