
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

#define CONNMAX 100
#define BYTES 1024
#define CHILD_IPC_ID 44


char *ROOT;
int listenfd, connfd;

int tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid;
int tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, web_ep;
message m;

void startServer(char *);
void respond(int);

// fault handler
static void bail(const char *on_what) {
	perror(on_what);
	exit(1); 
}

void main(int argc, char **argv){
	// webserver
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char c;

	char PORT[6];
	ROOT = getenv("PWD");
	strcpy(PORT, "10000");

	int i = 0;
	int slot = 0;

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

	printf("WebInterface: tempSen_pid=%d, tempCnt_pid=%d, heatAct_pid=%d, alarmAct_pid=%d,web_pid=%d, tempSen_ep=%d, tempCnt_ep=%d, heatAct_ep=%d, alarmAct_ep=%d, web_ep=%d\n", tempSen_pid, tempCnt_pid, heatAct_pid, alarmAct_pid, web_pid, tempSen_ep, tempCnt_ep, heatAct_ep, alarmAct_ep, web_ep);

	printf("WebInterface: webserver started at port no. %s%s%s with root directory as %s%s%s\n", "\033[92m",PORT,"\033[0m","\033[92m",ROOT,"\033[0m");

	startServer(PORT);

	for(i = 0; i < 5; i++){
		int pid = fork2(CHILD_IPC_ID);
		if(pid == 0){
			// child
			while(1){
		        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);
		        if(connfd == -1)
		        	continue;
		        respond(connfd);
		        close(connfd);
    		}
		}else if(pid > 0){
			// parent
			printf("child pid is %d\n", pid);
		}else{
			bail("fork2()");
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

	int pid = getpid();

	memset( (void*)mesg, (int)'\0', 99999);

	rcvd = recv(n, mesg, 99999, 0);

	if(rcvd < 0){
		fprintf(stderr, "%d: recv() error\n", pid);
	}else if(rcvd == 0){
		fprintf(stderr, "%d: Client Disconnected Unexpectedly.\n", pid);
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
				printf("file: %s\n", path);

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

            m.m_type = newsetpoint;
            ipc_sendrec(tempCnt_ep, &m);

            if((fd=open(path, O_RDONLY)) != -1){
            	send(n, "HTTP/1.0 200 OK\n\n", 17, 0);
            	while((bytes_read=read(fd, data_to_send, BYTES)) > 0)
                        write (n, data_to_send, bytes_read);
            }
            else{
                write(n, "HTTP/1.0 404 Not Found\n", 23);
            }
        }
	}
}
