/**************************************************************
*	Heater Actuator Program								      *
*	by Daniel Wang											  *
***************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <lib.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>


// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}


void main(int argc, char **argv){
	int tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid;
	int tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, web_ep;
	int status;

	int paststatus = -1;
	int currentstatus = -1;

	message m_in;
	message m_out;

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

	printf("heatActuator: tempSen_pid=%d, tempCnt_pid=%d, heatAct_pid=%d, alarmAct_pid=%d,web_pid=%d, tempSen_ep=%d, tempCnt_ep=%d, heatAct_ep=%d, alarmAct_ep=%d, web_ep=%d\n", tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid, tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, web_ep);

	while(1){
		ipc_receive(tempCnt_ep, &m_in, &status);

		paststatus = currentstatus;
		currentstatus = m_in.m_type;

		printf("heatActuator: paststatus=%d, receiving message turning the fan %s\n", paststatus, (m_in.m_type ? "ON" : "OFF"));

		m_out.m_type = currentstatus;
		ipc_send(tempCnt_ep, &m_out);

	}
	exit(1);

}