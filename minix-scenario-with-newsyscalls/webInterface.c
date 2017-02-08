/**************************************************************
*	WebInterface							  				  *
*	by Daniel Wang											  *
***************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/cdefs.h>
#include <netdb.h>
#include <lib.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <fcntl.h>
#include "msg.h"

#define CONNMAX 100
#define BYTES 1024
#define CHILD_AC_ID 104


char *ROOT;
char PORT[6];
int listenfd, connfd;

int scenario_ep, tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, vpm_ep;
int ep_array[5] = {0};
message m;

void startServer(char *);
void respond(int);
int killep(int ep);

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

// function for initialization
void initialize(){
	int r, status;
	message mess;
	// setup tempControl endpoint
	tempCnt_ep = getendpoint_name("tempControl");
	tempSen_ep = getendpoint_name("tempSensor");
	heatAct_ep = getendpoint_name("heatActuator");
	alarmAct_ep = getendpoint_name("alarmActuator");
	alarmAct_ep = getendpoint_name("alarmActuator");
	vpm_ep = getendpoint_name("virtualPM");
}

// spoofing
void spoofing(void){
	printf("Start Spoofing !!!!!!!!!!!!!!\n");
}

// simulate kill()
int killep(int ep){
	int status;
	message mess;
	memset(&mess, 0, sizeof(mess));
	mess.m_type = VPM_DOKILL;
	mess.m_m1.m1i1 = ep;
	status = ipc_sendrec(vpm_ep, &mess);
	if(status != 0){
		return status;
	}

	return mess.m_m1.m1i1;

}

// killing
void killing(void){
	int r;
	printf("Start Killing %d !!!!!!!!!!!!!!\n", heatAct_ep);
	r = killep(heatAct_ep);
}


// send children process's endpoints
int sendEpTempControl(){
	memset(&m, 0, sizeof(m));
	m.m_type = WEB_EP_UPDATE;
	m.m_m7.m7i1 = ep_array[0];
	m.m_m7.m7i2 = ep_array[1];
	m.m_m7.m7i3 = ep_array[2];
	m.m_m7.m7i4 = ep_array[3];
	m.m_m7.m7i5 = ep_array[4];

	printf("WebInterface: sendEPUpdate: m_type: %d, value: %d, %d, %d, %d, %d\n", m.m_type, m.m_m7.m7i1, m.m_m7.m7i2, m.m_m7.m7i3, m.m_m7.m7i4, m.m_m7.m7i5);

	ipc_sendrec(tempCnt_ep, &m);
	if(m.m_type == WEB_CONFIRM)
		return 0;
	else
		return 1;
}

/**************************************************************
*	main function							 				  *
***************************************************************/
void main(int argc, char **argv){
	// webserver
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char c;

	ROOT = getenv("PWD");
	strcpy(PORT, "10000");

	int i = 0;
	int slot = 0;

	printf("WebInterface: webserver started at port no. %s%s%s with root directory as %s%s%s\n", "\033[92m",PORT,"\033[0m","\033[92m",ROOT,"\033[0m");

	initialize();
	printf("WEB: tempCnt_ep: %d\n", tempCnt_ep);

	startServer(PORT);

	for(i = 0; i < 5; i++){
		int pid = fork2(CHILD_AC_ID);
		if(pid == 0){
			// child process
			while(1){
		        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);
		        if(connfd == -1)
		        	continue;
		        respond(connfd);
		        close(connfd);
    		}
		}else if(pid > 0){
			// parent process
			ep_array[i] = getendpoint(pid);
			printf("child pid is %d\n", pid);
		}else{
			bail("fork2()");
		}
	}

	// tell tempControl its children process's endpoints
	if(sendEpTempControl()){
		bail("sendEpTempControl()");
	}

	while(1){
		addrlen = sizeof(clientaddr);
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);
        if(connfd == -1)
        	continue;
        respond(connfd);
        close(connfd);
    }
    exit(0);
}


