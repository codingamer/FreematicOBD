#include "simple_obd_test.h"
#include "customized_GPS.h"
#include "customized_OBD.h"
#include "customized_DateAndTime.h"

extern void clr_buff(char*,uint32_t);
extern FreematicsESP32 sys;
GPS_DATA* gd=0;

void init_GPS_dev(void){
	if (sys.gpsBegin(GPS_SERIAL_BAUDRATE)) {
		#if USE_SERIAL==1
			Serial.println("GPS is initialized successfully");
		#endif
	}else{
		#if USE_SERIAL==1
			Serial.println("Error occured while initializing GPS");
		#endif
		while(1);
	}
	pinMode(26, OUTPUT);
	pinMode(34, INPUT);
	pinMode(PIN_GPS_POWER, OUTPUT);
	digitalWrite(PIN_GPS_POWER, HIGH);
	int cnt=0;
	while(!isDataGPSReady(10000)){
		delay(100);
		cnt++;
		
		if(cnt==5){
			esp_sleep_enable_timer_wakeup(1 * uS_TO_S_FACTOR);
			esp_deep_sleep_start();
		}
	}
	UpdateTime();
	SetSysInstance(&sys);
}

int isDataGPSReady(int wait_MS){
	for (uint32_t t=millis();millis()-t<wait_MS;){
		digitalWrite(26, digitalRead(34));
		if (sys.gpsGetData(&gd))
			return 1;
	}
	return 0;
}

int FilterGpsData(char* isoTime, double* res){
	
	static char lastTimeStamp2[time_size]={0};
	static float meanGPSCoord[NBR_GPS_POINTS][4];
	float AuxGPSCoord[4];
	int j;
	int nbr_pts=NBR_GPS_POINTS;
	static int cnt=0;
	int slope_flag=0;
	int shiftTime =0;
	
	if (lastTimeStamp2[0]){
		shiftTime=GetShiftTime(lastTimeStamp2,isoTime);
		if (shiftTime>5){
			meanGPSCoord[0][0]=gd->alt;
			meanGPSCoord[0][1]=gd->lng;
			meanGPSCoord[0][2]=gd->lat;
			meanGPSCoord[0][3]=gd->speed*1.852f;
			AuxGPSCoord[0]=gd->alt;
			AuxGPSCoord[1]=gd->lng;
			AuxGPSCoord[2]=gd->lat;
			AuxGPSCoord[3]=gd->speed*1.852f;
			cnt=0;
		}else{
			if (cnt+1<NBR_GPS_POINTS){
				nbr_pts=cnt;
			}

			AuxGPSCoord[0]=gd->alt;
			AuxGPSCoord[1]=gd->lng;
			AuxGPSCoord[2]=gd->lat;
			AuxGPSCoord[3]=gd->speed*1.852f;
			
			for(j=0;j<nbr_pts;j++){
				AuxGPSCoord[0]+=meanGPSCoord[j][0];
				AuxGPSCoord[1]+=meanGPSCoord[j][1];
				AuxGPSCoord[2]+=meanGPSCoord[j][2];
				AuxGPSCoord[3]+=meanGPSCoord[j][3];
			}
			AuxGPSCoord[0]/=(nbr_pts+1);
			AuxGPSCoord[1]/=(nbr_pts+1);
			AuxGPSCoord[2]/=(nbr_pts+1);
			AuxGPSCoord[3]/=(nbr_pts+1);
			slope_flag=1;
		}
	}else{
		meanGPSCoord[0][0]=gd->alt;
		meanGPSCoord[0][1]=gd->lng;
		meanGPSCoord[0][2]=gd->lat;
		meanGPSCoord[0][3]=gd->speed*1.852f;
		AuxGPSCoord[0]=gd->alt;
		AuxGPSCoord[1]=gd->lng;
		AuxGPSCoord[2]=gd->lat;
		AuxGPSCoord[3]=gd->speed*1.852f;
		cnt=0;
	}

	int n=(cnt+1)%NBR_GPS_POINTS;
	int prev= !(n) ? NBR_GPS_POINTS-1 : n-1;
	res[0]=AuxGPSCoord[0];
	res[1]=AuxGPSCoord[1];
	res[2]=AuxGPSCoord[2];
	res[4]=AuxGPSCoord[3];
	
	if (slope_flag){
		double dist 		= (double) TinyGPS::distance_between((float) meanGPSCoord[prev][2],(float) meanGPSCoord[prev][1],(float) res[2],(float) res[1]);
		#if USE_OBD==0
			double dist_max		= gd->speed*shiftTime/3.6*1.25;
		#elif
			double dist_max		= getOBDSpeed()*shiftTime/3.6*1.2;
		#endif
		if ((dist>dist_max) || (dist>(85*shiftTime)))
			return -1;
		res[3]=(res[0]-meanGPSCoord[prev][0])/(dist+10)*100;
		meanGPSCoord[n][0]=res[0];
		meanGPSCoord[n][1]=res[1];
		meanGPSCoord[n][2]=res[2];
		meanGPSCoord[n][3]=res[4];
		cnt++;

	}
	for (j=0;j<time_size;j++){
		lastTimeStamp2[j]=isoTime[j];
	}
	#if USE_OBD==0
		isVehicleStopped(isoTime,res[4]);
	#endif
	return slope_flag;
}

void isVehicleStopped(char* isoTime,double val){
	static char lastTimeStamp[time_size]={0};
	static float meanSpeed=0;
	int j;

	if (lastTimeStamp[0]){
		if (GetShiftTime(lastTimeStamp,isoTime)>NBR_GPS_POINTS){
			if ((meanSpeed<2)&&(abs(val-meanSpeed)<0.5)){
				esp_sleep_enable_timer_wakeup(5 * uS_TO_S_FACTOR);
				esp_deep_sleep_start();
			}else{
				meanSpeed=val;
				for (j=0;j<time_size;j++){
					lastTimeStamp[j]=isoTime[j];
				}
			}
		}else{
			meanSpeed+=val;
			meanSpeed/=2;
		}
	}else{
		meanSpeed=val;
		for (j=0;j<time_size;j++){
			lastTimeStamp[j]=isoTime[j];
		}
	}
}

int AddGPSData(char* isoTime,char* p, int* nbr){
	
	int GPS_Stat=0;
	char isoGPSTime[time_size]={0};
	int flg_filter=0;
	double res[5];
	int i=0;
	*nbr=0;
	clr_buff(isoGPSTime,time_size);

	if (!isDataGPSReady(1200)) {
		#if USE_SERIAL==1
			Serial.println("GPS signal lost !");
		#endif
		GPS_Stat=0;
	}else{
		GetCurrentTime(isoTime);
		GetCurrentGPSTime(isoGPSTime);
		flg_filter=FilterGpsData(isoTime, res);
		if (flg_filter!=-1){
			if (flg_filter==1){
				i=sprintf(p,"%s,%s,%lf,%lf,%lf,%lf,%lf",isoTime,isoGPSTime,res[0],res[1],res[2],res[3],res[4]);
			}else if (flg_filter==0) {
				i=sprintf(p,"%s,%s,%lf,%lf,%lf,,%lf",isoTime,isoGPSTime,res[0],res[1],res[2],res[4]);
			}
			*nbr+=i;
			GPS_Stat=1;
		}else{
			GPS_Stat=0;
		}
	}
	return GPS_Stat;

}
