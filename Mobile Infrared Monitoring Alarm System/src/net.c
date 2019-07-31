#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include "net.h"


//创建tcp连接并连接到服务器
int tcp_connect(const char *host, const char *serv)
{
	int sockfd,n;
	struct addrinfo hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((n = getaddrinfo(host, serv, &hints, &res))!=0)
	{
		fprintf(stderr, "tcp_connect() getaddrinfo error for %s,%s:%s\n", host, serv, gai_strerror(n));
		return(-1);
	}
	ressave = res;
	do{
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if(sockfd < 0)
		{
			continue;//ignore this one
		}
		if(connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
		{
			break;//success
		}
		close(sockfd);
	}while((res = res->ai_next) != NULL);

	if(res == NULL)//errno set from final connect()
	{
		fprintf(stderr, "tcp_connect error for %s,%s\n", host, serv);
		return(-1);
	}

	freeaddrinfo(ressave);
	return sockfd;
}

//创建tcp连接并绑定到服务器总所周知的端口
int tcp_listen(const char *host, const char *serv, socklen_t *addrlenp)
{
	int listenfd, n;
	const int on = 1;
	struct addrinfo hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((n =getaddrinfo(host, serv, &hints, &res)) != 0)
	{
		printf("tcp_listen error for %s,%s:%s\n",host,serv,gai_strerror(n));
		return(-1);
	}
	ressave = res;

	do{
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listenfd < 0)
			continue;		/* error, try next one */

		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
			break;			/* success */

		close(listenfd);	/* bind error, close and try next one */
	}while((res = res->ai_next)!=NULL);

	if(res == NULL)	/* errno from final socket() or bind() */
	{
		fprintf(stderr, "tcp_listen error for %s, %s", host, serv);
		return(-1);
	}
	listen(listenfd, LISTENQ);

	if(addrlenp)
		*addrlenp = res->ai_addrlen;	/* return size of protocol address */

	freeaddrinfo(ressave);

	return(listenfd);
}


int send_data(int sock_fd, const char *buf, int size)
{
	int total_bytes_sent;
	int cur_bytes_sent;
	if(sock_fd <= 0)
	{
		fprintf(stderr, "send_data():Invalid socket fd %d.", sock_fd);
		return -1;
	}
	total_bytes_sent = cur_bytes_sent = 0;
	if(size > MAXLINE)
	{
		while(total_bytes_sent < size)
		{
			if((cur_bytes_sent = send(sock_fd, buf, MAXLINE, 0)) == -1)
				return -1;
			total_bytes_sent += cur_bytes_sent;
			buf += cur_bytes_sent;
		}
		return total_bytes_sent;
	}
	else
	{
		if((cur_bytes_sent = send(sock_fd, buf, size, 0)) == -1)
			return -1;
		return cur_bytes_sent;
	}
}


