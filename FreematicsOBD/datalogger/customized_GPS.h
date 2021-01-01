#ifndef __customized_GPS_H
#define __customized_GPS_H

//initialize GPS device
void init_GPS_dev(void);

//check for GPS Data
int isDataGPSReady(int);

//return -1 if GPS coordinates are incorrect accroding to speed
int FilterGpsData(char*, double*);

//check if the vehicle has stopped
void isVehicleStopped(char*,double);

int AddGPSData(char*,char*,int*);

#endif /* __customized_GPS_H */
