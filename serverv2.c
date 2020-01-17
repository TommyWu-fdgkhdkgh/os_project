#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>

#define BUF_SIZE 1000000
//bigger than a size of frame
//先測一下frame size多少
//可以在python client那邊print

#define SERV_PORT 50000


void sig_child(int signo){
  pid_t pid;
  int stat;

  while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0);

  return;
}


void process_data(int sockfd) {
  ssize_t n;
  char buf[BUF_SIZE];
  char frame_buf[BUF_SIZE];
  char frame_size_buf[100];

  int fd = open("./gg.jpg", O_CREAT | O_WRONLY);
  if(fd < 0) {
    printf("open error!\n");
    assert(-1);
  }

  int receive_frame_size = 1;
  int temp_buf_size = 16;

  memset(frame_size_buf, 0, 100); 


again:

  //讀取可能會是8bytes 8bytes突然跳到40幾萬bytes
  //第一部分先確保“只”讀16bytes
  //剩下的確保“只”讀16bytes拿到的frame大小

  while(1) {

    n = read(sockfd, buf, temp_buf_size);
    if(n <= 0) {
      break;
    }
    temp_buf_size -= n; 

    if(receive_frame_size) {

      if(temp_buf_size == 0) {
        receive_frame_size = 0;
	buf[16] = 0;
	temp_buf_size = atoi(buf);
        receive_frame_size = 0; 	
      } else if(temp_buf_size > 0) {
        
      } else {
        printf("receive_frame_size error!\n");
        assert(-1);
      } 

    } else {
      //以後試試不要寫到檔案裡，而是直接用memory
      //因為看到opencv的api都是讀檔，所以先用讀檔來頂一下
      //frame_buf
      memcpy(frame_buf, buf, );            
 



    }
  }

  while((n = read(sockfd, buf, BUF_SIZE)) > 0) {
    printf("receive %ld bytes\n", n);
    /*if(first_receive) {
      printf("%s\n", buf);
      first_receive = 0;
    } else {
      if(beginning_of_frame) {
        memcpy(frame_size_buf, buf, 16);
        int frame_size = atoi(frame_size_buf);
        printf("frame_size : %d\n", frame_size);
        beginning_of_frame = 0;

	write(fd, buf+16, n-16);
      } else {
        write(fd, buf, n);
      }
    }*/
  }


  if(n == 0) {
    printf("connection close\n");
    close(fd);
  }

  if(n < 0 && errno == EINTR) {
    goto again;
  } else if (n < 0) {
    printf("read error!\n");
    close(fd);
    assert(-1);
  }
  close(fd);
}

int main() {

  char input_buf[BUF_SIZE];
  int sockfd = 0, forClientSockfd = 0;
  struct sockaddr_in serverInfo,clientInfo;
  int addrlen = sizeof(struct sockaddr_in);

  //for signal 
  struct sigaction act, oact;

  //for fork
  pid_t childpid;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0) {
    printf("failed to get sockfd!\n");
    exit(-1);
  }

  memset(&serverInfo, 0, sizeof(struct sockaddr_in));

  //set server IP and port
  serverInfo.sin_family = PF_INET;
  serverInfo.sin_addr.s_addr = INADDR_ANY;
  serverInfo.sin_port = htons(SERV_PORT);

  act.sa_handler = &sig_child;
  sigemptyset (&act.sa_mask);
  act.sa_flags = 0;
  
  if (sigaction (SIGCHLD, &act, &oact) < 0) {
    printf("sigaction error !\n");
    exit(-1);
  }

  bind(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo)); 

  listen(sockfd, 30);

  while(1) {
    forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
    if(forClientSockfd < 0) {
      //get signal or really error
      if(errno == EINTR) {
        continue;
      } else {
        printf("accept error!\n");
      }

    }

    childpid = fork();
    if(childpid < 0) {
      printf("fork error!\n");
      exit(-1);
    }

    if(childpid == 0) {
      close(sockfd);

      process_data(forClientSockfd);
      //do the things
      //recv(forClientSockfd, input_buf, sizeof(input_buf), 0);
      //printf("Get:%s\n",input_buf);
      exit(0);
    }

    //let the child process do the things
    close(forClientSockfd); 

  }

  return 0;
}
