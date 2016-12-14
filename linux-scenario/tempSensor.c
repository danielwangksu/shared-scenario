/**************************************************************
*	Temperature Sensor Program								  *
*	by Daniel Wang											  *
***************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include "msg.h"

const int sensordata[] =  {70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, -1};

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

void main(int argc, char **argv){
	mqd_t mqd_sc;
	int piro = 0;
	int status;
	int i = 0;
	int flag = O_WRONLY;

	mqd_sc = mq_open("/sen-cnt", flag);
	if(mqd_sc == (mqd_t) -1 ){
		bail("ts: mq_open(/sen-cnt)");
	}

	printf("tempSensor is loaded\n");

	while(1){
		if(sensordata[i] == -1){
			break;
		}
		sleep(2);

		printf("tempSensor: sending %d\n", sensordata[i]);

		Msg message = {SENSOR_UPDATE, sensordata[i]};
		status = mq_send(mqd_sc, (const char *) &message, sizeof(message), 0);
		i++;
	}
	status = mq_unlink("/sen-cnt");
	if(status != 0){
		exit(-1);
	}
	exit(0);
}