#include "csapp.h"

/**
 * echo()함수 : 
 * 1. 소켓을 통해 데이터를 버퍼로 읽는다.
 * 2. 읽은 데이터의 크기를 stdin으로 출력한다.
 * 3. 버퍼의 데이터를 다시 소켓을 통해 전송한다.
*/
void echo(int connfd){
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, buf, MAXLINE))!=0){
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}