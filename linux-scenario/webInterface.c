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

char *ROOT;
char PORT[6];
int listenfd;
mqd_t mqd;
mqd_t mqd_sw;
Msg message;
int tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid;

void startServer(char *);
void respond(int);

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

// spoofing
void spoofing(void){
	printf("Start Spoofing !!!!!!!!!!!!!!\n");
}

// killing
void killing(void){
	printf("Start Killing %d!!!!!!!!!!!!!!\n", heatAct_pid);
	kill(heatAct_pid, SIGKILL);
}

void receivePid(void){
	int len;
	unsigned int prio;
	int flag = O_RDWR;
	struct mq_attr attr;

	mqd_sw = mq_open("/sce-web", flag);
	if(mqd == (mqd_t) -1 )
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

	ROOT = getenv("PWD");
	strcpy(PORT, "10000");

	printf("webInterface is loaded\n");

	startServer(PORT);
	signal(SIGPIPE, SIG_IGN);

	for(i = 0; i < 5; i++){
		int pid = fork();
		if(pid == 0){
			//child
			while(1){
				addrlen = sizeof(clientaddr);
				connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);
				if(connfd == -1)
        			continue;
				respond(connfd);
				close(connfd);
			}
		}else if(pid > 0){
			//parent
			printf("child pid is %d\n", pid);
		}else{
			bail("web: fork()");
		}
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
	int rcvd, pid;

	pid = getpid();
	memset((void*)mesg, (int)'\0', 99999);

	rcvd = recv(n, mesg, 99999, 0);

	if(rcvd < 0){
		fprintf(stderr, "%d: recv() %d error\n", pid, errno);
	}else if(rcvd == 0){
		fprintf(stderr, "Client Disconnected Unexpectedly.\n");
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