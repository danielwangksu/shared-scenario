/**************************************************************
*	Temperature Control Scenario Program					  *
*	by Daniel Wang											  *
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <lib.h>
#include <sys/un.h>
#include <strings.h>

#define MAX_ARGC  18
#define MAX_ARGS_LENGTH  10
#define INITIAL_AC_ID 100
int tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid, vpm_pid; // process pid

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

// main function
void main(void){
	int tempSen_ac_id, tempCnt_ac_id, heatAct_ac_id, alarmAct_ac_id, web_ac_id, vpm_ac_id; // process ac_id (for access control matrix)
	char args[MAX_ARGC][MAX_ARGS_LENGTH], line[MAX_ARGS_LENGTH];
	char *argv[MAX_ARGC+2];
	char *argv_control[] = {"holder", 0};
	int sce2vpm[2];
	int child_status;
	int t_pid;
	int num_bytes;
	int i;

	tempSen_ac_id = INITIAL_AC_ID;
	tempCnt_ac_id = INITIAL_AC_ID + 1;
	heatAct_ac_id = INITIAL_AC_ID + 2;
	alarmAct_ac_id = INITIAL_AC_ID + 3;
	web_ac_id = INITIAL_AC_ID + 4;
	vpm_ac_id = INITIAL_AC_ID + 5;

	pipe(sce2vpm);

	// prepare for argv array for execv
	for(i = 0; i < MAX_ARGC; i++)
    	argv[i+1] = args[i];
    argv[MAX_ARGC+1] = NULL;

	if((tempSen_pid = fork2(tempSen_ac_id)) == 0){ // fork2 tempSensor process
		//temp sensor process
		if (execv("tempSensor", argv_control) == -1) {
			printf("Return not expected.\n");
			bail("execv(tempSensor)");
		}
		exit(1);

	}else if((tempCnt_pid = fork2(tempCnt_ac_id)) == 0){ // fork2 tempControl process
		//temp control process
		if (execv("tempControl", argv_control) == -1) {
			printf("Return not expected.\n");
			bail("execv(tempControl)");
		}
		exit(2);

	}else if((heatAct_pid = fork2(heatAct_ac_id)) == 0){ // fork2 heatActuator process
		//heater actuator process
		if (execv("heatActuator", argv_control) == -1) {
			printf("Return not expected.\n");
			bail("execv(heatActuator)");
		}
		exit(3);

	}else if((alarmAct_pid = fork2(alarmAct_ac_id)) == 0){ // fork2 alarmActuator process
		// alarm actuator process
		if (execv("alarmActuator", argv_control) == -1) {
			printf("Return not expected.\n");
			bail("execv(alarmActuator)");
		}
		exit(4);
	}else if((web_pid = fork2(web_ac_id)) == 0){ // fork2 webInterface process
		// alarm actuator process
		if (execv("webInterface", argv_control) == -1) {
			printf("Return not expected.\n");
			bail("execv(webInterface)");
		}
		exit(5);
	}else if((vpm_pid = fork2(vpm_ac_id)) == 0){
		num_bytes = read(sce2vpm[0], args, sizeof(args));
		if (num_bytes != sizeof(args))
        	bail("read(sce2vpm[0])");

        argv[0] = "virtualPM";
        argv[0] = "virtualPM";
     	if (execv("virtualPM", argv) == -1) {
       		printf("virtualPM: Exec error, errno = %d\n", errno);
       		exit(10);
     	}
     	exit(20);

	}else{
		// parent process
		int tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, web_ep;
		int ret;

		sprintf(line, "%d", tempSen_pid); strcpy(args[0], line);
		sprintf(line, "%d", tempCnt_pid); strcpy(args[1], line);
		sprintf(line, "%d", heatAct_pid); strcpy(args[2], line);
		sprintf(line, "%d", alarmAct_pid); strcpy(args[3], line);
		sprintf(line, "%d", web_pid); strcpy(args[4], line);
		num_bytes = write(sce2vpm[1], args, sizeof(args));
		if (num_bytes != sizeof(args))
          bail("write(sce2vpm[1])");

		tempSen_ep = getendpoint(tempSen_pid); 
		tempCnt_ep = getendpoint(tempCnt_pid);
		heatAct_ep = getendpoint(heatAct_pid);
		alarmAct_ep = getendpoint(alarmAct_pid);
		web_ep = getendpoint(web_pid);

		printf("Parent: tempSen_pid=%d, tempCnt_pid=%d, heatAct_pid=%d, alarmAct_pid=%d, web_pid=%d\n", tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid);

		printf("Parent: tempSen_ep=%d, tempCnt_ep=%d, heatAct_ep=%d, alarmAct_ep=%d, web_ep=%d\n", tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, web_ep);

		// wait for children to terminate, if one terminate then terminate all
		t_pid = wait(&child_status);
		printf("Parent: child process(%d) terminated with exit status %d\n", t_pid, child_status);
		// ret = kill(alarmAct_pid, SIGKILL);
		// printf("Parent: kill alarmAct return %d\n", ret);
		// ret = kill(heatAct_pid, SIGKILL);
		// printf("Parent: kill heatAct return %d\n", ret);
		// ret = kill(tempCnt_pid, SIGKILL);
		// printf("Parent: kill tempCnt return %d\n", ret);
		// ret = kill(web_pid, SIGKILL);
		// printf("Parent: kill web return %d\n", ret);
		exit(1);
	}

}
