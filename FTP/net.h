#ifndef NET_H
#define NET_H

#define MAXLINE 512
#define LISTENQ 1024


int tcp_connect(const char *host, const char *serv);
int tcp_listen(const char *host, const char *serv, socklen_t *addrlenp);
int send_data(int sock_fd, const char *buf, int size);

#endif
