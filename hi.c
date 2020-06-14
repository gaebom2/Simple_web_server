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
#define LOG   44
#define HOME /index.html

pthread_mutex_t m_lock; //쓰레드 변수

//클라이언트 연결정보 구조체
typedef struct num{
	int *nn;
	char *buff;
	//char *path;
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

int cgi(char *buf){	//cgi 
   char *result;	//char형식의 변수 선언 
   char token[]="=&";	// char 형식의  변수 선언 
   int i;		//int형 변수 선언 
   int num1,num2;	//int형변수 n1,n2선언 
   int sum=0;		//int형 변수 sum선언 

   result=strtok(buf,token);
   result=strtok(NULL,token);
   num1=atoi(result);

   strtok(NULL,token);
   result=strtok(NULL,token);
   num2=atoi(result);

   i = (num2-num1)+1;
   for(i=num1;i<=num2;i++){
       sum+=i;
   }
   return sum;
}

void log(char *s1, char *s2,int size) //아이피,파일명, 파일 크기
{
   int fd;
   char logbuff[200];
   sprintf(logbuff,"%s %s %d\n",s1,s2,size);

   if((fd = open("./log.txt", O_WRONLY | O_APPEND,0644)) >= 0) {
      write(fd,logbuff,strlen(logbuff));
      close(fd);
   }
}

void *web(void *n1)
{
	struct stat s;		//파일 정보 얻기
	num2 *n2=(num2 *)n1;	//소켓 정보 저장
	
	int *fd = n2->nn;	//현재 소켓 번호 저장
	char *buff=n2->buff;	//현재 소켓 주소 저장
	//char *path = n2->path;
	printf("%s\n",buff);
	char file_name[50];	//파일 이름
	int size;		//파일 크기 
	int j, file_fd, buflen, len;
	int i, ret;
	char * fstr;		//content type
	char buffer[BUFSIZE+1];	//버퍼

	ret=read(fd,buffer,BUFSIZE); //fd에서 계속 읽어옴  
	
	if(ret == 0 || ret == -1) //읽기 실패
	    return 0;
	if(ret > 0 && ret < BUFSIZE) //정상 범위라면
	    buffer[ret]=0;
	else
	    buffer[0]=0;

      for(i=4;i<BUFSIZE;i++) { //경로 만들기
         if(buffer[i] == ' ') { //공백이면
            buffer[i] = 0; //값은 0
            break;
         }
      }

      if(!strncmp(&buffer[0],"GET /\0",6)) //GET \0이면
        strcpy(buffer,"GET /index.html"); //index.html 출력하도록 buffer에 저장
      
      buflen=strlen(buffer); //길이 저장
      fstr = NULL; //fstr 초기화
      for(i=0;extensions[i].ext != 0;i++) { //구조체 검색
         len = strlen(extensions[i].ext); //길이 지정
         if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) { //길이만큼 비교
            fstr =extensions[i].filetype; //gif형식이면 image/gif로
            break;
         }
      }



      strcpy(file_name, &buffer[5]); //파일 이름 복사
      //sprintf(n2->path, "%s%s", path, file_name);

      file_fd = open(file_name,O_RDONLY); //get을 떼어네고 파일을 열어봄 없는 파일일때 
      fstat(file_fd, &s); //파일인지 아닌지 확인
      if(file_fd == -1){ //파일이 아닌 경우
         if(strstr(buffer,"&to")){  //파일 이름에 &나 to가 있는 경우 검색
	    int n = cgi(&buffer[5]); //cgi 함수 호출
            sprintf(buffer,"HTTP/1.1 200 OK\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>%d</H1></BODY></HTML>\r\n", n); /* Header + a blank line */
            write(fd,buffer,strlen(buffer));
	    log(buff,file_name,strlen(buffer)-80); //로그 (ip, 파일, 몇번째)
         }
	 

	 else{
            //write(fd,"HTTP/1.1 404 Found",18); 
	    sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>NOT FOUND</H1></BODY></HTML>\r\n");
	     write(fd,buffer,strlen(buffer));
	     log(buff, file_name,9); //로그
	 }    
      }

      else if(S_ISDIR(s.st_mode)){ //파일인 경우
         //write(fd,"HTTP/1.1 404 Found",18); 
         sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>NOT FOUND</H1></BODY></HTML>\r\n");   
         write(fd,buffer,strlen(buffer));
	 log(buff, file_name, 9);
      }

      sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
      write(fd, buffer, strlen(buffer));
      while((ret = read(file_fd, buffer, BUFSIZE))>0){
	  write(fd, buffer, ret);
      }

    
      //size=s.st_size;
      // log(LOG,"SEND",buff,&buffer[5],hit); //ip,파일, 몇번 째
      
      
     //log(buff,file_name,size); //ip,파일, 몇번 째
     
     
     /* printf("%s\nend\n\n",buffer);
      write(fd,buffer,strlen(buffer));
       printf("im buff1: %s\n",buffer);
      
      while ((ret = read(file_fd, buffer, BUFSIZE)) > 0 ) {
         //   printf("im buff2: %s\n",buffer);
         write(fd,buffer,ret);
      }*/
	close(fd);
	free(n1);
	return 0;
}


int main(int argc, char *argv[])
{
    int fd;
    fd = open("./log.txt", O_CREAT | O_TRUNC, 0644); //로그파일 열기
    close(fd); //닫기

    pthread_t tid; //쓰레드 변수

    num2 *n1; //클라이언트 소켓 번호

    int i=0, j=0;		    //반복문 변수
    struct sockaddr_in sin,cli;	    //소켓 구조체 생성
    int sd,clientlen=sizeof(cli);   //소켓 번호
    //int *ns;

    //n1->path = argv[1];
    //printf("%s", n1->path);
    
    if(chdir(argv[1]) == -1){ //입력 경로로 이동
    	printf("ERROR: Can't Change to directory %s\n",argv[1]);
    }
    if((sd=socket(AF_INET,SOCK_STREAM,0)) == -1){   //소켓 생성
    	printf("socket() error");		    //에러메세지 출력
    }
    
    //소켓 구조체 정보 저장
    memset((char*)&sin,'\0',sizeof(sin));	//초기화
    sin.sin_family=AF_INET;			//인터넷 소켓 지정
    sin.sin_port=htons(atoi(argv[2]));		//포트번호 저장
    sin.sin_addr.s_addr=inet_addr("0.0.0.0");	//클라이언트 주소 저장

    if(bind(sd,(struct sockaddr *)&sin,sizeof(sin)) == -1){ //sd에 정보 bind
    	perror("bind() error");
    }
    if(listen(sd,100) == -1){	    //클라이언트 요청 대기
    	perror("listen() error");
    }

    while(1){
	pthread_mutex_init(&m_lock, NULL);
	
	n1=(num2*)malloc(sizeof(num2)); //동적 할당
	//ns=(int *)malloc(sizeof(int));
	if((n1->nn=accept(sd,(struct sockaddr *)&cli,&clientlen)) == -1){ //연결요청 수락
	    perror("accept() error");
	}	
	n1->buff=inet_ntoa(cli.sin_addr); //현재 연결된 클라이언트 주소 구조체에 저장
	pthread_create(&tid,NULL,web,n1); //쓰레드 생성

	pthread_mutex_destroy(&m_lock);

    }
    return 0;
}
