
#ifndef __customized_MEMS_H
#define __customized_MEMS_H

typedef struct{
	MEMS_I2C* mems;
	float acc[3];
	float gyr[3];
	float mag[3];
	float accBias[3];
	float gyrBias[3];
	float magBias[3];

}MEMS_dev;

void init_MEMS_dev(void);
void calibrateMEMS(void);
int isDataMEMSReady(MEMS_dev*);
int AddMEMSData(char*,char*,int*,int);

#endif /* __customized_MEMS_H */
