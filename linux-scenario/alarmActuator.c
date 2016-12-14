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

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

void main(int argc, char **argv){
	mqd_t mqd_ca, mqd_ac;
	int prio = 0;
	int status;
	int flag = O_RDONLY;
	int rwflag = O_RDWR;
	ssize_t len;
	Msg message;
	struct mq_attr attr_ca;
	int past_alarm_status = -1;
	int alarm_status = -1;

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

	while(1){
		len = mq_receive(mqd_ca, (char *) &message, attr_ca.mq_msgsize, &prio);
		if(len == -1)
			bail("al: mq_receive(mqd_ca)");
		if(message.type == ALARM_COMMAND){
			past_alarm_status = alarm_status;
			alarm_status = message.data;

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