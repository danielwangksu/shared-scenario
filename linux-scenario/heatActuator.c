/**************************************************************
*	Heater Actuator Program								      *
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
#include "msg.h"

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

void main(int argc, char **argv){
	mqd_t mqd_ch, mqd_hc;
	int prio = 0;
	int status;
	int flag = O_RDONLY;
	int rwflag = O_RDWR;
	ssize_t len;
	Msg message;
	struct mq_attr attr_ch;
	int past_heater_status = -1;
	int heater_status = -1;

	mqd_ch = mq_open("/cnt-heat", flag);
	if(mqd_ch == (mqd_t) -1 ){
		bail("hc: mq_open(/cnt-heat)");
	}
	if (mq_getattr(mqd_ch, &attr_ch) == -1)
		bail("hc: mq_getattr(mqd_ch)");

	mqd_hc = mq_open("/heat-cnt", rwflag);
	if(mqd_hc == (mqd_t) -1 ){
		bail("hc: mq_open(/heat-cnt)");
	}

	printf("heatActuator is loaded\n");

	while(1){
		len = mq_receive(mqd_ch, (char *) &message, attr_ch.mq_msgsize, &prio);
		if(len == -1)
			bail("hc: mq_receive(mqd_ch)");
		if(message.type == HEATER_COMMAND){
			past_heater_status = heater_status;
			heater_status = message.data;

			printf("heatActuator: old_status=%d, current_status=%d\n", past_heater_status, heater_status);

			Msg message_return = {HEATER_CONFIRM, heater_status};
			status = mq_send(mqd_hc, (const char *) &message_return, sizeof(message_return), 0);
			if(status != 0){
				bail("ha: send failed");
			}
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