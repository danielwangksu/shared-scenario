/**************************************************************
*	Temperature Sensor Program								  *
*	by Daniel Wang											  *
***************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <lib.h>
#include <stdlib.h>
#include <strings.h>
#include "msg.h"

// message data structure (OS define type)  
message m;
// tempControl process endpoint (OS defined type) 
endpoint_t tempCnt_ep;
int i = 0;

// function for initialization
void initialize(){
	// setup tempControl endpoint
	tempCnt_ep = getendpoint_name("tempControl");
}

// simulate the process of retrieving data from sensor
void retrieveSensorData(){
	// fake sensor data
const int sensordata[] =  {70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, -1};
	
	// simulate periodic arriving of sensor data
	sleep(3);
	if(sensordata[i] == -1)
		exit(1);
	m.m_type = SENSOR_UPDATE;
	m.m_m1.m1i1 = sensordata[i];
	i++;
}

int sendDataToTempControl(){
	printf("SENSOR: NBsend currentTemp: m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);
	// uses non-blocking send
	ipc_sendnb(tempCnt_ep, &m);
}

/**************************************************************
*	main function							 				  *
***************************************************************/
void main(int argc, char **argv){
	char ch;
	int i = 0;

	initialize();
	printf("SENSOR: tempCnt_ep: %d\n", tempCnt_ep);

	while(1){
		retrieveSensorData();

		sendDataToTempControl();
	}
	exit(1);

}