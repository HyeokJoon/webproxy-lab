
/* csapp.h에 선언된 모든 함수를 구현*/
#include "csapp.h"


void unix_error(char *msg){
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
void gai_error(int code, char *msg) /* Getaddrinfo-style error */
{
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
    exit(0);
}
void app_error(char *msg) /* Application error */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

/* 서버의 소켓 오픈 */
int open_listenfd(char *port){
    fprintf(stdout,"1");
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    /* 바인딩 될 수 있는 주소정보를 반환, 포트번호로만 반환, 로컬 호스트와 같은 프로토콜을 사용하는 주소만 반환 */
    hints.ai_flags = AI_PASSIVE|AI_ADDRCONFIG|AI_NUMERICSERV;

    /**
     * getaddrinfo()함수의 도메인 매개변수에 NULL을 넣는 이유
     * 1. 로컬 호스트에 대한 주소 정보만 사용할 때
     * 2. 로컬 호스트의 모든 IP주소를 허용하는 경우 (ifconfig로 확인되는 IP주소를 사용하는 모든 호스트는 접근 가능)
    */
    if((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0){
        fprintf(stderr, "getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
        return -2;
    }

    for(p= listp; p; p= p->ai_next){
        if((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;
        /**
         * 소켓에 옵션을 지정
         * SO_REUSEADDR : 소켓에 바인딩된 주소와 포트가 이미 다른 소켓에 의해 사용중일 때도 소켓을 바인딩 할 수 있다. 
         * 
         * 사용 이유 :
         * 1. 서버가 비정상적으로 종료되거나 소켓이 바인딩된 후 즉시 다시 바인딩 해야 하는 경우에 사용
         *    서버가 재시작 되었을 때 클라이언트와 연결을 놓지지 않도록 한다.
         * 2. 멀티캐스트, 브로드캐스트 소켓에서 사용할 때 같은 주소와 포트에 바인딩 되어야 하는 상황이 발생할 수 있다.
         * 3. 테스트 및 디버깅 목적
         * 
         * SO_RCVBUF : 수신버퍼 크기 조절
         * SO_SCVBUF : 송신버퍼 크기 조절
         * */
        fprintf(stdout,"0");
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
        /**
         * bind()함수 : IP주소와 Port번호를 바인딩 
         * + 바인딩된 정보는 운영체제 내부 데이터 구조에 저장된다.
         * + 데이터구조는 네트워크 스택에서 관리한다.
         * argv : 소켓 디스크립터, 바인딩할 소켓주소(IP, Port가 들어있음), 주소정보크기
        */
        if(bind(listenfd, p->ai_addr, p->ai_addrlen) == 0){
            fprintf(stdout, "bind success\n");
            break;
        }
        if(close(listenfd < 0)){
            fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }
    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* No address worked */
        return -1;
    
    /**
     * listen()함수 : 수신 대기열(큐)를 네트워크 스택에 만들어 저장한다.
     * argv : 소켓 디스크립터, 대기열의 크기
     * 성공하면 0, 실패하면 -1/errno 반환
    */
    if(listen(listenfd, LISTENQ) < 0){
        close(listenfd);
        return -1;
    }
    return listenfd;
}

/* open_listenfd 의 wrapper함수 유효성 검사 */
int Open_listenfd(char *port){
    int rc;
    printf("%s\n", port);
    /* open_listenfd 호출에 문제가 있을 때 unix_error 호출 */
    if ((rc = open_listenfd(port)) < 0)
	    unix_error("Open_listenfd error");
    return rc;
}

/* 재진입성을 고려한 클라이언트 소켓 오픈
 * hostname : 도메인 이름, port : 포트번호
 * 재진입이 가능한 루틴은 동시에 접근해도 언제나 같은 실행 결과를 보장한다. 

    1. 정적 (전역) 변수를 사용하면 안 된다.
    2. 정적 (전역) 변수의 주소를 반환하면 안 된다.
    3. 호출자가 호출 시 제공한 매개변수만으로 동작해야 한다.
    4. 싱글턴 객체의 잠금에 의존하면 안 된다.
    5. 다른 비-재진입 함수를 호출하면 안 된다.
 */
int open_clientfd(char *hostname, char *port){
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; //TCP 소켓만 반환하겠다
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG; //포트번호로만 반환, 로컬 호스트와 같은 프로토콜을 사용하는 주소만 반환
    
    /* getaddrinfo() : IPv4, IPv6 소켓 주소를 찾기 위한 함수
     * argv: 도메인 이름(IP), 포트번호(서비스 이름), 힌트(flag조건), 반환리스트
     * 반환 : 정상 동작하면 0을 반환
    */
    if((rc = getaddrinfo(hostname, port, &hints, &listp) !=0)){
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        return -2;
    }

    /* 리스트를 돌면서 내가 찾는 (IP, port) 연결을 찾음
     * flags에 맞는 address로 연결시도
     */
    for(p = listp; p; p = p->ai_next){
        /* socket()
         * 소켓 디스크립터 생성
         * argv : 도메인 (AF_INET:"32bit인터넷이다"라는 의미), 소켓 타입(SOCK_STREAM), 프로토콜(TCP, UDP)
        */
        if((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        /**
         * connect()
         * 연결 확인
         * argv : 소켓 디스크립터, 서버의 소켓 주소, 서버 소켓주소 길이
        */
        if((connect(clientfd, p->ai_addr, p->ai_addrlen)) != -1)
            break;

        /* 파일 닫기(close) : unistd.h에 정의되어 있음
         * 에러 출력(strerror) : string.h에 정의되어 있음
         */
        if(close(clientfd) < 0){
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1;
        }
    }

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* All connects failed */
        return -1;
    else    /* The last connect succeeded */
        return clientfd;
    
}

/* open_clientfd 의 wrapper함수 유효성 검사 */
int Open_clientfd(char *hostname, char *port){
    int rc;
    if((rc = open_clientfd(hostname, port)) < 0){
        unix_error("Open_clientfd error");
    }
    return rc;
}

/* Robust I/O 함수 */
void rio_readinitb(rio_t* rio, int clientfd){
    rio->rio_fd = clientfd;
    rio->rio_cnt = 0;
    rio->rio_bufptr = rio->rio_buf;
}
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* Refill if buf is empty */
	rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, 
			   sizeof(rp->rio_buf));
	if (rp->rio_cnt < 0) {
	    if (errno != EINTR) /* Interrupted by sig handler return */
		return -1;
	}
	else if (rp->rio_cnt == 0)  /* EOF */
	    return 0;
	else 
	    rp->rio_bufptr = rp->rio_buf; /* Reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;          
    if (rp->rio_cnt < n)   
	cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

ssize_t rio_writen(int clientfd, char* buf, size_t len){
    // 버퍼에 남은 문자 수
    size_t nleft = len;
    ssize_t nwritten;
    // 버퍼 포인터 지정
    char *bufp = buf;
    
    // 남은 문자수가 0보다 클때
    while(nleft > 0){
        /* fd에 bufp에 있는 내용을 nleft만큼 적는다. */
        if((nwritten = write(clientfd, bufp, nleft)) <= 0){
            /* 인터럽트 시스템콜이 발생할 때 */
            if(errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        /* 쓴 데이터만큼 이동 */
        nleft -= nwritten;
        bufp += nwritten;
    }
    return len;
}
/*  */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) { 
        if ((rc = rio_read(rp, &c, 1)) == 1) {
	    *bufp++ = c;
	    if (c == '\n') {
                n++;
     		break;
            }
	} else if (rc == 0) {
	    if (n == 1)
		return 0; /* EOF, no data read */
	    else
		break;    /* EOF, some data was read */
	} else
	    return -1;	  /* Error */
    }
    *bufp = 0;
    return n-1;
}


/* rio_readinit 의 wrapper함수 유효성 검사 */
void Rio_readinitb(rio_t* rio, int clientfd){
    rio_readinitb(rio, clientfd);
}
void Rio_writen(int clientfd, char* buf, size_t len){
    // 쓴 길이가 요청한 길이가 아니면 error
    if(rio_writen(clientfd, buf, len) != len)
        unix_error("Rio_writen error");
}

/* rio_readlineb의 wrapper함수 : */
ssize_t Rio_readlineb(rio_t* rio, void *buf, size_t bufsize){
    ssize_t rc;

    if ((rc = rio_readlineb(rio, buf, bufsize)) < 0)
	    unix_error("Rio_readlineb error");
    return rc;
}

/* stdio wrapper함수 유효성 검사 */
char *Fgets(char *ptr, int n, FILE *stream) 
{
    char *rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
	app_error("Fgets error");

    return rptr;
}

void Fputs(const char *ptr, FILE *stream) 
{
    if (fputs(ptr, stream) == EOF)
	unix_error("Fputs error");
}

/* Unix I/O wrapper함수 유효성 검사*/
void Close(int fd){
    int rc;

    if ((rc = close(fd)) < 0)
	    unix_error("Close error");
}

/**
 * accept()함수 : 서버 소켓에서 클라이언트의 연결요청을 받아들여 연결을 수립하는 역할
 * 클라이언트와 통신하기 위한 새로운 소켓(!= 서버소켓)을 만들어 반환한다.
*/
int Accept(int listenfd, struct sockaddr *clientaddr, socklen_t *clientlen){
    int rc;
    if ((rc = accept(listenfd, clientaddr, clientlen)) < 0)
        unix_error("Accept error");
    return rc;
}

void Getnameinfo(struct sockaddr *clientaddr, socklen_t clientlen, char *client_hostname, size_t hostlen, char *client_port, size_t servlen, int flags){
    int rc;
    if((rc = getnameinfo(clientaddr, clientlen, client_hostname, hostlen,
                         client_port, servlen, flags)) !=0)
        gai_error(rc, "Getnameinfo error");
}

