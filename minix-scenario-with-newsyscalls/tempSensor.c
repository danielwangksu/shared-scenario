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

/************** Variables related to BMP Sensor - START ******************/
//defines the status of the file open/close operation
int fileio;
int filestat;
//the name of the character specific device file
char filename[] = "/dev/bmp085b3s77";

//the operation mode of the above file
char filemode[] = "r";

//character buffer to read the data from the file
char readbuf[255];

//File pointer
FILE *fp;
FILE *fp1;

//variable to store the temperature value
int temperatureValue;

//variable to store the status of fan(0 - Off, 1 - On)
int fanStatus = 0;

//variable to store the pressure value
float pressureValue;
int i = 0;
int serv_stat;

/************** Variables related to BMP Sensor - END ********************/


// function for initialization
void initialize(){
	memset(&m, 0, sizeof(m));
	// setup tempControl endpoint
	tempCnt_ep = getendpoint_name("tempControl");
}

// simulate the process of retrieving data from sensor
void retrieveSensorData(){
	// fake sensor data
const int sensordata[] = {70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, -1};
	/******** start the data retrieval process from the sensor ********/	
	/*filestat = start();
        if(filestat != 0)
                exit(1);*/
	
	// simulate periodic arriving of sensor data
	sleep(3);

	if(sensordata[i] == -1)
		exit(1);

	m.m_type = SENSOR_UPDATE;
	m.m_m1.m1i1 = sensordata[i];
	/*send BMP sensor data by calling the bmpdata function*/	
	//m.m_m1.m1i1 = bmpdata();
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

/*int start()
{
        //open the character specific device file
        fileio = openFile();

        //check if the file is opened successfully
        if(fileio != 0){
                if(fileio == 1){
                        printf("\n FILE opening failed. Trying to bring the service up");
                        serv_stat = serviceup();
                        if(serv_stat == 0){
                                //return 0;
                                fileio = openFile();
                                if(fileio != 0)
                                        return fileio;
                                else
                                        return fileio;
                        }
                        else
                                return serv_stat;
                }
                else{
                        printf("\nProgram terminated with an error");
                        return 0;
                }
        }
        printf("\nContinue Process\n");
       // fileio = closeFile();
       // if(fileio != 0){
       //         printf("File close failed\n");
       //         return 0;
       // }
       // printf("\nFile closed successfully\n");
        return 0;
}*/

/********this function brings the bmp085 service up (this is a ine time process) ********/
/*int serviceup(){
        serv_stat = system("/bin/service up /service/bmp085 -label bmp085.3.77 dev /dev/bmp085b3s77 -args 'bus=3 address=0x77'");
        return serv_stat;
}*/

/******** this function opens the character device file ********/
/*int openFile(){
        fp = fopen(filename,filemode);
        if(fp == NULL){
                printf("\nError : couldn't open the file %s",filename);
                return 1;
        }
        else{
                printf("\nFile opened successfully");
                return 0;
        }
}*/

/******** this function reads the data from the buffer and parses temperature into an integer value and returns to the calling function initialise() ********/
/*int bmpdata(){
        fscanf(fp, "%s", readbuf);
        fgets(readbuf, 255, (FILE*)fp);
        temperatureValue = atoi(readbuf);
        printf("\n*******************\nTemperature Value : %d\n",temperatureValue);
        fscanf(fp, "%s", readbuf);
        fgets(readbuf, 255, (FILE*)fp);
        pressureValue = atof(readbuf);
        printf("Pressure Value : %.2f\n",pressureValue);
        return temperatureValue;
}*/