// send new setpoint to tempControl process
void sendSetpointUpdate(int newsetpoint){
        m.m_type = SETPOINT_UPDATE;
        m.m_m1.m1i1 = newsetpoint;
        printf("WEB: sendTEMPCONTROL: m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);
        ipc_sendrec(tempCnt_ep, &m);
}


// start server
void startServer(char *port){
	struct addrinfo hints, *res, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if(getaddrinfo(NULL, port, &hints, &res) != 0){
		bail("getaddrinfo()");
	}

	for(p = res; p != NULL; p = p->ai_next){
		listenfd = socket(p->ai_family, p->ai_socktype, 0);
		if(listenfd == -1) continue;
		if(bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
	}

	if(p == NULL){
		bail("socket() or bind()");
	}
	freeaddrinfo(res);

	if (listen(listenfd, 99999) != 0){
        bail("listen() error");
    }
}

// handle HTTP GET request
void handleGet(int n){
	char *reqline[2], path[99999], data_to_send[BYTES];
	int fd, bytes_read;
	pid_t pid = 0;

	pid = getpid();

	reqline[0] = strtok(NULL, " \t");
	reqline[1] = strtok(NULL, " \t\n");
	if(strncmp(reqline[1], "HTTP/1.0", 8) != 0 && strncmp(reqline[1], "HTTP/1.1", 8) != 0){
		write(n, "HTTP/1.0 400 Bad Request\n", 25);
	}else{
		if(strncmp(reqline[0], "/\0", 2) == 0)
			reqline[0] = "/index.html";
		
		strcpy(path, ROOT);
		strcpy(&path[strlen(ROOT)], reqline[0]);
		printf("%d: file: %s\n", pid, path);

		if((fd = open(path, O_RDONLY)) != -1){
			send(n, "HTTP/1.0 200 OK\n\n", 17, 0);

			while((bytes_read = read(fd, data_to_send, BYTES)) >0){
				write(n, data_to_send, bytes_read);
			}
		}else{
			write(n, "HTTP/1.0 404 Not Found\n", 23);
		}
	}
}

// handle HTTP POST request
void handlePost(int n){
	char *reqline[2], path[99999], data_to_send[BYTES];
	int fd, bytes_read;
	pid_t pid = 0;
	int newsetpoint = 0;

	pid = getpid();

	do{
		reqline[0] = strtok(NULL, "\r\n\r\n");
	}while(strncmp(reqline[0], "new_setpoint", 12) != 0);

	reqline[1] = strsep(&reqline[0], "=");
	newsetpoint = atoi(reqline[0]);
	// normal situation
    if(newsetpoint != 0){
    	printf("%d: newsetpoint = %d\n", pid, newsetpoint);

    	reqline[1] = "/index.html";
    	
    	sendSetpointUpdate(newsetpoint);

    }else if(newsetpoint == 0){
    	// simulate attack
    	printf("!!!!!!!!!!!!!! %s !!!!!!!!!\n", reqline[0]);
    	if(strncmp(reqline[0], "spoofing", 8) == 0){
            spoofing();
        }else if(strncmp(reqline[0], "killing", 7) == 0){
            killing();
        }
        reqline[1] = "/attack.html";
    }
    strcpy(path, ROOT);
	strcpy(&path[strlen(ROOT)], reqline[1]);
	printf("%d: file: %s\n", pid, path);

	if((fd=open(path, O_RDONLY)) != -1){
		send(n, "HTTP/1.0 200 OK\n\n", 17, 0);
		while((bytes_read=read(fd, data_to_send, BYTES)) > 0)
        	write (n, data_to_send, bytes_read);
	}else{
    	write(n, "HTTP/1.0 404 Not Found\n", 23);
	}
}

// client connection
void respond(int n){

	char mesg[99999], *reqline[3];
	int rcvd;

	int pid = getpid();

	memset((void*)mesg, (int)'\0', 99999);

	rcvd = recv(n, mesg, 99999, 0);

	if(rcvd < 0){
		fprintf(stderr, "%d: recv() error\n", pid);
	}else if(rcvd == 0){
		fprintf(stderr, "%d: Client Disconnected Unexpectedly.\n", pid);
	}else{
		printf("%d: %s\n", pid, mesg);
		reqline[0] = strtok(mesg, " \t\n");
		if(strncmp(reqline[0], "GET\0", 4) == 0){
			handleGet(n);
		}else if(strncmp(reqline[0], "POST\0", 5)==0){
			handlePost(n);
        }
	}
}
