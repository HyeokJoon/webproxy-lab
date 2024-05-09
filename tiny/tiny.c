/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requestheader(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void serve_header(int fd, char *filename, int filesize);
int cnt = 0;
void sig_child(int signo){
  pid_t pid;

  pid = Wait(NULL);
  printf("%d child %d terminated\n", cnt++, pid);
}


int main(int argc, char **argv) {
  /* 시그널 핸들러 (시그널, 호출함수) */
  signal(SIGCHLD, sig_child);

  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

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
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}


void doit(int fd){
  rio_t rio;
  struct stat file_stat;
  char buffer[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], filename[MAXLINE], cgiargs[MAXLINE];
  /**
   * fd에서 요청을 읽기 (Robust I/O 사용 : 버퍼를 사용한 안정적인 데이터 읽기)
   * connfd에는 method URI version 형태의 요청이 첫째줄에 들어있음
   */
  Rio_readinitb(&rio, fd);
  // Rio_readlineb(&rio, buffer, MAXLINE);
  if(!Rio_readlineb(&rio, buffer, MAXLINE)) // 무한 루프 해결...? 아마도..? 수정 by 민준
    return;
  printf("input request to buffer : %s\n", buffer);
  /**
   * sscanf()함수 : 배열에서 값을 format형태로 추출하는 함수
   * 배열에서 여러개를 출력할 수 있다는 장점이 있음
   * 일반적으로 공백 문자(스페이스, 탭, 개행 등)가 가장 일반적인 구분자로 사용
  */
  sscanf(buffer, "%s %s %s", method, uri, version);
  printf("method: %s\n", method);
  printf("uri: %s\n", uri);
  printf("version: %s\n", version);

  // fscanf("%s", method);
  // fscanf("%s", uri);
  // fscanf("%s", version);
  /**
   * strcmp(): 대소문자를 구분하지 않고 비교
   * 두 인자는 비교 전에 소문자로 변환된다.
   * method가 
   */
  if(strcmp(method, "GET") && strcmp(method, "HEAD")){
    //GET요청이 아닌 경우 서버에서 처리할 수 없음 : 501에러
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  /* 요청 헤더 읽기 TODO: 좀 더 해석필요 rio_readlineb */
  read_requestheader(&rio);

  /* uri 파싱 */
  int is_static = parse_uri(uri, filename, cgiargs);

  /**
   * stat()함수 : file 경로에 있는 file의 정보를 file_stat에 넣는다.
   * 반환 : 성공하면 0, 실패하면 -1
   * 
   *  [stat 구조체]
      st_mode: 파일의 종류와 퍼미션을 나타내는 파일 모드를 나타내는 비트 플래그입니다.
      st_size: 파일의 크기 (바이트)입니다.
      st_uid: 파일의 소유자 ID입니다.
      st_gid: 파일의 그룹 ID입니다.
      st_atime: 파일의 마지막 접근 시간입니다.
      st_mtime: 파일의 마지막 수정 시간입니다.
      st_ctime: 파일의 마지막 변경 시간입니다.
   * 
   * 
  */
  if(stat(filename, &file_stat) < 0){
    /* file경로가 잘못됐다. == uri가 잘못됐다 == 클라이언트가 경로를 잘못 지정했다 == 404오류 */
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if(!strcmp(method, "HEAD")){
    serve_header(fd, filename, file_stat.st_size);
    return;
  }
  if(is_static){
    /* 
      S_ISREG : 파일 모드가 일반 파일(텍스트파일, 실행파일 등)이면 1 아니면(디렉터리, 디바이스, FIFO, 소켓) 0 반환하는 메소드
      S_IRUSR : 읽기 권한을 나타내는 flag
    */
    if(!(S_ISREG(file_stat.st_mode)) || !(S_IRUSR & file_stat.st_mode))
    {
      /* 읽을 수 없는 파일이거나 일반 파일이 아니다 == 접근 권한이 없다 == 403오류 */
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, file_stat.st_size);
  }
  else{
    if(!(S_ISREG(file_stat.st_mode)) || !(S_IXUSR & file_stat.st_mode)){
      /* 일반파일이 아니거나 실행 권한이 없다 == 403오류 */
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }


  return;
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,char *longmsg){

  char buf[MAXLINE], body[MAXBUF];
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body backgroundcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n",body);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Conent-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

/**
 * 첫 줄이 공백 줄로 시작한다는 특성을 이용한다.
 * 헤더 
*/
void read_requestheader(rio_t *rp){
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")){
    printf("%s", buf);
    Rio_readlineb(rp, buf, MAXLINE);
  }
  printf("%s", buf);
}

/**
 * parse_uri()함수 : filename과 cgiargs를 원하는 형태로 변경한다.
 * 
 * 1. 정적 컨텐츠 일 때 : filename = 원하는 파일, cgiargs = ""
 * 2. 동적 컨텐츠 일 때 : filename = ???, cgiargs = ??
 * 반환값 : 정적 컨텐츠 이면 1, 동적이면 0
*/
int parse_uri(char *uri, char *filename, char *cgiargs){
  // fprintf("\n\n\n%s\n", uri);
  /**
   * strstr() 함수 : 인자2가 인자1의 부분문자열인지 확인하는 함수
   * 부분문자열이면 포인터를 반환, 아니면 NULL을 반환
   * cgi-bin을 사용하는 경우에는 uri에 cgi-bin이 포함되어 있음(동적 컨텐츠)
   * NULL일때는 정적 컨텐츠이다.
  */
  if (!strstr(uri, "cgi-bin")){ 
    /**
     * strcpy()함수 : 왼쪽 인자의 메모리에 오른쪽 인자의 값을 복사 
     * 반환값 : 왼쪽인자의 포인터
     * 주의 : 왼쪽 인자의 메모리 크기가 오른쪽 인자보다 커야 함(버퍼 오버플로우 방지)
     */
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    /* strcat() 함수 : 문자열을 이어붙인다. */
    strcat(filename, uri);
    /**
     * uri가 비어있으면 (/) 홈페이지를 호출하도록 만들어야 한다.
     * 일반적으로 index.html을 추가한다.
    */
    if(uri[strlen(uri)-1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  /**
   * 동적 컨텐츠
   * 여러 동적 컨텐츠를 만들어서 사용할 수 있다.
   * 여기서는 adder.c만 사용
   * */
  else{
    /* index()함수 : 문자열에서 특정 문자를 검색하는 함수
     * 반환값 : 처음 문자를 발견한 위치의 주소를 반환 없으면 NULL반환
     */
    char *ptr = index(uri, '?');
    if(ptr){
      strcpy(cgiargs, ptr+1); // ?이후의 인자 예)arg1&arg2&arg3...
      *ptr = '\0';
    }
    // '?'를 못찾았을 때 : 인자가 없는 동적 컨텐츠
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_header(int fd, char *filename, int filesize){
  char filetype[MAXLINE], buf[MAXLINE];
  printf("start serve_static\n");
  printf("file_discriptor : %d\n", fd);

  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.1 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: Close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);
}

void serve_static(int fd, char *filename, int filesize){
  /* 정적 컨텐츠 출력 */
  int srcfd;
  char *filebuffer;
  serve_header(fd, filename, filesize);

  srcfd = Open(filename, O_RDONLY, 0);
  /**
   * mmap()함수 : 메모리 할당 후 포인터를 반환하는 함수
      
   * 정의 : void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)

   * arg1 : 매핑할 메모리 주소 지정 (0 or NULL일 때 커널이 적절한 주소 선택)
   * arg2 : 매핑할 영역의 길이 지정
   * arg3 : 매핑된 메모리 영역에 대한 보호 모드를 지정합니다. 읽기, 쓰기, 실행 등의 권한을 설정할 수 있습니다.
   * arg4 : 매핑 옵션을 지정합니다. 주로 MAP_SHARED, MAP_PRIVATE 등의 플래그를 사용합니다.
   * arg5 : 매핑할 파일의 파일 디스크립터를 지정합니다.
   * arg6 : 파일 내에서 매핑을 시작할 오프셋을 지정합니다.
   **/

  /* Mmap 사용 */
  // filebuffer = Mmap(0,filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  // Close(srcfd);
  // Rio_writen(fd, filebuffer, filesize);
  // Munmap(filebuffer, filesize);

  /* malloc 사용 */
  filebuffer = malloc(filesize);
  Rio_readn(srcfd, filebuffer, filesize);
  Close(srcfd);
  Rio_writen(fd, filebuffer, filesize);
  free(filebuffer);
}
/* 동적 컨텐츠 출력 */
void serve_dynamic(int fd, char *filename, char *cgiargs){
  char buf[MAXLINE], *emptylist[] = {NULL};
  printf("serve_dynamic start \n\n");
  sprintf(buf, "HTTP/1.1 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if(Fork() == 0){ // 자식프로세스 생성
    printf("fork success\n");
    printf("filename : %s\n", filename);
    printf("cgiargs : %s\n", cgiargs);
  /**
   * setenv()함수 : 환경변수를 추가 또는 수정하는 함수
   * argv : 환경변수 이름, 변수값, 이미 같은 이름의 변수가 있으면 변경할지 여부
   * 반환 : 성공하면 0 , 실패하면 -1
  */
    setenv("QUERY_STRING", cgiargs, 1);
    /* 파일 디스크립터를 1번 stdout 디스크립터를 가리키도록 함 */
    Dup2(fd, STDOUT_FILENO);
    //TODO: environ, emptylist왜 사용????
    Execve(filename, emptylist, environ);
  }
  /**
   * wait(&status)의 매개변수로 자식의 상태를 나타내는 변수를 넣을 수 있다.
   * => 자식 프로세스가 어떻게 종료됐는지에 따라서 예외처리를 할 수 있다.
  */
  // Wait(NULL);
}

/**
 * 파일 타입반환
 * text/html 등
*/
void get_filetype(char *filename, char *filetype){
  if(strstr(filename, ".html")){ //.html이 경로에 들어있으면 
    strcpy(filetype, "text/html");
  }
  else if(strstr(filename, ".gif")){
     strcpy(filetype, "image/gif");
  }
  else if(strstr(filename, ".png")){
     strcpy(filetype, "image/png");
  }
  else if(strstr(filename, ".jpg")){
     strcpy(filetype, "image/jpeg");
  }
  else if(strstr(filename, ".mp4")){
     strcpy(filetype, "video/mp4");
  }
  else
     strcpy(filetype, "text/plain");
}