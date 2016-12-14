/**************************************************************
*	Alarm Actuator Program								  	  *
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

#define OK 0
// message data structure (OS define type)  
message m;
// tempControl process endpoint (OS defined type) 
endpoint_t tempCnt_ep;
// alarm status (-1 uninitiated; 1 alarm on; 0 alarm off)
int alarm_status = -1;

// function for initialization
void initialize(){
	// setup tempControl endpoint
	tempCnt_ep = getendpoint_name("tempControl");
}

// receive command from tempControl endpoint
int receiveCommandFromTempControl(){
	int status;
	ipc_receive(tempCnt_ep, &m, &status);
	return status;
}

// turn alarm ON
int turnAlarmOn(){
	alarm_status = 1;
	printf("alarmActuator: the alarm is ON\n");
}

// turn alarm OFF
int turnAlarmOff(){
	alarm_status = 0;
	printf("alarmActuator: the alarm is OFF\n");
}

// control logic
void handleCommand(){
	int command;

	if(m.m_type == ALARM_COMMAND)
		command = m.m_m1.m1i1;

	memset(&m, 0, sizeof(m));

	switch(command){
		case 0:
			if(alarm_status == -1 || alarm_status == 1){
				turnAlarmOn();
			}
			break;
		case 1:
			if(alarm_status == -1 || alarm_status == 0){
				turnAlarmOff();
			}
			break;
		default:
			printf("alarmActuator: unknown command\n");
	}
}

// send confirmation to tempControl process 
void sendComfirmToTempControl(){
	m.m_type = ALARM_CONFIRM;
	m.m_m1.m1i1 = alarm_status;
	ipc_send(tempCnt_ep, &m);
}

/**************************************************************
*	main function							 				  *
***************************************************************/
void main(int argc, char **argv){

	int status;

	int paststatus = -1;
	int currentstatus = -1;

	initialize();
	printf("ALARM: tempCnt_ep: %d\n", tempCnt_ep);

	while(1){
		status = receiveCommandFromTempControl();
		if(status != OK){
			continue;
		}

		handleCommand();
		sendComfirmToTempControl();
	}
	exit(1);
}