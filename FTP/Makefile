TARGET=ftp_send

CC=arm-linux-gcc
CFLAGS=-g
all:$(TARGET)

ftp_send:net.c ftp.c
	$(CC) $(CFLAGS) net.c ftp.c -o ftp_send

clean:
	rm $(TARGET) -f
