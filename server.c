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
#include <sys/stat.h>
#include<pthread.h>
#define BUFSIZE 1024

int sum(char *cgiaddr);
void log(char *ip, char *fname, int fsize);
void *client_thread(void *n1);

pthread_mutex_t m_lock; //동기화문제 위한 mutex변수 정의

//클라이언트 연결정보 구조체
typedef struct num{
	int *nn; //클라이언트 소켓 변수
	char *buff; //연결된 클라이언트 ip 저장하는 변수
}num2;
//파일 확장자 정보 구조체
struct {
   char *ext; 
   char *filetype; 
} extensions [] = {
   {"gif", "image/gif" },
   {"jpg", "image/jpg"},
   {"jpeg","image/jpeg"},
   {"htm", "text/html" },  
   {"html","text/html" },
   {0,0} };

char path[BUFSIZE]; //로그파일 경로 저장할 변수

int main(int argc, char *argv[])
{
    int fd; //로그 변수
    fd = open("./log.txt", O_CREAT | O_TRUNC, 0644); //로그파일 열기
    close(fd); //파일 닫기

    pthread_t tid;		    //쓰레드 변수
    num2 *n1;			    //클라이언트 구조체 변수
	int i = 0;	    //반복문 변수
    struct sockaddr_in sin,cli;	    //소켓 구조체 생성
    int sd,clientlen=sizeof(cli);   //소켓 번호, 클라이언트 len저장 변수

    char *cwd;		    //현재 경로 저장할 변수
    cwd = getcwd(NULL, BUFSIZE); //현재 경로 저장
    strcat(cwd, "/log.txt"); //현재 경로+로그파일 로그파일 기록할 경로 지정
    strcpy(path, cwd); //log함수에서 쓰기 위해 전역변수에 저장


    if(chdir(argv[1]) == -1){ //입력 경로로 이동
    	printf("path error"); //실패 시 오류메세지 출력
    }
    if((sd=socket(AF_INET,SOCK_STREAM,0)) == -1){   //소켓 생성
    	printf("socket() error");		    //실패 시 오류메세지 출력
    }
    
    //소켓 구조체 정보 저장
    memset((char*)&sin,'\0',sizeof(sin));	//초기화
    sin.sin_family=AF_INET;			//인터넷 소켓 지정
    sin.sin_port=htons(atoi(argv[2]));		//포트번호 저장
    sin.sin_addr.s_addr=inet_addr("0.0.0.0");	//클라이언트 주소 저장

    if(bind(sd,(struct sockaddr *)&sin,sizeof(sin)) == -1){ //sd에 정보 bind
    	perror("bind() error"); //bind실패 시 오류메세지 출력
    }
    if(listen(sd,100) == -1){	    //클라이언트 요청 대기
    	perror("listen() error"); //실패 시 오류메세지 출력
    }

    while(1){
	pthread_mutex_init(&m_lock, NULL); //mutex변수 초기화 시켜주기
	
	n1=(num2*)malloc(sizeof(num2)); //동적 할당
	if((n1->nn=accept(sd,(struct sockaddr *)&cli,&clientlen)) == -1){ //연결요청 수락
	    perror("accept() error"); //실패 시 오류메세지 출력
	}	
	n1->buff=inet_ntoa(cli.sin_addr); //현재 연결된 클라이언트 주소 구조체에 저장
	pthread_create(&tid,NULL,client_thread,n1); //쓰레드 생성

	pthread_mutex_destroy(&m_lock); //쓰레드가 끝나고 변수를 쓸 일이 없으면 destroy 시킨다.

    }
    return 0;
}

int sum(char *cgiaddr) {	//total.cgi 처리
	char *result;	//char형식의 변수 선언 
	char token[] = "=&";	// char 형식의  변수 선언 
	int i;		//int형 변수 선언 
	int num1, num2;	//int형변수 n1,n2선언 
	int sum = 0;		//int형 변수 sum선언 

	result = strtok(cgiaddr, token);	//토큰 분리자 앞에거 지움
	result = strtok(NULL, token);	//토큰 분리
	num1 = atoi(result);		//첫번째 숫자 num1에 저장
    
	strtok(NULL, token);		//토큰 분리자 앞에거 지움
	result = strtok(NULL, token);	//토큰 분리
	num2 = atoi(result);		//두번째 숫자 num2에 저장

	i = (num2 - num1) + 1;		//반복 횟수
	for (i = num1; i <= num2; i++) {//계산
		sum += i;		
	}
	return sum;			//계산 값 리턴
}

