#ifndef __customized_OBD_H
#define __customized_OBD_H

typedef struct{
	char label[255];
	byte pid;
	int fr_max;
}ObdInfoPid;


//void init_OBD_dev(FreematicsESP32*);
void init_OBD_dev(void);
int set_obd(void);
int GetOBDInfo(uint8_t, int*);
void ErrorOBDCheck(void);
int OBD_looping(ObdInfoPid*, int,int);
void CheckObdSpeed(char*,char*,int,int*);
int GetOBDSpeed(void);
void setOBDSpeed(int);
#endif /* __customized_OBD_H */
