/**************************************************************
*	Temperature Control Program								  *
*	by Daniel Wang											  *
***************************************************************/

#define _MINIX_SYSTEM 1
#define ON 1
#define OK 0
#define OFF 0

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

int keepRunning = 1;
// message data structure (OS define type)  
message m;
// endpoints for each process
int tempSen_ep, heatAct_ep, alarmAct_ep, web_ep;
// desired temperature setpoint
float setpoint = 26.0;
// variable for storing changed setpoint
float temp_point = 0.0;
// timer threshold
time_t const time_threshold = 20; // 20 secs
// timer
time_t timer_count = 0;
// timer flag
int timer_flag = 0;
// current heater status
int heater_status = 0;
// current alarm status
int alarm_status = 0;
// current sensor temperature
float current_temp = 0.0;
// temperature threshold
float threshold = 1.0;

int ep_array[5] = {0};

static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

// function to convert a 3 digit int to single decimal float value
float itof(int intval){
	float q = intval / 10;
	float r = intval % 10;
	return (q + (r * 0.1));
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
void sendComfirmToWeb(int ep){
	memset(&m, 0, sizeof(m));
	m.m_type = WEB_CONFIRM;
	m.m_m1.m1i1 = 1;

	printf("TEMPCONTROL: sendCONFIRM: m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);
	ipc_send(ep, &m);
}

// receive web interface's children process endpoints
int receiveEpWeb(){
	int status, r;
	r = ipc_receive(web_ep, &m, &status);
	if(m.m_type == WEB_EP_UPDATE){
		ep_array[0] = m.m_m7.m7i1;
		ep_array[1] = m.m_m7.m7i2;
		ep_array[2] = m.m_m7.m7i3;
		ep_array[3] = m.m_m7.m7i4;
		ep_array[4] = m.m_m7.m7i5;
	}
	printf("TEMPCONTROL: receiveEPUpdate: status: %d, m_type: %d, value: %d, %d, %d, %d, %d\n", status, m.m_type, m.m_m7.m7i1, m.m_m7.m7i2, m.m_m7.m7i3, m.m_m7.m7i4, m.m_m7.m7i5);
	sendComfirmToWeb(web_ep);
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
	int data = 0;
	float delta = 0;

	if(m.m_type == SENSOR_UPDATE)
		data = m.m_m1.m1i1;

	current_temp = itof(data);

	printf("TEMPCONTROL: current temp %f\n", current_temp);

	memset(&m, 0, sizeof(m));
 
	if(current_temp > setpoint){
		// too hot
		delta = (current_temp - setpoint);				
		if(delta > threshold){
			return 1;
		}
	}
	else{
		// too cold
		delta = (setpoint - current_temp);
		printf("TEMPCONTOL: delta = %f\n", delta);
		if(delta > threshold){
			return -1;
		}
	}
	return 0;
}

void start_timer(){
	timer_count = time(NULL);
	timer_flag = 1;
}

void stop_timer(){
	timer_count = 0;
	timer_flag = 0;
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

// message alarmActuator process, 1 turn alarm on, 0 turn alarm off;
void controlAlarm(int status){
	memset(&m, 0, sizeof(m));
	m.m_type = ALARM_COMMAND;
	m.m_m1.m1i1 = status;

	printf("TEMPCONTROL: sendAlarm: m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);

	ipc_sendrec(alarmAct_ep, &m);
	if(m.m_type == ALARM_CONFIRM)
		alarm_status = m.m_m1.m1i1;
}

// message heatActuator process, 1 turn Fan on, 0 turn Fan off;
void controlFan(int status){
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
		if(m.m_type == SETPOINT_UPDATE)
			temp_point = m.m_m1.m1i1;
		sendComfirmToWeb(web_ep);
	}
	if(check_ep.ready[1]){
		r = ipc_receive(ep_array[0], &m, &status);
		printf("TEMPCONTROL: receiveWEB: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
		if(m.m_type == SETPOINT_UPDATE)
			temp_point = m.m_m1.m1i1;
		sendComfirmToWeb(ep_array[0]);
	}
	if(check_ep.ready[2]){
		r = ipc_receive(ep_array[1], &m, &status);
		printf("TEMPCONTROL: receiveWEB: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
		if(m.m_type == SETPOINT_UPDATE)
			temp_point = m.m_m1.m1i1;
		sendComfirmToWeb(ep_array[1]);
	}
	if(check_ep.ready[3]){
		r = ipc_receive(ep_array[2], &m, &status);
		printf("TEMPCONTROL: receiveWEB: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
		if(m.m_type == SETPOINT_UPDATE)
			temp_point = m.m_m1.m1i1;
		sendComfirmToWeb(ep_array[2]);
	}
	if(check_ep.ready[4]){
		r = ipc_receive(ep_array[3], &m, &status);
		printf("TEMPCONTROL: receiveWEB: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
		if(m.m_type == SETPOINT_UPDATE)
			temp_point = m.m_m1.m1i1;
		sendComfirmToWeb(ep_array[3]);
	}
	if(check_ep.ready[5]){
		r = ipc_receive(ep_array[4], &m, &status);
		printf("TEMPCONTROL: receiveWEB: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
		if(m.m_type == SETPOINT_UPDATE)
			temp_point = m.m_m1.m1i1;
		sendComfirmToWeb(ep_array[4]);
	}

 	return r;
}

// update setpoint to the new value with sanity check
void updateSetpoint(){
	int r = -1;

	if(temp_point > 20 && temp_point < 40 ){
		setpoint = temp_point;
		printf("tempControl: new setpoint is %f\n", setpoint);
		r = 0;
	}
}

//update log file in JSON format
int logging(){
	char time_string[500];
	time_t timestamp_json = time(NULL);
	struct tm *time_p = localtime(&timestamp_json);

	strftime(time_string, 500, "%r,%F", time_p);

	FILE *fp = fopen("load.txt", "w");
	if(fp == NULL){
		return -1;
	}
	fprintf(fp, "{\n\"Current Temperature\" : \"%.1f\", \"Desired Temperature\" : \"%.1f\", \"Fan status\" : \"%s\", \"Alarm status\" : \"%s\", \"Time\" : \"%s\"\n}", current_temp, setpoint, (heater_status) ? "ON" : "OFF", (alarm_status) ? "ON" : "OFF", time_string);
	fclose(fp);
	return 0;
}

// interrupt handler
void intHandler(int dummy){
	system("cat /gpio/GPIO2Off");
	system("cat /gpio/GPIO3Off");
	system("cat /gpio/GPIO1Off");
	keepRunning = 0;
}



/**************************************************************
*	main function							 				  *
***************************************************************/
void main(int argc, char **argv){
	int r, status;
	int control_flag = 0;

	initialize();
	printf("CONTROL: tempSen_ep: %d, heatAct_ep: %d, alarmAct_ep: %d, web_ep: %d\n", tempSen_ep, heatAct_ep, alarmAct_ep, web_ep);

	r = receiveEpWeb();
	if(r != OK){
		bail("receiveEpWeb()");
	}

	signal(SIGINT, intHandler);

	while(keepRunning){
		r = receiveSensorData();
		if(r != OK){
			continue;
		}

		control_flag = handleCommand();

		printf("TEMPCONTROL: control_flag %d\n", control_flag);

		if(control_flag  == OK || control_flag == -1){
			// temperature within desired range
			stop_timer();
			if(alarm_status == ON)
				controlAlarm(OFF);
		}
		if(control_flag == -1 || control_flag == 1){
			// too cold (-1) or too hot (1)
			if(control_flag == -1)
				controlFan(OFF);
			if(control_flag == 1)
				controlFan(ON);

			if(timer_flag != 1 && control_flag == 1){
				start_timer();
			}
			else if(timer_flag == 1 && check_timer()){
				controlAlarm(ON);
			}
		}

		if(tryReceiveFromWeb() == OK){
			updateSetpoint();
		}

		logging();
	}
	exit(0);

}
