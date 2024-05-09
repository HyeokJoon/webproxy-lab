
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);
void read_requestheader(rio_t *rp);
int parse_uri(char* host, char *port, char *method, char *uri, char *version, char* parse_request);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void makeheader(char *host, char *parse_header);

int main(int argc, char **argv) {

  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  socklen_t serverlen;
  struct sockaddr serveraddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  printf("\n\nlistenfd : %d\n\n", listenfd);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
  
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

void doit(int fd){

    char *host, *port, buf[MAXLINE], *method, *uri, *version;
    char parse_header[MAXLINE], parse_request[MAXLINE];
    rio_t rio;
    int n;

    Rio_readinitb(&rio, fd);
    if(!Rio_readlineb(&rio, buf, MAXLINE)) // 무한 루프 해결...? 아마도..? 수정 by 민준
      return;
    /* 요청 인자 추출 */
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("method: %s\n", method);
    printf("uri: %s\n", uri);
    printf("version: %s\n", version);

    parse_uri(host, port, method, uri, version, &parse_request);

    /* 헤더 생성 */
    makeheader(host, parse_header);
    read_requestheader(&rio);


    // 서버 소켓을 열고 전달
    int serverfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, serverfd);
 
    while((n = Rio_readlineb(&rio,buf, MAXBUF))>0){
      Rio_writen(serverfd, buf, MAXBUF);
    }
    
}
void read_requestheader(rio_t *rp){
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")){
    printf("%s", buf);
    Rio_readlineb(rp, buf, MAXLINE);
  }
  printf("%s", buf);
}


void makeheader(char *host, char* parse_header){
  char host_header[MAXLINE], connection[MAXLINE], proxy_connection[MAXLINE];
  strcpy(host_header, "\0");

  *host_header = host;
  *connection = "Connection: close";
  *proxy_connection = "Proxy-Connection: close";
}

int parse_uri(char* host, char *port, char *method, char *uri, char *version, char* parse_request){
  /* uri 변경 */
  char *p = strstr(uri, "/hub");
  if(p!=NULL){
      strncpy(host, uri, p-uri);
      strcpy(uri, p);
  }
  /* version 변경 */
  version = "HTTP/1.0";

  /* 새로운 요청 만들기 */
  parse_request = "\0";
  strcat(parse_request, method);
  strcat(parse_request, " ");
  strcat(parse_request, uri);
  strcat(parse_request, " ");
  strcat(parse_request, version);

  /* 확인 코드 */
  printf("change host : %s\n",host);
  printf("change method : %s\n",method);
  printf("change uri : %s\n",uri);
  printf("change version : %s\n\n",version);
}