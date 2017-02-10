/**************************************************************
*	Heater Actuator Program								      *
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
#include <math.h>
#include "msg.h"

#define OK 0
#define ON 1
#define OFF 0

// message data structure (OS define type)  
message m;
// tempControl process endpoint (OS defined type) 
endpoint_t tempCnt_ep;
// alarm status (-1 uninitiated; 1 heater on; 0 heater off)
int heater_status = -1;

void setGPIOFanOn(void){
	system("cat /gpio/GPIO01On");
}

void setGPIOFanOff(void){
	system("cat /gpio/GPIO01Off");
}

// function for initialization
void initialize(){
	// setup tempControl endpoint
	tempCnt_ep = getendpoint_name("tempControl");
}

// receive command from tempControl endpoint
int receiveCommandFromTempControl(){
	int r, status;
	r = ipc_receive(tempCnt_ep, &m, &status);
	printf("HEATER: receiveTEMPCONTROL: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
	return r;
}

// turn fan ON
int turnFanOn(){
	heater_status = 1;
	setGPIOFanOn();
	printf("heatActuator: the heater is ON\n");
}

// turn fan OFF
int turnFanOff(){
	heater_status = 0;
	setGPIOFanOff();
	printf("heatActuator: the heater is OFF\n");
}

// control logic
void handleCommand(){
	int command;

	if(m.m_type == HEATER_COMMAND)
		command = m.m_m1.m1i1;

	switch(command){
		case 0:
			if(heater_status == -1 || heater_status == 1){
				turnFanOff();
			}
			break;
		case 1:
			if(heater_status == -1 || heater_status == 0){
				turnFanOn();
			}
			break;
		default:
			printf("heatActuator: unknown command\n");
	}
}

// send confirmation to tempControl process 
void sendComfirmToTempControl(){
	memset(&m, 0, sizeof(m));
	m.m_type = CONTROL_CONFIRM;
	m.m_m1.m1i1 = heater_status;

	printf("HEATER: sendCONFIRM: m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);
	ipc_send(tempCnt_ep, &m);
}

/**************************************************************
*	main function							 				  *
***************************************************************/
void main(int argc, char **argv){

	int r;

	int paststatus = -1;
	int currentstatus = -1;

	initialize();
	printf("HEATER: tempCnt_ep: %d\n", tempCnt_ep);

	while(1){
		r = receiveCommandFromTempControl();
		if(r != OK){
			continue;
		}

		handleCommand();
		sendComfirmToTempControl();
	}
	exit(1);
}