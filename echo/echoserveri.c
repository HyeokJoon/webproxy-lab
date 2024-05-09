#include "csapp.h"

int main(int argc, char **argv){
    int listenfd, connfd;
    /* 소켓 주소 길이 : 32bit (unsigned int), socket.h에 정의되어 있음 */
    socklen_t clientlen;
    /* 교재의 sockaddr와 동일한 역할, sa_family를 가지고 있고 나머지는 data영역
     * IPv4, IPv6 인지 모르기 때문에 둘의 호환성을 유지하기 위해 다형성 모델 사용
     */
    struct sockaddr_storage clientaddr;
    /* 연결된 클라이언트의 정보를 저장 */
    char client_hostname[MAXLINE], client_port[MAXLINE];

    // echoserveri.c를 실행할 때, 서버의 port번호가 들어와야 한다. 첫 번째 매개변수는 항상 현재 프로세스
    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    printf("%s\n",argv[1]);

    /* port번호를 매개변수로 listen 파일 디스크립터를 생성 */
    listenfd = Open_listenfd(argv[1]);
    while(1){
        /* TODO:소켓 스토리지의 길이는 socklen_t와 같은 거 아닌가?????? */ 
        clientlen = sizeof(struct sockaddr_storage);
        /* accept하는 함수 */
        connfd = Accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        Getnameinfo((struct sockaddr *)&clientaddr, clientlen, client_hostname, MAXLINE,
                     client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd);
        Close(connfd);
    }
    exit(0);
}