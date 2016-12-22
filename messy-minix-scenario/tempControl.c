/**************************************************************
*	Temperature Control Program								  *
*	by Daniel Wang											  *
***************************************************************/
/*
TODO: implement PID control loop
	Control variable: the output of the controller that we get to adjust (Fan speed?)
	Process variable: the measured value in the system (Temperature/sensor data)
	Error: the difference between the process variable and the set point (delta)
	P = Error * Kp [Kp is the proportional coefficient]
	I = accumulation_of_error * Ki [accumulation_of_error += error * delta_time]
	D = derivative_of_error * Kd [derivative_of_error = (error - last_error) / delta_time]
	output = P + I + D
*/

#define _MINIX_SYSTEM 1
#define OK 0

#include <minix/endpoint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <lib.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <time.h>

int const threshold = 3;
time_t const time_threhold = 60; // 1 mins
// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

void main(int argc, char **argv){
	int tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid;
	int tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, web_ep;
	int r, status;

	time_t timestamp_s = 0;
	time_t timestamp_e = 0;
	time_t time_elipsed = 0;
	int flag = 0;

	int delta = 0;

	int setpoint = 75;
	int newsetpoint = 0;

	message m_sensor;
	message m_heater;
	message m_alarm;

	int sensor = -1;
	int heater = -1;
	int alarm = -1;

	tempSen_pid = atoi(argv[1]);
	tempCnt_pid = atoi(argv[2]);
	heatAct_pid = atoi(argv[3]);
	alarmAct_pid = atoi(argv[4]);
	web_pid = atoi(argv[5]);
	tempSen_ep = atoi(argv[6]);
	tempCnt_ep = atoi(argv[7]);
	heatAct_ep = atoi(argv[8]);
	alarmAct_ep = atoi(argv[9]);
	web_ep = atoi(argv[10]);

	printf("tempControl: tempSen_pid=%d, tempCnt_pid=%d, heatAct_pid=%d, alarmAct_pid=%d,web_pid=%d, tempSen_ep=%d, tempCnt_ep=%d, heatAct_ep=%d, alarmAct_ep=%d, web_ep=%d\n", tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid, tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, web_ep);

	while(1){
		r = ipc_receive(ANY, &m_sensor, &status);
		if(r != OK){
			continue;
		}

		if(m_sensor.m_source == tempSen_ep){

			printf("tempControl: the current temperature: %d, desired temperature %d\n", m_sensor.m_type, setpoint);

			sensor = m_sensor.m_type;

			if(sensor > setpoint){
				// too hot
				delta = (sensor - setpoint);
				if(delta > threshold){
					printf("tempControl: we need turn the fan on\n");
					m_heater.m_type = 1;
					ipc_sendrec(heatAct_ep, &m_heater);
					printf("tempControl: receiving message the fan is %s\n", (m_heater.m_type ? "ON" : "OFF"));

					heater = m_heater.m_type;
					
					if(flag == 0){
						timestamp_s = time(NULL);
						flag = 1;
					}else{
						timestamp_e = time(NULL);
						time_elipsed = timestamp_e - timestamp_s;
						if(time_elipsed > time_threhold){
							printf("tempControl: we need turn the alarm on\n");
							m_alarm.m_type = 1;
							ipc_sendrec(alarmAct_ep, &m_alarm);
							printf("tempControl: receiving message the alarm is %s\n", (m_alarm.m_type ? "ON" : "OFF"));

							alarm = m_alarm.m_type;
						}
					}
				}
				else{
					flag =0;
					timestamp_s = 0;
					printf("tempControl: we need turn the alarm off\n");
					m_alarm.m_type = 0;
					ipc_sendrec(alarmAct_ep, &m_alarm);
					printf("tempControl: receiving message the alarm is %s\n", (m_alarm.m_type ? "ON" : "OFF"));

					alarm = m_alarm.m_type;
				}
			}else{
				// too cold
				delta = (setpoint - sensor);
				if(delta > threshold){
					printf("tempControl: we need turning the fan off\n");
					m_heater.m_type = 0;
					ipc_sendrec(heatAct_ep, &m_heater);
					printf("tempControl: receiving message the fan is %s\n", (m_heater.m_type ? "ON" : "OFF"));

					heater = m_heater.m_type;

					if(flag == 0){
						timestamp_s = time(NULL);
						flag = 1;
					}else{
						timestamp_e = time(NULL);
						time_elipsed = timestamp_e - timestamp_s;
						if(time_elipsed > time_threhold){
							printf("tempControl: we need turn the alarm on\n");
							m_alarm.m_type = 1;
							ipc_sendrec(alarmAct_ep, &m_alarm);
							printf("tempControl: receiving message the alarm is %s\n", (m_alarm.m_type ? "ON" : "OFF"));

							alarm = m_alarm.m_type;
						}
					}
				}
				else{
					flag = 0;
					timestamp_s = 0;
					printf("tempControl: we need turn the alarm off\n");
					m_alarm.m_type = 0;
					ipc_sendrec(alarmAct_ep, &m_alarm);
					printf("tempControl: receiving message the alarm is %s\n", (m_alarm.m_type ? "ON" : "OFF"));

					alarm = m_alarm.m_type;
				}
			}

		}else{

			newsetpoint = m_sensor.m_type;

			printf("tempControl: receives message from webInterface to update setpoint to %d\n", m_sensor.m_type);

			if(((newsetpoint >= 60) && (newsetpoint <=90))){
				setpoint = newsetpoint;
			}else{
				printf("tempControl: failed to update the setpoint, out of range\n");
			}
			m_sensor.m_type = OK;
			ipc_sendnb(web_ep, &m_sensor);

		}

		//update log file in JSON format
		char time_string[500];

		time_t timestamp_json = time(NULL);
		struct tm *time_p = localtime(&timestamp_json);

		strftime(time_string, 500, "%r,%F", time_p);

		FILE *fp = fopen("load.txt", "w");
		if(fp == NULL){
			printf("Error opening file!\n");
			exit(1);
		}
		fprintf(fp, "{\n\"Current Temperature\" : \"%d\", \"Desired Temperature\" : \"%d\", \"Fan status\" : \"%d\", \"Alarm status\" : \"%d\", \"Time\" : \"%s\"\n}", sensor, setpoint, heater, alarm, time_string);
		fclose(fp);

	}
	exit(1);

}
