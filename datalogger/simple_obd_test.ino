#include "simple_obd_test.h"
#include "customized_SD.h"
#include "customized_DateAndTime.h"
#include "customized_MEMS.h"
#include "customized_OBD.h"
#include "customized_GPS.h"
FreematicsESP32 sys;

MEMS_I2C* mems=0;

//mapping each PID of OBD
ObdInfoPid PIDs[]={
	{"PID_SPEED",PID_SPEED,1},
	{"PID_AMBIENT_TEMP",PID_AMBIENT_TEMP,0},
	{"PID_RPM",PID_RPM,1},
	{"PID_ENGINE_REF_TORQUE",PID_ENGINE_REF_TORQUE,1},
	{"PID_RUNTIME",PID_RUNTIME,0},
	{"PID_BAROMETRIC",PID_BAROMETRIC,0},
	{"PID_FUEL_LEVEL",PID_FUEL_LEVEL,0},
	{"PID_HYBRID_BATTERY_PERCENTAGE",PID_HYBRID_BATTERY_PERCENTAGE,0}
};
//length of PIDs table
#define nbr_elem sizeof(PIDs)/sizeof(ObdInfoPid)

//queue where the data will be send to the daemon task
static QueueHandle_t xQueue_print=NULL;

//clear a char buffer with certain length
void clr_buff(char*,uint32_t);

//appending char string to CSV file
void print_data(char*,uint32_t);

//task writing OBD data which frequency is maximal  
void write_task_1(void*);

//task collecting GPS + MEMS data
void write_task_2(void*);

//task writing OBD data which frequency is 1 sample per minute
void write_task_3(void*);

//daemon task
void vTask_Transmit(void *);

//message structure
typedef struct{
	char* data;
	int len;
	SemaphoreHandle_t completion;
}Irp;


void setup(){
	pinMode(PIN_LED, OUTPUT);
	digitalWrite(PIN_LED, HIGH);
	delay(1000);
	digitalWrite(PIN_LED, LOW);
	Serial.begin(115200);

	//initializations
	while(!sys.begin());
	
	//initialize SD_Card
	init_SD_Card(PIDs,nbr_elem);

	//initialize GPS device
	init_GPS_dev();
	
	//initialize MEMS device
	init_MEMS_dev();

	#if USE_OBD==1
		init_OBD_dev();
	#endif

	// turn on buzzer at 2000Hz frequency 
	sys.buzzer(2000);
	delay(300);
	// turn off buzzer
	sys.buzzer(0);

	//creating queue with a size (15*sizeof(Irp*))
	xQueue_print = xQueueCreate(15,sizeof(Irp*));
	
	#if USE_OBD==1
		//f=fmax
		xTaskCreate(write_task_1,"write_task_1",STACK_SIZE*3,(void*) 0,tskIDLE_PRIORITY+1,NULL);
	#endif

	//f=10Hz
	xTaskCreate(write_task_2,"write_task_2",STACK_SIZE*5,(void*) 0,tskIDLE_PRIORITY+4,NULL);

	#if USE_OBD==1
		//f=1/60 Hz
		xTaskCreate(write_task_3,"write_task_3",STACK_SIZE*3,(void*) 0,tskIDLE_PRIORITY+1,NULL);
	#endif
	
	//create daemon task
	xTaskCreate(vTask_Transmit,"vTask_Transmit",STACK_SIZE*40,(void*) 0,tskIDLE_PRIORITY+3,NULL);
}


void loop(){vTaskDelay(3000);}

void write_task_1(void* parameter)
{
	int sleep_tme;
	while(1){
		sleep_tme=OBD_looping(PIDs,nbr_elem,1);
		taskYIELD();
		ErrorOBDCheck();
		vTaskDelay(pdMS_TO_TICKS(sleep_tme));
	}
	vTaskDelete(NULL);
}
void write_task_2(void* parameter)
{
	char buff[buff_SIZE];
	uint32_t nbr;
	char isoTime[time_size]={0};
	int i;
	int GPS_Stat=0;
	int MEMS_Stat=0;
	char* p;
	
	while(1){
		clr_buff(isoTime,time_size);
		clr_buff(buff,buff_SIZE);
		nbr=0;
		p=buff;
		GPS_Stat	=AddGPSData(isoTime,p,&i);
		p+=i;
		nbr+=i;
		MEMS_Stat	=AddMEMSData(isoTime,p, &i,GPS_Stat);
		p+=i;
		nbr+=i;
		if((MEMS_Stat)||(GPS_Stat)){
			sprintf(p,"\n");
			print_data(buff,nbr+1);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	vTaskDelete(NULL);
}
void write_task_3(void* parameter){
	int sleep_tme;
	while(1){
		sleep_tme=OBD_looping(PIDs,nbr_elem,0);
		taskYIELD();
		ErrorOBDCheck();
		vTaskDelay(pdMS_TO_TICKS(sleep_tme));
	}
	vTaskDelete(NULL);
}

void vTask_Transmit(void *pvParameters){
	Irp* ReceiveItem;
	char buff[buff_SIZE];
	char isoTime[time_size];
	char* p;
	int i=0;
	int nbr=0;
	for(;;){
		nbr=0;
		clr_buff(buff,buff_SIZE);
		clr_buff(isoTime,time_size);
		//receiving ReceiveItem via xQueue_print
			xQueueReceive(xQueue_print,&ReceiveItem,portMAX_DELAY);
			//GetCurrentTime(isoTime);
			p=buff;
			//p+=sprintf(p,"%s,",isoTime);
			for(i=0;i<ReceiveItem->len;i++)
				*p++=ReceiveItem->data[i];
		//sending  data via USART
		#if USE_SERIAL==1
			Serial.println(buff);
		#endif
		//writing into SD card
		appendFile(SD, getFileName(),buff);
		//free the semaphore
			xSemaphoreGive(ReceiveItem->completion);			
		taskYIELD();
	}

}

void print_data(char* data, uint32_t len){
	//dynamic allocation of SendItem and filling its fields
		Irp* SendItem=(Irp*)pvPortMalloc(sizeof(Irp));
		char* data_aux =(char*) pvPortMalloc(len*sizeof(char));
		for (int i=0;i<len;i++){
			data_aux[i]=data[i];
		}
		SendItem->data=data_aux;
		SendItem->len=len;
		SendItem->completion=xSemaphoreCreateBinary();
	//sending the adress of the pointer SendItem via xQueue_print
		xQueueSend(xQueue_print,&SendItem,portMAX_DELAY);
	//passive wait for the end of transmission
		xSemaphoreTake(SendItem->completion, portMAX_DELAY);
	//free the memory pointed by SendItem
		vSemaphoreDelete(SendItem->completion); 
		vPortFree(SendItem->data);
		vPortFree(SendItem);
}


void clr_buff(char* buf,uint32_t len){
	for(int i=0;i<len;i++)
		buf[i]='\000';
}
