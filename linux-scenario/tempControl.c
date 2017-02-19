/**************************************************************
*       Temperature Control Program                           *
*       by Daniel Wang                                        *
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
#define OK 0
#define OFF 0
#define MAX_BUF 64
#define FAN 115
#define LED_RED 112
#define LED_GREEN 15
#define LED_BLUE 14


float itof(int);
int gpioChange(int gpio, int value);

static volatile int keepRunning = 1;
float const threshold = 1.0;
time_t const time_threshold = 30; // 30 sec
float delta = 0.0;
float setpoint = 27.0;

time_t timestamp_s = 0;
time_t timestamp_e = 0;
time_t time_elipsed = 0;
int set_alarm = 0;

int mqd_sc, mqd_ch, mqd_hc, mqd_ca, mqd_ac, mqd_cw;
struct mq_attr attr_sc, attr_ch, attr_hc, attr_ca, attr_ac, attr_cw;
Msg message_sc, message_ch, message_hc, message_ca, message_ac, message_cw;

int sensor_data = -1;
int heater_data = 0;
int alarm_data = 0;
int web_setpoint = -1;

// fault handler
static void bail(const char *on_what) {
		perror(on_what);
}

void open_mq(void){
	int rflag = O_RDONLY;
	int rnflag = O_RDONLY | O_NONBLOCK;
	int rwflag = O_RDWR;

	// open queue for tempSensor
	mqd_sc = mq_open("/sen-cnt", rflag);
	if(mqd_sc == (mqd_t) -1 )
			bail("tc: mq_open(/sen-cnt)");
	if (mq_getattr(mqd_sc, &attr_sc) == -1)
			bail("tc: mq_getattr(mqd_sc)");

	// open queue for heaterActuator
	mqd_ch = mq_open("/cnt-heat", rwflag);
	if(mqd_ch == (mqd_t) -1 )
			bail("tc: mq_open(/cnt-heat)");
	if (mq_getattr(mqd_ch, &attr_ch) == -1)
			bail("tc: mq_getattr(mqd_ch)");

	mqd_hc = mq_open("/heat-cnt", rwflag);
	if(mqd_hc == (mqd_t) -1 )
			bail("tc: mq_open(/heat-cnt)");
	if (mq_getattr(mqd_hc, &attr_hc) == -1)
			bail("tc: mq_getattr(mqd_hc)");

	// open queue for alarmActuator
	mqd_ca = mq_open("/cnt-alarm", rwflag);
	if(mqd_ca == (mqd_t) -1 )
			bail("tc: mq_open(/cnt-alarm)");
	if (mq_getattr(mqd_ca, &attr_ca) == -1)
			bail("tc: mq_getattr(mqd_ca)");

	mqd_ac = mq_open("/alarm-cnt", rwflag);
	if(mqd_ac == (mqd_t) -1 )
			bail("tc: mq_open(/alarm-cnt)");
	if (mq_getattr(mqd_ac, &attr_ac) == -1)
			bail("tc: mq_getattr(mqd_ac)");

	// open queue for webInterface
	mqd_cw = mq_open("/cnt-web", rflag);
	if(mqd_cw == (mqd_t) -1 )
			bail("tc: mq_open(/cnt-web)");
	if (mq_getattr(mqd_cw, &attr_cw) == -1)
			bail("tc: mq_getattr(mqd_cw)");
}

// check timer and turn ON alarm if needed
void check_timer(){
	int status;
	// check timer
	if(set_alarm == 0){
		timestamp_s = time(NULL);
		set_alarm = 1;
	}
	else{
		timestamp_e = time(NULL);
		time_elipsed = timestamp_e - timestamp_s;
		if(time_elipsed > time_threshold){
			printf("tempControl: we need turn the alarm ON\n");
			status = mq_send_alarm(ON);
			if(status != OK)
				bail("mq_send_heater(ON)");
		}
	}
}

// communicate with heater
int mq_send_heater(int command){
	int status, len, prio; 
	message_ch = (Msg) {HEATER_COMMAND, command};
	status = mq_send(mqd_ch, (const char *) &message_ch, sizeof(message_ch), 0);
	if(status != OK)
		return status;

	len = mq_receive(mqd_hc, (char *) &message_hc, attr_hc.mq_msgsize, &prio);
	if(len == -1)
		return -1;
	if(message_hc.type == HEATER_CONFIRM)
		heater_data = message_hc.data;
	return 0;
}

// communicate with alarm
int mq_send_alarm(int command){
	int status, len, prio;
	message_ca = (Msg) {ALARM_COMMAND, command};

	status = mq_send(mqd_ca, (const char *) &message_ca, sizeof(message_ca), 0);
	if(status != OK)
		return status;

	len = mq_receive(mqd_ac, (char *) &message_ac, attr_ac.mq_msgsize, &prio);
	if(len == -1)
		return -1;
	if(message_ac.type == ALARM_CONFIRM)
		alarm_data = message_ac.data;
	return 0;
}

// real control logic
void control_start(void){
	int len, status;

	sensor_data = message_sc.data;
	printf("tempControl: received sensor data: %f, current desired temperature %f\n", itof(sensor_data), setpoint);

	if(itof(sensor_data) > setpoint){
		// too hot
		delta = (itof(sensor_data) - setpoint);
		printf("\nDelta : %f\n",delta);
		if(delta > threshold){
			printf("tempControl: we need to turn the fan ON\n");
			status = mq_send_heater(ON);
			if(status != OK)
				bail("mq_send_heater(ON)");
			check_timer();
		}
		else{
			if(alarm_data){
				set_alarm = 0;
				timestamp_s = 0;
				printf("tempControl: we need turn the alarm OFF\n");
				status = mq_send_alarm(OFF);
				if(status != OK)
					bail("mq_send_heater(OFF)");
			}
		}
	}
	else{
		// too cold
		delta = (setpoint - itof(sensor_data));
		printf("\nDelta : %f\n",delta);
		if(delta > threshold){
			printf("tempControl: we need to turn the fan OFF\n");
			status = mq_send_heater(OFF);
			if(status != OK)
				bail("mq_send_heater(OFF)");
			// check_timer();
		}
		// else{
		// 	if(alarm_data){
		// 		set_alarm = 0;
		// 		timestamp_s = 0;
		// 		printf("tempControl: we need turn the alarm OFF\n");
		// 		status = mq_send_alarm(OFF);
		// 		if(status != OK)
		// 			bail("mq_send_heater(OFF)");
		// 	}
		// }
	}
}

// logging function
int logging(void){
	char time_string[500];
	time_t timestamp_json = time(NULL);
	struct tm *time_p = localtime(&timestamp_json);

	strftime(time_string, 500, "%r,%F", time_p);

	FILE *fp = fopen("load.txt", "w");
	if(fp == NULL){
		return -1;
	}
	fprintf(fp, "{\n\"Current Temperature\" : \"%.1f\", \"Desired Temperature\" : \"%.1f\", \"Fan status\" : \"%s\", \"Alarm status\" : \"%s\", \"Time\" : \"%s\"\n}", itof(sensor_data), setpoint, (heater_data) ? "ON" : "OFF", (alarm_data) ? "ON" : "OFF", time_string);
	fclose(fp);
	return 0;
}

// polling for setpoint update
void update_setpoint(){
	struct pollfd fds;
	fds.fd = mqd_cw;
	fds.events = POLLIN;
	int timeout_msecs = 500;
	int prio, status, len;

	status = poll(&fds, 1, timeout_msecs);

	if(status > 0){
		len = mq_receive(mqd_cw, (char *) &message_cw, attr_cw.mq_msgsize, &prio);
		if(len == -1)
			bail("tc: mq_receive(mqd_cw)");
		web_setpoint = message_cw.data;
		if(web_setpoint > 20 && web_setpoint < 40){
			setpoint = web_setpoint;
			printf("tc: updated setpoint to %f\n", setpoint);
		}
	}
}

void intHandler(int dummy){
	int ret, status;
	char filename[] = "load.txt";

	printf("TC: get interrupt\n");
	ret = remove(filename);
	printf("ret = %d\n", ret);
	fflush(stdout);
	ret = gpioChange(LED_BLUE,OFF);
	ret = gpioChange(LED_GREEN,OFF);
	ret = gpioChange(LED_RED,OFF);
	ret = gpioChange(FAN,OFF);
	status = mq_close(mqd_sc);
	status = mq_close(mqd_ch);
	status = mq_close(mqd_hc);
	status = mq_close(mqd_ca);
	status = mq_close(mqd_ac);
	status = mq_close(mqd_cw);
	keepRunning = 0;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*       main function                           *
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void main(int argc, char **argv){
	ssize_t len;
	unsigned int prio;
	int status;

	// initialize message queue
	open_mq();

	printf("tempContol is loaded\n");

	signal(SIGINT, intHandler);

	while(keepRunning){
		len = mq_receive(mqd_sc, (char *) &message_sc, attr_sc.mq_msgsize, &prio);
		if(len == -1)
			bail("tc: mq_receive(mqd_sc)");

		if(message_sc.type == SENSOR_UPDATE){
			// real control logic
			control_start();
		}
		else{
			continue;
		}

		// update log file in JSON format
		status = logging();
		if(status != OK){
			bail("log()");
		}

		// poll for setpoint update
		update_setpoint();

	}// end while
	status = mq_unlink("/sen-cnt");
	if(status != OK){
		bail("mq_unlink(/sen-cnt)");
	}
	status = mq_unlink("/cnt-heat");
	if(status != OK){
		bail("mq_unlink(/cnt-heat)");
	}
	status = mq_unlink("/heat-cnt");
	if(status != OK){
		bail("mq_unlink(/heat-cnt)");
	}
	status = mq_unlink("/cnt-alarm");
	if(status != OK){
		bail("mq_unlink(/cnt-alarm)");
	}
	status = mq_unlink("/alarm-cnt");
	if(status != OK){
		bail("mq_unlink(/alarm-cnt)");
	}
	status = mq_unlink("/cnt-web");
	if(status != OK){
		bail("mq_unlink(/cnt-web)");
	}
	exit(0);
}

// function to convert a 3 digit int to single decimal float value
float itof(int intval){
	float q = intval/10;
	float r = intval%10;
	return (q+(r*0.1));
}

// function for change GPIO device
int gpioChange(int gpio,int value){
	FILE *gfp;
	char path[MAX_BUF];
	//Open a file to the specified GPIO and write the values of either 0 or $
	snprintf(path, sizeof path, "/sys/class/gpio/gpio%d/value",gpio);
	printf("\nGPIO path is : %s Value : %d\n",path,value);
	if((gfp = fopen(path,"w")) == NULL){
		printf("file open failed");
		return 1;
	}
	rewind(gfp);
	fprintf(gfp, "%d", value);
	fflush(gfp);
	fclose(gfp);
	return 0;
}