void log(char *ip, char *fname, int fsize) //클라이언트ip, 전송파일명, 전송 크기
{
	int logfd; //로그 변수
	char logbuff[200]; //로그에 적기 위한 버퍼 변수

	sprintf(logbuff, "%s %s %d\n", ip, fname, fsize); //매개변수들 합치기
	//printf("%s", logbuff);

	if ((logfd = open(path, O_WRONLY | O_APPEND, 0644))) { //로그파일 열기
	    //printf("로그파일 open!\n");
	    write(logfd, logbuff, strlen(logbuff)); //로그에 버퍼 내용 작성
	    close(logfd); //파일 닫기
	}
}

void *client_thread(void *n1)
{
	struct stat s;		//파일 정보 얻기
	num2 *n2 = (num2 *)n1;	//소켓 정보 저장

	int *curfd = n2->nn;	//현재 소켓 번호 저장
	char *buff = n2->buff;	//현재 소켓 주소 저장

	char file_name[50];	//파일 이름
	int size;		//파일 크기 
	int file_fd; //파일이 없는 경우 리턴값 저장하기 위한 변수
	int buflen, len;
	int i, ret;
	char * fstr;		//content type
	char buffer[BUFSIZE + 1];	//버퍼

	ret = read(curfd, buffer, BUFSIZE); //fd에서 읽어옴  

	if (ret == 0 || ret == -1) //읽기 실패
		return 0;
	if (ret > 0 && ret < BUFSIZE) //정상 범위라면
		buffer[ret] = 0;
	else
		buffer[0] = 0;

	for (i = 4; i < BUFSIZE; i++) { //경로 만들기
		if (buffer[i] == ' ') { //공백이면
			buffer[i] = 0; //값은 0
			break;
		}
	}

	if (!strncmp(&buffer[0], "GET /\0", 6)) //GET \0이면
		strcpy(buffer, "GET /index.html"); //index.html 출력하도록 buffer에 저장

	buflen = strlen(buffer); //길이 저장
	fstr = NULL; //fstr 초기화
	for (i = 0; extensions[i].ext != 0; i++) { //구조체 검색
		len = strlen(extensions[i].ext); //길이 지정
		if (!strncmp(&buffer[buflen - len], extensions[i].ext, len)) { //길이만큼 비교
			fstr = extensions[i].filetype; //gif형식이면 image/gif로
			break;
		}
	}

	strcpy(file_name, &buffer[5]); //파일 이름 복사
	file_fd = open(file_name, O_RDONLY); //get을 떼어네고 파일을 열어봄 없는 파일일때 
	fstat(file_fd, &s); //파일인지 아닌지 확인
						
	//파일이 아닌 경우
	if (file_fd == -1) {
		if (strstr(buffer, "&to")) {  //주소에 &, to가 있는 경우
			int n = sum(&buffer[5]); //sum 함수 호출
			sprintf(buffer, "'HTTP/1.1 200 OK'\r\n\r\n<!DOCTYPE HTML><HTML><HEAD><meta http-equiv='Content-type' content='text/html'; charset=utf-8'/></HEAD><BODY><H1>%d</H1></BODY></HTML>\r\n", n);
			write(curfd, buffer, strlen(buffer));
			log(buff, file_name, strlen(buffer)); //로그 작성
		}
		else { //주소에 &, to 가 들어가지 않은 경우
			sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>NOT FOUND</H1></BODY></HTML>\r\n");
			write(curfd, buffer, strlen(buffer));
			log(buff, file_name, 9); //로그 작성
		}
	}

	//파일인 경우
	else if (S_ISDIR(s.st_mode)) { //파일 이름이 틀린 경우
		sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>NOT FOUND</H1></BODY></HTML>\r\n");
		write(curfd, buffer, strlen(buffer));
		log(buff, file_name, 9); //로그 작성
	}

	//정상적인 파일인 경우
	sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
	log(buff, file_name, s.st_size); //로그 작성
	write(curfd, buffer, strlen(buffer));
	
	//
	while ((ret = read(file_fd, buffer, BUFSIZE)) > 0) {
		write(curfd, buffer, ret);
	}
	close(curfd);//파일close
	free(n1); //동적할당 해제
	return 0;
}
