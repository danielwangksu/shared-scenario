/**************************************************************
*	Temperature Sensor Program								  *
*	by Daniel Wang											  *
***************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include "msg.h"
#define MAX_BUF 64

/*Sid : variables for the BMP180 sensor read process - START*/
FILE *fp;
int tempval;
char path[MAX_BUF];
char buf[MAX_BUF];
/*Sid : variables for the BMP180 sensor read process - END*/

const int sensordata[] =  {70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, -1};

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

void main(int argc, char **argv){
	mqd_t mqd_sc;
	int piro = 0;
	int status;
	int i = 0;
	int flag = O_WRONLY;

	//Sid : call the initSensor function	
	if(initSensor() != 0)
		exit(1);
	
	//Sid : call the openDevicefile function	
	if(openDevicefile() != 0)
		exit(1);
	mqd_sc = mq_open("/sen-cnt", flag);
	if(mqd_sc == (mqd_t) -1 ){
		bail("ts: mq_open(/sen-cnt)");
	}

	printf("tempSensor is loaded\n");

	while(1){
		if(sensordata[i] == -1){
			break;
		}

		//Sid : rewind the device file to read the updated data		
		rewind(fp);		

		sleep(2);
		
		//Sid : call the readDevicefile to read the data and convert to an int of 3 digits the last digit is after the decimal//
		printf("tempSensor: sending %d\n", readDevicefile());

		Msg message = {SENSOR_UPDATE, sensordata[i]};
		status = mq_send(mqd_sc, (const char *) &message, sizeof(message), 0);
		i++;
	}
	status = mq_unlink("/sen-cnt");
	if(status != 0){
		exit(-1);
	}
	exit(0);
}

/*Sid : function to initialize the BMP180 sensor - START*/
int initSensor(){
       snprintf(path,sizeof path, "/sys/class/i2c-adapter/i2c-1/new_device");
	if((fp = fopen(path,"w")) == NULL){
		printf("cannot initialize device file");
		return 1;
	        }
       rewind(fp);
       fprintf(fp, "bmp085 0x77\n");
       fflush(fp);
       fclose(fp);
       return 0;
}
/*Sid : function to initialize the BMP180 sensor - END*/

/*Sid : function to open the BMP180 sensor device file- START*/
int openDevicefile(){
	snprintf(path, sizeof path, "/sys/bus/i2c/drivers/bmp085/1-0077/temp0_input");
	if((fp = fopen(path, "r")) == NULL){
            printf("cannot open device file");
            return 1;
            }
	return 0;
}
/*Sid : function to open the BMP180 sensor device file- END*/

/*Sid : function to read the BMP180 sensor device file- START*/
int readDevicefile(){
        if((fgets(buf, MAX_BUF, fp)) == NULL){
            printf("cannot read device file");
	    fflush(fp);
	    fclose(fp);
           }
	tempval = atoi(buf);
        printf("Current Temperature: %d\n",tempval);
	return tempval;
}
/*Sid : function to read the BMP180 sensor device file- END*/
