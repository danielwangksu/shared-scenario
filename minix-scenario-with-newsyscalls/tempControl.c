/**************************************************************
*	Temperature Control Program								  *
*	by Daniel Wang											  *
***************************************************************/

#define _MINIX_SYSTEM 1
#define OK 0

#include <minix/endpoint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <sys/epselect.h>
#include <lib.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <time.h>
#include "msg.h"

//defined for system calls used to trigger the GPIO ports
int system(const char *command);

// message data structure (OS define type)  
message m;
// endpoints for each process
int tempSen_ep, heatAct_ep, alarmAct_ep, web_ep;
// desired temperature setpoint
int setpoint = 29;
// timer threshold
time_t const time_threshold = 60; // 1 mins
// timer
time_t timer_count = 0;
// timer flag
int timer_flag = 0;
// current heater status
int heater_status = -1;
// current alarm status
int alarm_status = -1;
// current sensor temperature
int current_temp = -1;

int ep_array[5] = {0};

static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

// function for initialization
void initialize(){
	// setup tempControl endpoint
	tempSen_ep = getendpoint_name("tempSensor");
	heatAct_ep = getendpoint_name("heatActuator");
	alarmAct_ep = getendpoint_name("alarmActuator");
	web_ep = getendpoint_name("webInterface");
}

