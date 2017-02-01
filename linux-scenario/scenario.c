/**************************************************************
*	Temperature Control Scenario Program					  *
*	by Daniel Wang											  *
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>
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

void main(void){
	int tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid;
	int t_pid;
	int waitstatus;
	int status;

	int flag;
	mode_t perms;
	int mqd_sc, mqd_ch, mqd_hc, mqd_ca, mqd_ac, mqd_cw, mqd_sw;
	struct mq_attr attr;

	attr.mq_maxmsg = 10;
	attr.mq_msgsize = 2048;
	attr.mq_curmsgs = 0;
	flag = O_RDWR | O_CREAT;
	perms = S_IRUSR | S_IWUSR;

	char *argv[2];
	argv[1] = NULL;

	// queue for tempSensor tempControl
	mqd_sc = mq_open("/sen-cnt", flag, perms, &attr);
	if(mqd_sc == (mqd_t) -1 ){
		bail("p: mq_open(/sen-cnt)");
	}

	// queue for tempControl heaterActuator
	mqd_ch = mq_open("/cnt-heat", flag, perms, &attr);
	if(mqd_ch == (mqd_t) -1 ){
		bail("mq_open(/cnt-heat)");
	}

	mqd_hc = mq_open("/heat-cnt", flag, perms, &attr);
	if(mqd_hc == (mqd_t) -1 ){
		bail("mq_open(/heat-cnt)");
	}

	// queue for tempControl alarmActuator
	mqd_ca = mq_open("/cnt-alarm", flag, perms, &attr);
	if(mqd_ca == (mqd_t) -1 ){
		bail("mq_open(/cnt-alarm)");
	}

	mqd_ac = mq_open("/alarm-cnt", flag, perms, &attr);
	if(mqd_ac == (mqd_t) -1 ){
		bail("mq_open(/alarm-cnt)");
	}

	// queue for tempControl webInterface
	mqd_cw = mq_open("/cnt-web", flag, perms, &attr);
	if(mqd_cw == (mqd_t) -1 ){
		bail("mq_open(/cnt-web)");
	}

	// queue for scenario webInterface
	mqd_sw = mq_open("/sce-web", flag, perms, &attr);
	if(mqd_sw == (mqd_t) -1 ){
		bail("mq_open(/sce-web)");
	}

	if((tempSen_pid = fork()) == 0){
		// temp sensor process
		argv[0] = "tempSensor";
		if (execv("tempSensor", argv) == -1) {
			printf("Return not expected.\n");
			bail("execv(tempSensor)");
		}
		exit(0);
	}else if((tempCnt_pid = fork()) == 0){
		// temp control process
		argv[0] = "tempControl";
		if (execv("tempControl", argv) == -1) {
			printf("Return not expected.\n");
			bail("execv(tempControl)");
		}
		exit(0);
	}else if((heatAct_pid = fork()) == 0){
		// heater actuator process
		argv[0] = "heatActuator";
		if (execv("heatActuator", argv) == -1) {
			printf("Return not expected.\n");
			bail("execv(heatActuator)");
		}
		exit(0);
	}else if((alarmAct_pid = fork()) == 0){
		// alarm actuator process
		argv[0] = "alarmActuator";
		if (execv("alarmActuator", argv) == -1) {
			printf("Return not expected.\n");
			bail("execv(alarmActuator)");
		}
		exit(0);
	}else if((web_pid = fork()) == 0){
		// alarm actuator process
		argv[0] = "webInterface";
		if (execv("webInterface", argv) == -1) {
			printf("Return not expected.\n");
			bail("execv(webInterface)");
		}
		exit(0);
	}else{

		printf("tempSen_pid %d, tempCnt_pid %d, heatAct_pid %d, alarmAct_pid %d\n", tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid);
		Msg message = {PID_UPDATE, tempSen_pid};
		status = mq_send(mqd_sw, (const char *) &message, sizeof(message), 0);

		message.data = tempCnt_pid;
		status = mq_send(mqd_sw, (const char *) &message, sizeof(message), 0);

		message.data = heatAct_pid;
		status = mq_send(mqd_sw, (const char *) &message, sizeof(message), 0);

		message.data = alarmAct_pid;
		status = mq_send(mqd_sw, (const char *) &message, sizeof(message), 0);

		mq_close(mqd_sw);

		while(1){
			t_pid = wait(&waitstatus);
			printf("parent: child process(%d) terminated with exit status %d\n", t_pid, waitstatus);
			if(t_pid != tempSen_pid){
				continue;
			}else{
				kill(tempCnt_pid, 9);
				kill(heatAct_pid, 9);
				kill(alarmAct_pid, 9);
				kill(web_pid, 9);
				status = mq_unlink("/sen-cnt");
				if(status != 0){
					bail("mq_unlink(/sen-cnt)");
				}
				status = mq_unlink("/cnt-heat");
				if(status != 0){
					bail("mq_unlink(/cnt-heat)");
				}
				status = mq_unlink("/heat-cnt");
				if(status != 0){
					bail("mq_unlink(/heat-cnt)");
				}
				status = mq_unlink("/cnt-alarm");
				if(status != 0){
					bail("mq_unlink(/cnt-alarm)");
				}
				status = mq_unlink("/alarm-cnt");
				if(status != 0){
					bail("mq_unlink(/alarm-cnt)");
				}
				status = mq_unlink("/cnt-web");
				if(status != 0){
					bail("mq_unlink(/cnt-web)");
				}
				exit(1);
			}
		}
	}
}
