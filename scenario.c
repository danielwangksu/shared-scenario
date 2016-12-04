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

#define INITIAL_IPC_ID 40
#define MAX_ARGC  18
#define MAX_ARGS_LENGTH  10

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

// main function
void main(void){
	int tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid; // process pid
	int tempSen_ipc_id, tempCnt_ipc_id, heatAct_ipc_id, alarmAct_ipc_id, web_ipc_id; // process ipc_id (for access control matrix)
	int p2tS[2], p2tC[2], p2tH[2], p2tA[2], p2tW[2]; // pipe for passing pid and endpoint
	int tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, web_ep; // process endpoint
	int args_byte;
	int waitstatus;
	int t_pid;
	int i;

	char args[MAX_ARGC][MAX_ARGS_LENGTH]; // array to pass variables to newly forked and execed processes
	char *argv[MAX_ARGC+2]; // args pointer for c convention
	char line[MAX_ARGS_LENGTH]; // intermediate array for args

	tempSen_ipc_id = INITIAL_IPC_ID;
	tempCnt_ipc_id = INITIAL_IPC_ID + 1;
	heatAct_ipc_id = INITIAL_IPC_ID + 2;
	alarmAct_ipc_id = INITIAL_IPC_ID + 3;
	web_ipc_id = INITIAL_IPC_ID + 4;

	// create pipe for each child processes
	pipe(p2tS);
	pipe(p2tC);
	pipe(p2tH);
	pipe(p2tA);
	pipe(p2tW);

	// initialize the argv pointer
	for(i = 0; i < MAX_ARGC; i++){
		argv[i+1] = args[i];
	}
	argv[MAX_ARGC+1] = NULL;

	if((tempSen_pid = fork2(tempSen_ipc_id)) == 0){ // fork2 tempSensor process
		//temp sensor process
		args_byte = read(p2tS[0], args, sizeof(args));
		if(args_byte != sizeof(args)){
			bail("read(tempSensor)");
		}

		argv[0] = "tempSensor";
		if (execv("tempSensor", argv) == -1) {
			printf("Return not expected.\n");
			bail("execv(tempSensor)");
		}
		exit(1);

	}else if((tempCnt_pid = fork2(tempCnt_ipc_id)) == 0){ // fork2 tempControl process
		//temp control process
		args_byte = read(p2tC[0], args, sizeof(args));
		if(args_byte != sizeof(args)){
			bail("read(tempControl)");
		}

		argv[0] = "tempControl";
		if (execv("tempControl", argv) == -1) {
			printf("Return not expected.\n");
			bail("execv(tempControl)");
		}
		exit(2);

	}else if((heatAct_pid = fork2(heatAct_ipc_id)) == 0){ // fork2 heatActuator process
		//heater actuator process
		args_byte = read(p2tH[0], args, sizeof(args));
		if(args_byte != sizeof(args)){
			bail("read(heatActuator)");
		}

		argv[0] = "heatActuator";
		if (execv("heatActuator", argv) == -1) {
			printf("Return not expected.\n");
			bail("execv(heatActuator)");
		}
		exit(3);

	}else if((alarmAct_pid = fork2(alarmAct_ipc_id)) == 0){ // fork2 alarmActuator process
		// alarm actuator process
		args_byte = read(p2tA[0], args, sizeof(args));
		if(args_byte != sizeof(args)){
			bail("read(alarmActuator)");
		}

		argv[0] = "alarmActuator";
		if (execv("alarmActuator", argv) == -1) {
			printf("Return not expected.\n");
			bail("execv(alarmActuator)");
		}
		exit(4);
	}else if((web_pid = fork2(web_ipc_id)) == 0){ // fork2 webInterface process
		// alarm actuator process
		args_byte = read(p2tW[0], args, sizeof(args));
		if(args_byte != sizeof(args)){
			bail("read(webInterface)");
		}

		argv[0] = "webInterface";
		if (execv("webInterface", argv) == -1) {
			printf("Return not expected.\n");
			bail("execv(webInterface)");
		}
		exit(5);
	}else{
		// parent process
		// get child process endpoints from pid
		tempSen_ep = getendpoint(tempSen_pid); 
		tempCnt_ep = getendpoint(tempCnt_pid);
		heatAct_ep = getendpoint(heatAct_pid);
		alarmAct_ep = getendpoint(alarmAct_pid);
		web_ep = getendpoint(web_pid);

		printf("Parent: tempSen_pid=%d, tempCnt_pid=%d, heatAct_pid=%d, alarmAct_pid=%d,web_pid=%d, tempSen_ep=%d, tempCnt_ep=%d, heatAct_ep=%d, alarmAct_ep=%d, web_ep=%d\n", tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid, tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, web_ep);


		sprintf(line, "%d", tempSen_pid); strcpy(args[0], line);
        sprintf(line, "%d", tempCnt_pid); strcpy(args[1], line);
        sprintf(line, "%d", heatAct_pid); strcpy(args[2], line);
        sprintf(line, "%d", alarmAct_pid); strcpy(args[3], line);
        sprintf(line, "%d", web_pid); strcpy(args[4], line);
        sprintf(line, "%d", tempSen_ep); strcpy(args[5], line);
        sprintf(line, "%d", tempCnt_ep); strcpy(args[6], line);
        sprintf(line, "%d", heatAct_ep); strcpy(args[7], line);
        sprintf(line, "%d", alarmAct_ep); strcpy(args[8], line);
        sprintf(line, "%d", web_ep); strcpy(args[9], line);

		args_byte = write(p2tS[1], args, sizeof(args));
		if(args_byte != sizeof(args)){
			bail("write(tempSensor)");
		}

		args_byte = write(p2tC[1], args, sizeof(args));
		if(args_byte != sizeof(args)){
			bail("write(tempControl)");
		}

		args_byte = write(p2tH[1], args, sizeof(args));
		if(args_byte != sizeof(args)){
			bail("write(heatActuator)");
		}

		args_byte = write(p2tA[1], args, sizeof(args));
		if(args_byte != sizeof(args)){
			bail("write(alarmActuator)");
		}

		args_byte = write(p2tW[1], args, sizeof(args));
		if(args_byte != sizeof(args)){
			bail("write(webInterface)");
		}

		// wait for children to terminate, if tempSensor terminate then terminate all
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
				exit(1);
			}
		}
	}

}
