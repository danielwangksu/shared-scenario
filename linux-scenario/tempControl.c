/**************************************************************
*	Temperature Control Program								  *
*	by Daniel Wang											  *
***************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <math.h>
#include <time.h>
#include <poll.h>
#include "msg.h"

#define ON 1
#define OFF 0
int const threshold = 3;
time_t const time_threshold = 60; // 1 mins

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

void main(int argc, char **argv){
	int rflag = O_RDONLY;
	int rnflag = O_RDONLY | O_NONBLOCK;
	int rwflag = O_RDWR;

	ssize_t len;
	unsigned int prio;
	int status;

	int mqd_sc, mqd_ch, mqd_hc, mqd_ca, mqd_ac, mqd_cw;
	struct mq_attr attr_sc, attr_ch, attr_hc, attr_ca, attr_ac, attr_cw;

	time_t timestamp_s = 0;
	time_t timestamp_e = 0;
	time_t time_elipsed = 0;
	int set_alarm = 0;

	int delta = 0;
	int setpoint = 75;

	int sensor_data = -1;
	int heater_data = -1;
	int alarm_data = -1;
	int web_setpoint = -1;

	// open queue for tempSensor
	mqd_sc = mq_open("/sen-cnt", rflag);
	if(mqd_sc == (mqd_t) -1 )
		bail("tc: mq_open(/sen-cnt)");
	if (mq_getattr(mqd_sc, &attr_sc) == -1)
		bail("tc: mq_getattr(mqd_sc)");
	Msg message_sc;

	// open queue for heaterActuator
	mqd_ch = mq_open("/cnt-heat", rwflag);
	if(mqd_ch == (mqd_t) -1 )
		bail("tc: mq_open(/cnt-heat)");
	if (mq_getattr(mqd_ch, &attr_ch) == -1)
		bail("tc: mq_getattr(mqd_ch)");
	Msg message_ch;

	mqd_hc = mq_open("/heat-cnt", rwflag);
	if(mqd_hc == (mqd_t) -1 )
		bail("tc: mq_open(/heat-cnt)");
	if (mq_getattr(mqd_hc, &attr_hc) == -1)
		bail("tc: mq_getattr(mqd_hc)");
	Msg message_hc;

	// open queue for alarmActuator
	mqd_ca = mq_open("/cnt-alarm", rwflag);
	if(mqd_ca == (mqd_t) -1 )
		bail("tc: mq_open(/cnt-alarm)");
	if (mq_getattr(mqd_ca, &attr_ca) == -1)
		bail("tc: mq_getattr(mqd_ca)");
	Msg message_ca;

	mqd_ac = mq_open("/alarm-cnt", rwflag);
	if(mqd_ac == (mqd_t) -1 )
		bail("tc: mq_open(/alarm-cnt)");
	if (mq_getattr(mqd_ac, &attr_ac) == -1)
		bail("tc: mq_getattr(mqd_ac)");
	Msg message_ac;

	// open queue for webInterface
	mqd_cw = mq_open("/cnt-web", rflag);
	if(mqd_cw == (mqd_t) -1 )
		bail("tc: mq_open(/cnt-web)");
	if (mq_getattr(mqd_cw, &attr_cw) == -1)
		bail("tc: mq_getattr(mqd_cw)");
	Msg message_cw;

	printf("tempContol is loaded\n");

	while(1){
		len = mq_receive(mqd_sc, (char *) &message_sc, attr_sc.mq_msgsize, &prio);
		if(len == -1)
			bail("tc: mq_receive(mqd_sc)");

		if(message_sc.type == SENSOR_UPDATE){
			sensor_data = message_sc.data;
			printf("tempControl: received sensor data: %d, current desired temperature %d\n", sensor_data, setpoint);

			if(sensor_data > setpoint){
				// too hot?
				delta = (sensor_data - setpoint);
				if(delta > threshold){
					printf("tempControl: we need to turn the fan on\n");
					message_ch = (Msg) {HEATER_COMMAND, ON};
					status = mq_send(mqd_ch, (const char *) &message_ch, sizeof(message_ch), 0);

					len = mq_receive(mqd_hc, (char *) &message_hc, attr_hc.mq_msgsize, &prio);
					if(len == -1)
						bail("tc: mq_receive(mqd_hc)");
					if(message_hc.type == HEATER_CONFIRM)
						heater_data = message_hc.data;

					if(set_alarm == 0){
						timestamp_s = time(NULL);
						set_alarm = 1;
					}else{
						timestamp_e = time(NULL);
						time_elipsed = timestamp_e - timestamp_s;
						if(time_elipsed > time_threshold){
							printf("tempControl: we need turn the alarm on\n");
							message_ca = (Msg) {ALARM_COMMAND, ON};
							status = mq_send(mqd_ca, (const char *) &message_ca, sizeof(message_ca), 0);

							len = mq_receive(mqd_ac, (char *) &message_ac, attr_ac.mq_msgsize, &prio);
							if(len == -1)
								bail("tc: mq_receive(mqd_ac)");
							if(message_ac.type == ALARM_CONFIRM)
								alarm_data = message_ac.data;
						}
					}
				}else{
					set_alarm = 0;
					timestamp_s = 0;
					printf("tempControl: we need turn the alarm off\n");
					message_ca = (Msg) {ALARM_COMMAND, OFF};
					status = mq_send(mqd_ca, (const char *) &message_ca, sizeof(message_ca), 0);

					len = mq_receive(mqd_ac, (char *) &message_ac, attr_ac.mq_msgsize, &prio);
					if(len == -1)
						bail("tc: mq_receive(mqd_ac)");
					if(message_ac.type == ALARM_CONFIRM)
						alarm_data = message_ac.data;
				}
			}else{
				// too cold?
				delta = (setpoint - sensor_data);
				if(delta > threshold){
					printf("tempControl: we need to turn the fan off\n");
					message_ch = (Msg) {HEATER_COMMAND, OFF};
					status = mq_send(mqd_ch, (const char *) &message_ch, sizeof(message_ch), 0);

					len = mq_receive(mqd_hc, (char *) &message_hc, attr_hc.mq_msgsize, &prio);
					if(len == -1)
						bail("tc: mq_receive(mqd_hc)");
					if(message_hc.type == HEATER_CONFIRM)
						heater_data = message_hc.data;

					if(set_alarm == 0){
						timestamp_s = time(NULL);
						set_alarm = 1;
					}else{
						timestamp_e = time(NULL);
						time_elipsed = timestamp_e - timestamp_s;
						if(time_elipsed > time_threshold){
							printf("tempControl: we need turn the alarm on\n");
							message_ca = (Msg) {ALARM_COMMAND, ON};
							status = mq_send(mqd_ca, (const char *) &message_ca, sizeof(message_ca), 0);
						}
					}
				}else{
					set_alarm = 0;
					timestamp_s = 0;
					printf("tempControl: we need turn the alarm off\n");
					message_ca = (Msg) {ALARM_COMMAND, OFF};
					status = mq_send(mqd_ca, (const char *) &message_ca, sizeof(message_ca), 0);

					len = mq_receive(mqd_ac, (char *) &message_ac, attr_ac.mq_msgsize, &prio);
					if(len == -1)
						bail("tc: mq_receive(mqd_ac)");
					if(message_ac.type == ALARM_CONFIRM)
						alarm_data = message_ac.data;
				}
			}
		}

		// update log file in JSON format
		char time_string[500];
		time_t timestamp_json = time(NULL);
		struct tm *time_p = localtime(&timestamp_json);

		strftime(time_string, 500, "%r,%F", time_p);

		FILE *fp = fopen("load.txt", "w");
		if(fp == NULL){
			printf("Error opening file!\n");
			exit(1);
		}
		fprintf(fp, "{\n\"Current Temperature\" : \"%d\", \"Desired Temperature\" : \"%d\", \"Fan status\" : \"%d\", \"Alarm status\" : \"%d\", \"Time\" : \"%s\"\n}", sensor_data, setpoint, heater_data, alarm_data, time_string);
		fclose(fp);

		struct pollfd fds;
		fds.fd = mqd_cw;
		fds.events = POLLIN;
		int timeout_msecs = 500;
		status = poll(&fds, 1, timeout_msecs);
		if(status > 0){
			len = mq_receive(mqd_cw, (char *) &message_cw, attr_cw.mq_msgsize, &prio);
			if(len == -1)
				bail("tc: mq_receive(mqd_cw)");
			web_setpoint = message_cw.data;
			if(web_setpoint > 60 && web_setpoint < 85){
				setpoint = web_setpoint;
				printf("tc: updated setpoint to %d\n", setpoint);
			}
		}

	}
	status = mq_unlink("/sen-cnt");
	if(status != 0){
		bail("mq_unlink(/sen-cnt)");
	}
	status = mq_unlink("/cnt-heat");
	if(status != 0){
		bail("mq_unlink(/cnt-heat)");
	}
	status = mq_unlink("/cnt-alarm");
	if(status != 0){
		bail("mq_unlink(/cnt-alarm)");
	}
	status = mq_unlink("/cnt-web");
	if(status != 0){
		bail("mq_unlink(/cnt-web)");
	}
	exit(0);
}