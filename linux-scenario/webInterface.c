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
int listenfd;
mqd_t mqd;
Msg message;

void startServer(char *);
void respond(int);

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

void main(int argc, char **argv){
	
	int prio = 0;
	int status;
	int flag = O_WRONLY;
	ssize_t len;
	struct mq_attr attr;

	mqd = mq_open("/cnt-web", flag);
	if(mqd == (mqd_t) -1 )
		bail("web: mq_open(/cnt-web)");
	if (mq_getattr(mqd, &attr) == -1)
		bail("web: mq_getattr(mqd)");

	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char c;

	char PORT[6];
	ROOT = getenv("PWD");
	strcpy(PORT, "10000");

	int i = 0;
	int connfd;

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

// client connection
void respond(int n){

	char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999];
	int rcvd, fd, bytes_read;
	pid_t pid = 0;

	pid = getpid();

	memset( (void*)mesg, (int)'\0', 99999);

	rcvd = recv(n, mesg, 99999, 0);

	if(rcvd < 0){
		fprintf(stderr, "%d: recv() %d error\n", pid, errno);
	}else if(rcvd == 0){
		fprintf(stderr, "Client Disconnected Unexpectedly.\n");
	}else{
		printf("%d: %s\n", pid, mesg);
		reqline[0] = strtok(mesg, " \t\n");
		if(strncmp(reqline[0], "GET\0", 4) == 0){
			reqline[1] = strtok(NULL, " \t");
			reqline[2] = strtok(NULL, " \t\n");
			if(strncmp(reqline[2], "HTTP/1.0", 8) != 0 && strncmp(reqline[2], "HTTP/1.1", 8) != 0){
				write(n, "HTTP/1.0 400 Bad Request\n", 25);
			}else{
				if(strncmp(reqline[1], "/\0", 2) == 0)
					reqline[1] = "/index.html";

				strcpy(path, ROOT);
				strcpy(&path[strlen(ROOT)], reqline[1]);
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
		}else if(strncmp(reqline[0], "POST\0", 5)==0){
            int newsetpoint = 0;

            do{
                reqline[1] = strtok(NULL, "\r\n\r\n");
            }
            while(strncmp(reqline[1], "new_setpoint", 12) != 0);
        
            reqline[2] = strsep(&reqline[1], "=");

            newsetpoint = atoi(reqline[1]);

            printf("%d: newsetpoint = %d\n", pid, newsetpoint);

            reqline[1] = "/index.html";
            strcpy(path, ROOT);
            strcpy(&path[strlen(ROOT)], reqline[1]);
            printf("%d: file: %s\n", pid, path);

            if((fd=open(path, O_RDONLY)) != -1){
            	send(n, "HTTP/1.0 200 OK\n\n", 17, 0);
            	while((bytes_read=read(fd, data_to_send, BYTES)) > 0)
                        write (n, data_to_send, bytes_read);
            }
            else{
                write(n, "HTTP/1.0 404 Not Found\n", 23);
            }
            message = (Msg) {SETPOINT_UPDATE, newsetpoint};
            int status = mq_send(mqd, (const char *) &message, sizeof(message), 0);
        }
	}
}