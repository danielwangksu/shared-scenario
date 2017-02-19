/**************************************************************
*	Temperature Sensor Program								  *
*	by Daniel Wang											  *
***************************************************************/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include "msg.h"

#define CONNMAX 5
#define BYTES 1024

#define ON 1
#define OK 0
#define OFF 0

char *ROOT;
char PORT[6];
int listenfd;
mqd_t mqd;
mqd_t mqd_sw;
mqd_t mqd_spoof_fan;
mqd_t mqd_spoof_alarm;
Msg message;
int tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid;
static volatile int keepRunning = 1;
int pid[5];

void startServer(char *);
void respond(int);

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

void spoofingFan(int command){
	int status, len, prio;
	Msg message_spoofing;
	message_spoofing = (Msg) {HEATER_COMMAND, command};
	status = mq_send(mqd_spoof_fan, (const char *) &message_spoofing, sizeof(message_spoofing), 0);
}

void spoofingAlarm(int command){
	int status, len, prio;
	Msg message_spoofing;
	message_spoofing = (Msg) {ALARM_COMMAND, command};
	status = mq_send(mqd_spoof_alarm, (const char *) &message_spoofing, sizeof(message_spoofing), 0);
}

// open spoofing mq
void open_spoof_mq(void){
	int rwflag = O_RDWR;
	struct mq_attr attr;

	mqd_spoof_fan = mq_open("/cnt-heat", rwflag);
	if(mqd_spoof_fan == (mqd_t) -1 )
		bail("mq_open(/cnt-heat)");
	if (mq_getattr(mqd_spoof_fan, &attr) == -1)
		bail("mq_getattr(mqd_spoof_fan)");

	mqd_spoof_alarm = mq_open("/cnt-alarm", rwflag);
	if(mqd_spoof_alarm == (mqd_t) -1 )
		bail("mq_open(/cnt-alarm)");
	if (mq_getattr(mqd_spoof_alarm, &attr) == -1)
		bail("mq_getattr(mqd_spoof_alarm)");
}

// spoofing
void spoofing(void){
	int i = 100;
	printf("Start Spoofing !!!!!!!!!!!!!!\n");
	open_spoof_mq();
	while(i >= 1){
		spoofingFan(OFF);
		spoofingAlarm(OFF);
		sleep(1);
	}
}

// killing
void killing(void){
	printf("Start Killing %d!!!!!!!!!!!!!!\n", heatAct_pid);
	kill(heatAct_pid, SIGKILL);
	kill(alarmAct_pid, SIGKILL);
}

void receivePid(void){
	int len;
	unsigned int prio;
	int flag = O_RDWR;
	struct mq_attr attr;

	mqd_sw = mq_open("/sce-web", flag);
	if(mqd_sw == (mqd_t) -1 )
		bail("web: mq_open(/sce-web)");
	if (mq_getattr(mqd_sw, &attr) == -1)
		bail("web: mq_getattr(mqd_sw)");

	len = mq_receive(mqd_sw, (char *) &message, attr.mq_msgsize, &prio);
	if(len == -1)
		bail("web: mq_receive(mqd_sw)");
	tempSen_pid = message.data;

	len = mq_receive(mqd_sw, (char *) &message, attr.mq_msgsize, &prio);
	if(len == -1)
		bail("web: mq_receive(mqd_sw)");
	tempCnt_pid = message.data;

	len = mq_receive(mqd_sw, (char *) &message, attr.mq_msgsize, &prio);
	if(len == -1)
		bail("web: mq_receive(mqd_sw)");
	heatAct_pid = message.data;

	len = mq_receive(mqd_sw, (char *) &message, attr.mq_msgsize, &prio);
	if(len == -1)
		bail("web: mq_receive(mqd_sw)");
	alarmAct_pid = message.data;

	mq_close(mqd_sw);
}

void intHandler(int dummy){
	int i, status;

	printf("WEB: got the interrupt\n");
	status = mq_close(mqd);
	status = mq_close(mqd_spoof_fan);
	status = mq_close(mqd_spoof_alarm);
	for(i = 0; i < 5; i++){
		kill(pid[i], SIGKILL);
	}
	exit(0);
}

void childIntHandler(int dummy){
	int i, status;

	printf("WEB: got the interrupt\n");
	status = mq_close(mqd);
	status = mq_close(mqd_spoof_fan);
	status = mq_close(mqd_spoof_alarm);
	exit(0);
}

void main(int argc, char **argv){
	
	int prio = 0;
	int status;
	int flag = O_WRONLY;
	ssize_t len;
	struct mq_attr attr;

	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char c;

	int i = 0;
	int connfd;

	mqd = mq_open("/cnt-web", flag);
	if(mqd == (mqd_t) -1 )
		bail("web: mq_open(/cnt-web)");
	if (mq_getattr(mqd, &attr) == -1)
		bail("web: mq_getattr(mqd)");

	receivePid();
	signal(SIGINT, intHandler);

	ROOT = getenv("PWD");
	strcpy(PORT, "10000");

	printf("webInterface is loaded\n");

	startServer(PORT);
	signal(SIGPIPE, SIG_IGN);

	for(i = 0; i < 5; i++){
		pid[i] = fork();
		if(pid[i] == 0){
			//child
			signal(SIGINT, childIntHandler);
			while(1){
				addrlen = sizeof(clientaddr);
				connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);
				if(connfd == -1)
        			continue;
				respond(connfd);
				close(connfd);
			}
			exit(0);
		}else if(pid[i] > 0){
			//parent
			printf("child pid is %d\n", pid[i]);
		}else{
			bail("web: fork()");
		}
	}

	int t_pid, waitstatus;
	t_pid = wait(&waitstatus);
	if(t_pid > 0){
		for(i = 0; i < 5; i++){
			kill(pid[i], SIGKILL);
		}
		exit(0);
	}
}

// start server
void startServer(char *port){
	struct addrinfo hints, *res, *p;
	int option = 1;

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
		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
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

// handle GET request
void handleGet(int n){
	char *reqline[2], data_to_send[BYTES], path[99999];
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
		// printf("%d: file: %s\n", pid, path);

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

// handle POST request
void handlePost(int n){
	char *reqline[2], data_to_send[BYTES], path[99999];
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
    	
    	message = (Msg) {SETPOINT_UPDATE, newsetpoint};
    	int status = mq_send(mqd, (const char *) &message, sizeof(message), 0);
    }else if(newsetpoint == 0){
    	// simulate attack
    	if(strncmp(reqline[0], "spoofing", 8) == 0){
            spoofing();
        }else if(strncmp(reqline[0], "killing", 7) == 0){
            killing();
        }
        reqline[1] = "/attack.html";
    }
    strcpy(path, ROOT);
	strcpy(&path[strlen(ROOT)], reqline[1]);
	// printf("%d: file: %s\n", pid, path);

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
	int rcvd, pid;

	pid = getpid();
	memset((void*)mesg, (int)'\0', 99999);

	rcvd = recv(n, mesg, 99999, 0);

	if(rcvd < 0){
		fprintf(stderr, "%d: recv() %d error\n", pid, errno);
	}else if(rcvd == 0){
		fprintf(stderr, "Client Disconnected Unexpectedly.\n");
	}else{
		// printf("%d: %s\n", pid, mesg);
		reqline[0] = strtok(mesg, " \t\n");
		if(strncmp(reqline[0], "GET\0", 4) == 0){
			handleGet(n);
		}else if(strncmp(reqline[0], "POST\0", 5)==0){
			handlePost(n);
		}
	}
}
