#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define VERSION 1
#define BUFSIZE 8096
#define ERROR      42
#define LOG        44

/*
 * Based on the super-minimal **nweb** web server from IBM.
 *
 * https://www.ibm.com/developerworks/systems/library/es-nweb/index.html
 *
 */

/* this is a child web server process, so we can exit on errors */
void web(int fd, int hit)
{
	int j, file_fd, buflen, retlen;
	long i, ret, len;
	static char request[BUFSIZE+1]; /* static so zero filled */
	static char retval[BUFSIZE+1]; /* static so zero filled */

	ret =read(fd,request,BUFSIZE); 	/* read Web request in one go */
	if(ret == 0 || ret == -1) {	/* read failure stop now */
		//logger(FORBIDDEN,"failed to read browser request","",fd);
	}
	if(ret > 0 && ret < BUFSIZE)	/* return code is valid chars */
		request[ret]=0;		/* terminate the request */
	else request[0]=0;
	for(i=0;i<ret;i++)	/* remove CF and LF characters */
		if(request[i] == '\r' || request[i] == '\n')
			request[i]='*';
	//logger(LOG,"request",request,hit);
	if( strncmp(request,"GET ",4) && strncmp(request,"get ",4) ) {
		//logger(FORBIDDEN,"Only simple GET operation supported",request,fd);
	}

	for(i=4;i<BUFSIZE;i++) { /* null terminate after the second space to ignore extra stuff */
		if(request[i] == ' ') { /* string is "GET URL " +lots of other stuff */
			request[i] = 0;
			break;
		}
	}

	if( !strncmp(&request[0],"GET /status",11) || !strncmp(&request[0],"get /status",11) ) {
        /* convert no filename to index file */
		(void)strcpy(retval,"ok");
    }

	if( !strncmp(&request[0],"GET /\0",6) || !strncmp(&request[0],"get /\0",6) ) {
        /* convert no filename to index file */
		(void)strcpy(retval,"hello");
    }


	/* work out the file type and check we support it */
	retlen=strlen(retval);

    if (retlen>0) {

        (void)printf("Request: %s  Response: %s\n", request, retval);

        /* build the entire response */
        (void)sprintf(request,"HTTP/1.1 200 OK\nServer: statusServer/%d.0\nContent-Length: %d\nConnection: close\nContent-Type: text/html\n\n%s", VERSION, retlen, retval);

        /* print to the web socket */
	    (void)write(fd,request,strlen(request));

	    sleep(1);	/* allow socket to drain before signalling the socket is closed */
    }
    else {
        /* build the entire response */
        (void)sprintf(request,"HTTP/1.1 404 Not Found\nServer: statusServer/%d.0\nContent-Length: %d\nConnection: close\nContent-Type: text/html\n\n%s", VERSION, retlen, retval);

        /* print to the web socket */
	    (void)write(fd,request,strlen(request));

	    sleep(1);	/* allow socket to drain before signalling the socket is closed */
    }

	close(fd);

	exit(1);
}

int main(int argc, char **argv)
{
	int i, port=8080, pid, listenfd, socketfd, hit;

	socklen_t length;

	static struct sockaddr_in cli_addr; /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */

	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0) {
		//logger(ERROR, "system call","socket",0);
    }

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0) {
		(void)printf("ERROR: system call: bind - port %d already in use!\n", port);
        exit(1);
    }

	if(listen(listenfd,64) <0) {
		(void)printf("ERROR: system call: listen\n");
    }

    (void)printf("OK: %s (pid=%d) running on port %d\n", argv[0], getpid(), port);

    /* break away from process group */
	(void)setpgrp();

	for(hit=1; ;hit++) {
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0) {
			//logger(ERROR,"system call","accept",0);
        }
		if((pid = fork()) < 0) {
			(void)printf("Fork failed!\n");
		}
		else {
			if(pid == 0) { 	/* child */
				(void)close(listenfd);
				web(socketfd,hit); /* never returns */
			} else { 	/* parent */
				(void)close(socketfd);
			}
		}
	}
}
