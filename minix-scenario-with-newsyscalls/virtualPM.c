/**************************************************************
*	Virtual PM Program										  *
*	by Daniel Wang											  *
***************************************************************/

#define _MINIX_SYSTEM 1
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <lib.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include "msg.h"

#define OK 0
int tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid;
int tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, web_ep;

void initialize(){
	tempSen_ep = getendpoint(tempSen_pid); 
	tempCnt_ep = getendpoint(tempCnt_pid);
	heatAct_ep = getendpoint(heatAct_pid);
	alarmAct_ep = getendpoint(alarmAct_pid);
	web_ep = getendpoint(web_pid);
}

/**************************************************************
*	main function							 				  *
***************************************************************/
void main(int argc, char **argv){
	int r, status, result;
	int who_e;
	message m;

	tempSen_pid = atoi(argv[1]);
	tempCnt_pid = atoi(argv[2]);
	heatAct_pid = atoi(argv[3]);
	alarmAct_pid = atoi(argv[4]);
	web_pid = atoi(argv[5]);

	initialize();
	printf("VPM: tempSen_pid=%d, tempCnt_pid=%d, heatAct_pid=%d, alarmAct_pid=%d, web_pid=%d\n", tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid);

	printf("VPM: tempSen_ep=%d, tempCnt_ep=%d, heatAct_ep=%d, alarmAct_ep=%d, web_ep=%d\n", tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, web_ep);

	while(1){
		memset(&m, 0, sizeof(m));
		r = ipc_receive(ANY, &m, &status);
		if(m.m_type == VPM_DOKILL){

			who_e = m.m_source;

			if(m.m_m1.m1i1 == tempSen_ep)
				result = kill(tempSen_pid, SIGKILL);
			if(m.m_m1.m1i1 == tempCnt_ep)
				result = kill(tempCnt_pid, SIGKILL);
			if(m.m_m1.m1i1 == heatAct_ep)
				result = kill(heatAct_pid, SIGKILL);
			if(m.m_m1.m1i1 == alarmAct_ep)
				result = kill(alarmAct_pid, SIGKILL);
			if(m.m_m1.m1i1 == web_ep)
				result = kill(web_pid, SIGKILL);

			memset(&m, 0, sizeof(m));
			m.m_type = 0;
			m.m_m1.m1i1 = result;
			r = ipc_sendnb(who_e, &m);
		}
	}
}