#ifndef FTP_H_
#define FTP_H_

#define TYPE_IMAGE              0
#define TYPE_ASCII              1
#define DEFAULT_FTP_PORT        "21"

int ftp_connect_to_server(char *ip, char *port);
int ftp_login_server(int ftp_cmd_fd, char *user, char *password);
int ftp_chdir(int ftp_cmd_fd, char *new_dir);
int ftp_set_xfer_type(int ftp_cmd_fd, int type);
int ftp_send_file_pasv(int ftp_cmd_fd, int type, char *local_filename, char *remote_filename);
int ftp_quit(int ftp_cmd_fd);


#endif
