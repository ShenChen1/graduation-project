#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ftp.h"
#include "net.h"

#define CMD_BUF_SIZE            1024
#define PASV_CMD                "PASV \r\n"
#define QUIT_CMD                "QUIT \r\n"
#define CONNECTION_READY        220
#define USER_OK                 331
#define PWD_OK                  230
#define CWD_OK                  250
#define OP_DONE                 200

#define DEBUG

static int ftp_send_cmd(int ftp_cmd_fd, char *cmd_buf, int size)
{
	int err;
	if(cmd_buf == NULL)
		return -1;
#ifdef DEBUG
	printf("FTP Client Send: %s",cmd_buf);
#endif
	if(send_data(ftp_cmd_fd, cmd_buf, strlen(cmd_buf)) == -1)
	{
		fprintf(stderr, "send_data() error.");
		return -1;
	}
	bzero(cmd_buf,size);
	recv(ftp_cmd_fd, cmd_buf, size, 0);

#ifdef DEBUG
	printf("FTP Server Return: %s",cmd_buf);
#endif
	err = atoi(cmd_buf);
	return err;
}

int ftp_connect_to_server(char *ip, char *port)
{
	int ftp_cmd_fd;
	char tmp_buf[CMD_BUF_SIZE] = {0};
	if(ip == NULL)
		return -1;
	if(port == NULL)
		port = DEFAULT_FTP_PORT;
	if((ftp_cmd_fd = tcp_connect(ip, port)) == -1)
		return -1;
	recv(ftp_cmd_fd, tmp_buf, CMD_BUF_SIZE, 0);
	if(atoi(tmp_buf) != CONNECTION_READY)
		return -1;

	return ftp_cmd_fd;
}


static void get_pasv_ip_port(char *cmd, char *ip, char *port)
{
	int count = 0;
	char *begin = NULL;
	char *end = NULL;
	char *tmp;
	begin = strchr(cmd, '(') + 1;
	end = strchr(cmd, ')');
	if(begin == NULL || end == NULL)
		return ;

	tmp = begin;
	*end = '\0';
	while(begin < end)
	{
		if(*begin == ',')
		{
			count++;
			if(count == 4)
				break;
			else
				*begin = '.';
		}
		begin++;
	}
	*begin++ = '\0';
	strcpy(ip, tmp);
	printf("Debug:ip = %s\n",ip);

	tmp = begin;
	while(*begin != ',')
		begin++;
	*begin = '\0';
	count = atoi(tmp);
	begin++;
	count = count * 256 + atoi(begin);
	sprintf(port, "%d", count);
	printf("Debug:port = %s\n", port);
}

static int ftp_pasv_get_xfersocket(int ftp_cmd_fd)
{
	char cmd[CMD_BUF_SIZE] = {0};
	char ip[16] = {0};
	char port[8] = {0};

	strcpy(cmd, PASV_CMD);
	ftp_send_cmd(ftp_cmd_fd, cmd, CMD_BUF_SIZE);
	get_pasv_ip_port(cmd, ip, port);
	if(strlen(ip) == 0 || strlen(port) == 0)
		return -1;

	return tcp_connect(ip, port);
}

int ftp_login_server(int ftp_cmd_fd, char *user, char *password)
{
	int ret;
	char cmd[CMD_BUF_SIZE] = {0};
	sprintf(cmd, "USER %s\r\n", user);
	if((ret = ftp_send_cmd(ftp_cmd_fd, cmd, CMD_BUF_SIZE)) != USER_OK)
	{
		fprintf(stderr, "FTP server : %d.\n", ret);
		return -1;
	}
	bzero(cmd, CMD_BUF_SIZE);
	sprintf(cmd, "PASS %s\r\n", password);
	if(ftp_send_cmd(ftp_cmd_fd, cmd, CMD_BUF_SIZE) != PWD_OK)
		return -1;
	return 0;
}

int ftp_chdir(int ftp_cmd_fd, char *new_dir)
{
	char cmd[CMD_BUF_SIZE] = {0};
	if(new_dir == NULL)
		return -1;
	sprintf(cmd, "CWD %s\r\n", new_dir);
	if(ftp_send_cmd(ftp_cmd_fd, cmd, CMD_BUF_SIZE) != CWD_OK)
		return -1;

	return 0;
}

int ftp_set_xfer_type(int ftp_cmd_fd, int type)
{
	char cmd[CMD_BUF_SIZE] = {0};
	switch(type)
	{
		case TYPE_IMAGE:
			strcpy(cmd, "TYPE I\r\n");
			break;
		case TYPE_ASCII:
			strcpy(cmd,	"TYPE A\r\n");
			break;
		default:
			return -1;
	}
	if(ftp_send_cmd(ftp_cmd_fd, cmd, CMD_BUF_SIZE) != OP_DONE)
		return -1;
	return 0;
}

int ftp_send_file_pasv(int ftp_cmd_fd, int type, char *local_filename, char *remote_filename)
{
	int ftp_xfer_fd;
	char cmd[CMD_BUF_SIZE] = {0};
	int cur_bytes_read = 0;
	int fd;
	char buf[MAXLINE];
	fd = open(local_filename, O_RDONLY);
	if(fd < 0)
		return -1;
	if(ftp_set_xfer_type(ftp_cmd_fd, type) == -1)
		return -1;
	ftp_xfer_fd = ftp_pasv_get_xfersocket(ftp_cmd_fd);
	if(ftp_xfer_fd == -1)
	{
#ifdef DEBUG
		fprintf(stderr, "Cannot open port for transfering.\n");
#endif
		return -1;
	}
	sprintf(cmd, "STOR %s\r\n", remote_filename);
	ftp_send_cmd(ftp_cmd_fd, cmd, CMD_BUF_SIZE);
	while(1)
	{
		cur_bytes_read = read(fd, buf, MAXLINE);
		if(cur_bytes_read > 0)
		{
			if(send_data(ftp_xfer_fd, buf, cur_bytes_read) == -1)
				return -1;
		}
		else
			break;
	}
	close(fd);
	close(ftp_xfer_fd);
	return 0;
}

int ftp_quit(int ftp_cmd_fd)
{
	char cmd[CMD_BUF_SIZE] = {0};
	strcpy(cmd, QUIT_CMD);
	ftp_send_cmd(ftp_cmd_fd, cmd, CMD_BUF_SIZE);
	close(ftp_cmd_fd);
	return 0;
}

int main()
{
	int ftp_cmd_fd = -1;

	ftp_cmd_fd = ftp_connect_to_server("shenchen1991.vicp.cc", "21");
	ftp_login_server(ftp_cmd_fd, "sc", "sc");
	ftp_chdir(ftp_cmd_fd, "/");
	ftp_send_file_pasv(ftp_cmd_fd, TYPE_IMAGE, "place0_pic0.jpeg", "place0_pic0.jpeg");
	ftp_quit(ftp_cmd_fd);

	return 0;
}
