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

#define MAX_BUF 255
// message data structure (OS define type)  
message m;
// tempControl process endpoint (OS defined type) 
endpoint_t tempCnt_ep;
int i = 0;

/************** Variables related to BMP Sensor - START ******************/
//the name of the character specific device file
char filename[] = "/dev/bmp085b3s77";

//character buffer to read the data from the file
char readbuf[MAX_BUF];

//File pointer
FILE *fp;
/************** Variables related to BMP Sensor - END ********************/

static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

// function for initialization
void initialize(){
	memset(&m, 0, sizeof(m));
	// setup tempControl endpoint
	tempCnt_ep = getendpoint_name("tempControl");
}

// simulate the process of retrieving data from sensor
void retrieveSensorData(){
	int status;
	int temp_data;
	// fake sensor data
// const int sensordata[] = {220, 210, 240, 250, 240, 200, 220, 210, 240, 250, 240, 200, 280, 270, 260, 250, 220, 210, 240, 250, 240, 200, 220, 210, 240, 250, 240, 200, 280, 270, 260, 250, -1};
	/******** start the data retrieval process from the sensor ********/	
	status = start();
		if(status != 0)
			bail("start()");
	
	// simulate periodic arriving of sensor data
	sleep(4);

	// if(sensordata[i] == -1)
	// 	exit(1);

	/*send BMP sensor data by calling the readBMPData() function*/	
	temp_data = readBMPData();
	m.m_type = SENSOR_UPDATE;
	m.m_m1.m1i1 = temp_data;

	// m.m_type = SENSOR_UPDATE;
	// m.m_m1.m1i1 = sensordata[i];

	i++;
	/*as the data for the BMP sensor is read from a character device file we rewind the file pointer to read the updated/changed values*/
	//rewind(fp);
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

/******** this function opens the character device file ********/
int openFile(){
	fp = fopen(filename, "r");
	if(fp == NULL){
		printf("\nError : couldn't open the file %s\n", filename);
		return -1;
	}
	// printf("File opened successfully\n");
	return 0;
}

/********this function brings the bmp085 service up (this is a ine time process) ********/
int serviceup(){
	int status;
	status = system("/bin/service up /service/bmp085 -label bmp085.3.77 -dev /dev/bmp085b3s77 -args 'bus=3 address=0x77'");
	return status;
}

// function for start service
int start(){
	int status;
	int fileio;
	//open the character specific device file
	fileio = openFile();
	if(fileio == -1){
		printf("FILE opening failed. Trying to bring the service up\n");
		status = serviceup();
		if(status != -1){
			fileio = openFile();
			if(fileio != 0)
				return fileio;
		}
		else
			return status;
	}
	return 0;
}

int ftoi(float value){
	int data;
	data = (int)(value * 10);
	return data;
}

/******** this function reads the data from the buffer and parses temperature into an integer value and returns to the calling function ********/
int readBMPData(){
	float temp_data;

	// fscanf(fp, "%s", readbuf);
	// if((fgets(readbuf, MAX_BUF, (FILE*)fp)) == NULL){
	// 	printf("cannot read device file\n");
	// }
	fscanf(fp, "%s %f", readbuf, &temp_data);
	fclose(fp);
	// printf("!!!%s!!!\n", readbuf);
	// temp_data = atof(readbuf);

	printf("\n*******************\nTemperature Value : %f\n", temp_data);

	// fscanf(fp, "%s", readbuf);
	// fgets(readbuf, 255, (FILE*)fp);
	// pressureValue = atof(readbuf);
	// printf("Pressure Value : %.2f\n",pressureValue);
	return ftoi(temp_data);
}
