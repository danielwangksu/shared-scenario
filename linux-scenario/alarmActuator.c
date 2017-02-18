/**************************************************************
*	Alarm Actuator Program								  *
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
#include "msg.h"

#define ON 1
#define OK 0
#define OFF 0
#define LED_RED 112
#define LED_GREEN 15
#define LED_BLUE 14
#define MAX_BUF 64

mqd_t mqd_ca, mqd_ac;
static volatile int keepRunning = 1;

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
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

void setLED(int alarm_status){
	int ret;
	if(alarm_status == ON){
		ret = gpioChange(LED_BLUE,OFF);
		ret = gpioChange(LED_GREEN,OFF);
		ret = gpioChange(LED_RED,ON);
	}
	if(alarm_status == OFF){
		ret = gpioChange(LED_RED,OFF);
		ret = gpioChange(LED_BLUE,OFF);
		ret = gpioChange(LED_GREEN,ON);

	}
}

void intHandler(int dummy){
	int status;

	printf("AA: get interrupt\n");
	status = mq_close(mqd_ca);
	status = mq_close(mqd_ac);
	keepRunning = 0;
}

void main(int argc, char **argv){
	int prio = 0;
	int status, retval;
	int flag = O_RDONLY;
	int rwflag = O_RDWR;
	ssize_t len;
	Msg message;
	struct mq_attr attr_ca;
	int past_alarm_status = -1;
	int alarm_status = -1;

	signal(SIGINT, intHandler);

	mqd_ca = mq_open("/cnt-alarm", flag);
	if(mqd_ca == (mqd_t) -1 ){
		bail("al: mq_open(/cnt-alarm)");
	}
	if (mq_getattr(mqd_ca, &attr_ca) == -1)
		bail("al: mq_getattr(mqd_ca)");

	mqd_ac = mq_open("/alarm-cnt", rwflag);
	if(mqd_ac == (mqd_t) -1 ){
		bail("al: mq_open(/alarm-cnt)");
	}

	printf("alarmActuator is loaded\n");

	while(keepRunning){
		len = mq_receive(mqd_ca, (char *) &message, attr_ca.mq_msgsize, &prio);
		if(len == -1)
			bail("al: mq_receive(mqd_ca)");
		if(message.type == ALARM_COMMAND){
			past_alarm_status = alarm_status;
			alarm_status = message.data;

			setLED(alarm_status);

			printf("alarmActuator: old_status=%d, current_status=%d\n", past_alarm_status, alarm_status);

			Msg message = {ALARM_CONFIRM, alarm_status};
			status = mq_send(mqd_ac, (const char *) &message, sizeof(message), 0);
		}
	}
	status = mq_unlink("/cnt-heat");
	if(status != 0){
		exit(-1);
	}
	status = mq_unlink("/heat-cnt");
	if(status != 0){
		exit(-1);
	}
	exit(0);
	
}