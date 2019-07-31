#ifndef SIM900A_H_
#define SIM900A_H_


int sim900a_sms_init();
int sim900a_sms_uninit();
int sim900a_sms_send(char *phone, char *msg, int len);
int sim900a_sms_recv(int *status, char *phone, char *msg);

#endif /* SIM900A_H_ */
