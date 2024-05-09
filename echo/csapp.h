
#ifndef __CSAPP_H__
#define __CSAPP_H__

#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> //implicit declaration of function 'exit' 오류
// #include <stdio.h>
// #include <stdlib.h>
// #include <stdarg.h>
// #include <unistd.h>
// #include <string.h>
// #include <ctype.h>
// #include <setjmp.h>
// #include <signal.h>
// #include <dirent.h>
// #include <sys/time.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <sys/mman.h>
// #include <errno.h>
// #include <math.h>
// #include <pthread.h>
// #include <semaphore.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>

#define MAXLINE 8192
#define RIO_BUFSIZE 8192
#define LISTENQ  1024

typedef struct sockaddr SA;
typedef __SIZE_TYPE__ size_t; // stddef.h에 있음
typedef struct {
    int rio_fd;                /* Descriptor for this internal buf */
    int rio_cnt;               /* Unread bytes in internal buf */
    char *rio_bufptr;          /* Next unread byte in internal buf */
    char rio_buf[RIO_BUFSIZE]; /* Internal buffer */
}rio_t;

/* 재진입성을 고려한 클라이언트 소켓 오픈 */
int open_clientfd(char *hostname, char *port);

/* open_clientfd 의 wrapper함수 유효성 검사 */
int Open_clientfd(char *hostname, char *port);

/* 서버의 소켓 오픈 */
int open_listenfd(char *port);

/* open_listenfd 의 wrapper함수 유효성 검사 */
int Open_listenfd(char *port);

/* Robust I/O 함수 */
void rio_readinitb(rio_t* rio, int clientfd);
ssize_t rio_writen(int clientfd, char* buf, size_t len);

/* rio_readinit 의 wrapper함수 유효성 검사 */
void Rio_readinitb(rio_t* rio, int clientfd);
ssize_t Rio_readlineb(rio_t* rio, void *buf, size_t bufsize);
void Rio_writen(int clientfd, char* buf, size_t len);

/* stdio wrapper함수 유효성 검사 */
char *Fgets(char *ptr, int n, FILE *stream);
void Fputs(const char *ptr, FILE *stream);

/* Unix I/O wrapper함수 유효성 검사*/
void Close(int fd);

/* echo 함수 */
void echo(int connfd);

/* accept()의 wrapper함수 accept()는 socket.h에 정의되어 있음 */
int Accept(int listenfd, struct sockaddr *clientaddr, socklen_t *clientlen);

void Getnameinfo(struct sockaddr *clientaddr, socklen_t clientlen, char *client_hostname, size_t hostlen, char *client_port, size_t servlen, int flags);

#endif