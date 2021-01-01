#include "simple_obd_test.h"
#include "customized_DateAndTime.h"
#include "customized_MEMS.h"

RTC_DATA_ATTR MEMS_dev md_dev;

void init_MEMS_dev(void){
	#if USE_SERIAL==1
		Serial.print("MEMS:");
	#endif
	md_dev.mems = new MPU9250;
	byte ret = (md_dev.mems)->begin(ENABLE_ORIENTATION);
	if (ret) {
		#if USE_SERIAL==1
			Serial.println("MPU-9250");
		#endif
	} else {
		(md_dev.mems)->end();
		delete md_dev.mems;
		md_dev.mems = new ICM_20948_I2C;
		ret = (md_dev.mems)->begin(ENABLE_ORIENTATION);
		if (ret) {
			#if USE_SERIAL==1
				Serial.println("ICM-20948");
			#endif
		} else {
			#if USE_SERIAL==1
				Serial.println("NO");
			#endif
			while(1);
		}
	}
	esp_sleep_wakeup_cause_t wakeup_reason;
	wakeup_reason = esp_sleep_get_wakeup_cause();
	if (wakeup_reason!=ESP_SLEEP_WAKEUP_TIMER)
		calibrateMEMS();

}


void calibrateMEMS(void){
	float temp=0;
	while (!((md_dev.mems)->read(md_dev.accBias, md_dev.gyrBias, md_dev.magBias, &temp))){}
	#if USE_SERIAL==1
		Serial.println("MEMS is calibrated successfully");
	#endif
}

int isDataMEMSReady(MEMS_dev* md){
	float temp=0;
	if (((md_dev.mems)->read(md_dev.acc, md_dev.gyr, md_dev.mag, &temp))){
		for (int i=0; i<3; i++){
			md_dev.acc[i]-=md_dev.accBias[i];
			md_dev.gyr[i]-=md_dev.gyrBias[i];
			md_dev.mag[i]-=md_dev.magBias[i];
		}
		*md=md_dev;
		return 1;
	}
	return 0;
}

int AddMEMSData(char* isoTime,char* p,int* nbr,int GPS_Stat){
	int MEMS_Stat=0;
	MEMS_dev md;
	int i;
	*nbr=0;

	if (!isDataMEMSReady(&md)){
		#if USE_SERIAL==1
			Serial.println("Unable to read MEMS !");
		#endif
		MEMS_Stat=0;
	}else{
		if (!GPS_Stat){
			GetCurrentTime(isoTime);
			i=sprintf(p,"%s,,,,,,",isoTime);
			*nbr+=i;
			p+=i;
		}
		i=sprintf(p,",%f,%f,%f,%f,%f,%f,%f,%f,%f",\
			md.acc[0],md.acc[1],md.acc[2],\
			md.gyr[0],md.gyr[1],md.gyr[2],\
			md.mag[0],md.mag[1],md.mag[2]\
			);
		*nbr+=i;
		MEMS_Stat=1;
	}
	return MEMS_Stat;
}

