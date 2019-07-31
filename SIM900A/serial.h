#ifndef _SERIAL_H
#define _SERIAL_H

int OpenComPort(int ComPort, int baudrate, int databit, const char *stopbit, char parity);
void CloseComPort(void);
int ReadComPort(char *data, int datalength);
int WriteComPort(char *data, int datalength);


#endif