// send confirmation to webInterface process 
void sendComfirmToWeb(){
	memset(&m, 0, sizeof(m));
	m.m_type = WEB_CONFIRM;
	m.m_m1.m1i1 = 1;

	printf("TEMPCONTROL: sendCONFIRM: m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);
	ipc_send(web_ep, &m);
}

// receive web interface's children process endpoints
int receive_ep_Web(){
	int status, r;
	r = ipc_receive(web_ep, &m, &status);
	if(m.m_type == WEB_EP_UPDATE){
		ep_array[0] = m.m_m7.m7i1;
		ep_array[1] = m.m_m7.m7i2;
		ep_array[2] = m.m_m7.m7i3;
		ep_array[3] = m.m_m7.m7i4;
		ep_array[4] = m.m_m7.m7i5;
	}
	printf("TEMPCONTROL: receiveEPUpdate: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m7.m7i1);
	sendComfirmToWeb();
}

// receive sensor data from tempSensor process
int receiveSensorData(){
	int status, r;
	r = ipc_receive(tempSen_ep, &m, &status);
	printf("TEMPCONTROL: receiveSENSOR: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
	return r;
}

// control logic (0 within range, 1 too hot, -1 too cold)
int handleCommand(){
	int data = 0, delta = 0;
	int threshold = 1;

	if(m.m_type == SENSOR_UPDATE)
		data = m.m_m1.m1i1;

	current_temp = data;

	memset(&m, 0, sizeof(m));

/***** The RGB LED works in the opposite way the lights glows 
	when the respective wires are grounded but as we are 
	connecting to the GPIO pins so when the pins are turned OFF
	the LED glows and when its ON the LED remains in off state 
	GPIO1 - Fan
	GPIO2 - Red Light in the RGB LED
	GPIO3 - Green Light on the RGB LED
	GPIO4 - Blue Light on the RGB LED
*****/
 
	if(data > setpoint){
		// too hot
		delta = (data - setpoint);				
		if(delta > threshold){
		/* The following system calls are used to turn on and off the LED and the Fan
			system("cat /gpio/GPIO1On");
                	system("cat /gpio/GPIO2Off");
                	system("cat /gpio/GPIO3On");
                	system("cat /gpio/GPIO4On");*/
                	//delta = (data - setpoint);
			return 1;}
	}else{
		// too cold
		delta = (data - setpoint);
		//if(delta < threshold)
		if(data < (setpoint - threshold)){
		/* The following system calls are used to turn on and off the LED and the Fan			
			system("cat /gpio/GPIO1Off");
                	system("cat /gpio/GPIO2On");
                	system("cat /gpio/GPIO3On");
                	system("cat /gpio/GPIO4Off");*/
                	//delta = (data - setpoint);
			return -1;}
	}
	/* The following system calls are used to turn on and off the LED and the Fan	
	system("cat /gpio/GPIO1Off");
	system("cat /gpio/GPIO2On");
	system("cat /gpio/GPIO3Off");
	system("cat /gpio/GPIO4On");*/
	return 0;
}

void start_timer(){
	timer_count = time(NULL);
	timer_flag = 1;
}

void stop_timer(){
	timer_count = 0;
	timer_flag = 1;
}

int check_timer(){
	time_t timer_now = 0;
	time_t timer_elipsed = 0;
	
	timer_now = time(NULL);
	timer_elipsed = timer_now - timer_count;
	if(timer_elipsed > time_threshold)
		return 1;
	return 0;
}

// message alarmActuator process, 0 turn alarm on, 1 turn alarm off;
void controlAlarm(int status){
	memset(&m, 0, sizeof(m));
	m.m_type = ALARM_COMMAND;
	m.m_m1.m1i1 = status;

	printf("TEMPCONTROL: sendAlarm: m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);

	ipc_sendrec(alarmAct_ep, &m);
	if(m.m_type == ALARM_CONFIRM)
		alarm_status = m.m_m1.m1i1;
}

// message heatActuator process, 0 turn alarm on, 1 turn alarm off;
void controlHeater(int status){
	memset(&m, 0, sizeof(m));
	m.m_type = HEATER_COMMAND;
	m.m_m1.m1i1 = status;

	printf("TEMPCONTROL: sendHeater: m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);

	ipc_sendrec(heatAct_ep, &m);
	if(m.m_type == HEATER_CONFIRM)
		heater_status = m.m_m1.m1i1;
}

// check if there is pending message from webInterface
int tryReceiveFromWeb(){
	struct ep_poll check_ep;
	int status, r = -1;

	memset(&check_ep, 0, sizeof check_ep);
	check_ep.num = 6;
	check_ep.ep[0] = web_ep;
	check_ep.ep[1] = ep_array[0];
	check_ep.ep[2] = ep_array[1];
	check_ep.ep[3] = ep_array[2];
	check_ep.ep[4] = ep_array[3];
	check_ep.ep[5] = ep_array[4];

	ep_poll(&check_ep);
	if(check_ep.ready[0]){
		r = ipc_receive(web_ep, &m, &status);
		printf("TEMPCONTROL: receiveWEB: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
	}
	if(check_ep.ready[1]){
		r = ipc_receive(ep_array[0], &m, &status);
		printf("TEMPCONTROL: receiveWEB: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
	}
	if(check_ep.ready[2]){
		r = ipc_receive(ep_array[1], &m, &status);
		printf("TEMPCONTROL: receiveWEB: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
	}
	if(check_ep.ready[3]){
		r = ipc_receive(ep_array[2], &m, &status);
		printf("TEMPCONTROL: receiveWEB: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
	}
	if(check_ep.ready[4]){
		r = ipc_receive(ep_array[3], &m, &status);
		printf("TEMPCONTROL: receiveWEB: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
	}
	if(check_ep.ready[5]){
		r = ipc_receive(ep_array[4], &m, &status);
		printf("TEMPCONTROL: receiveWEB: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
	}

 	return r;
}

// update setpoint to the new value with sanity check
void updateSetpoint(){
	int new_setpoint, r = -1;

	if(m.m_type == SETPOINT_UPDATE){
		new_setpoint = m.m_m1.m1i1;
		if(new_setpoint > 60 && new_setpoint < 90){
			setpoint = new_setpoint;
			printf("tempControl: new setpoint is %d\n", setpoint);
			r = 0;
		}
	}

	memset(&m, 0, sizeof(m));
	m.m_type = WEB_CONFIRM;
	m.m_m1.m1i1 = r;
	ipc_sendnb(web_ep, &m);
}

//update log file in JSON format
void logFile(){
	char time_string[500];

	time_t timestamp_json = time(NULL);
	struct tm *time_p = localtime(&timestamp_json);

	strftime(time_string, 500, "%r,%F", time_p);

	FILE *fp = fopen("load.txt", "w");
	if(fp == NULL){
		printf("Error opening file!\n");
		exit(1);
	}
	fprintf(fp, "{\n\"Current Temperature\" : \"%d\", \"Desired Temperature\" : \"%d\", \"Fan status\" : \"%d\", \"Alarm status\" : \"%d\", \"Time\" : \"%s\"\n}", current_temp, setpoint, heater_status, alarm_status, time_string);
	fclose(fp);

}

/**************************************************************
*	main function							 				  *
***************************************************************/
void main(int argc, char **argv){
	int r, status;
	int control_flag = 0;

	initialize();
	printf("CONTROL: tempSen_ep: %d, heatAct_ep: %d, alarmAct_ep: %d, web_ep: %d\n", tempSen_ep, heatAct_ep, alarmAct_ep, web_ep);

	r = receive_ep_Web();
	if(r != OK){
		bail("receive_ep_Web()");
	}

	while(1){
		r = receiveSensorData();
		if(r != OK){
			continue;
		}

		control_flag = handleCommand();

		if(control_flag  == 0){
			// temperature within desired range
			stop_timer();
			if(alarm_status == 1)
				controlAlarm(0);
			continue;
		}else if(control_flag == -1 || control_flag == 1){
			// too cold or too hot
			if(control_flag == -1)
				controlHeater(1);
			else
				controlHeater(0);

			if(timer_flag == 1 && check_timer()){
				controlAlarm(1);
			}
		}

		if(tryReceiveFromWeb() == OK){
			updateSetpoint();
		}

		logFile();
	}
	exit(1);

}
