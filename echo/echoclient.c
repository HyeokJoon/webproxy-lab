#include "csapp.h"

/*
 * 서버에 요청을 보내고 서버의 응답을 출력하는 함수
 * argc : 매개변수 개수
 * argv : 매개변수의 배열
*/
int main(int argc, char **argv){
    // 클라이언트의 소켓 파일 디스크립터
    int clientfd;
    // 호스트의 IP주소, 포트번호, 버퍼
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    // echoclient.c를 실행할 때, 매개변수로 host와 port번호가 들어와야 한다. 첫 번째 매개변수는 항상 현재 프로세스
    if(argc !=3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0); // 프로세스 종료
    }

    // 입력된 매개변수
    host = argv[1];
    port = argv[2];

    // 클라이언트 소켓을 열고 read버퍼에 넣음
    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);           

    // stdin파일에서 한줄씩 버퍼로 읽고, 읽은 만큼 clientfd에 쓰기 //TODO: 완벽하게 해석하기
    while(Fgets(buf, MAXLINE, stdin)!=NULL){ 
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}