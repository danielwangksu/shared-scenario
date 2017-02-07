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

#define ON 1
#define OK 0
#define OFF 0
#define FAN 115
#define MAX_BUF 64

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

void main(int argc, char **argv){
	mqd_t mqd_ch, mqd_hc;
	int prio = 0;
	int status, retval;
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
			retval = gpioChange(FAN, heater_status);
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