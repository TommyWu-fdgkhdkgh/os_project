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

#include "/usr/local/include/opencv2/core/core.hpp"        // Basic OpenCV structures (cv::Mat)
#include "/usr/local/include/opencv2/highgui/highgui.hpp"  // Video write
#include "/usr/local/include/opencv2/opencv.hpp"

//alpr
#include "tclap/CmdLine.h"
#include "support/filesystem.h"
#include "support/timing.h"
#include "support/platform.h"
#include "video/videobuffer.h"
#include "motiondetector.h"
#include "alpr.h"

//hadoop
#include "hdfs.h"

#define BUF_SIZE 10000
//bigger than a size of frame
//先測一下frame size多少
//可以在python client那邊print

#define SERV_PORT 50000

using namespace cv;
using namespace alpr;

//openalpr
MotionDetector motiondetector;
bool do_motiondetection = true;
bool measureProcessingTime = false;
std::string templatePattern;

void  upload2hdfs(char *filename){
  
  //for hdfs
  hdfsFS fs = hdfsConnect("master", 9000);
  int n;
  char buf[10005];
  char *writePath;

  writePath = (char *)malloc(100);
  memset(writePath, 0, 100);

  writePath[0] = '/';
  writePath[1] = 't';
  writePath[2] = 'm';
  writePath[3] = 'p';
  writePath[4] = '/';

  // 01.txt
  filename[3] = 't';
  filename[4] = 'x';
  filename[5] = 't';
  filename[6] = 0;
  
  int fd_frame_has_plate = open(filename, O_RDONLY);
  if(fd_frame_has_plate < 0) {
    printf("fd 1 error!\n");
    exit(-1);
  }

  /*
  filename[3] = 'g';
  filename[4] = 'g';
  filename[5] = 0;
  int fd_frame_has_plate_output = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

  if(fd_frame_has_plate_output < 0) {
    printf("fd 3 error!\n");
    exit(-1);
  }
  */


  strcat(writePath, filename);
  writePath[11] = 0;
  printf("writePath : %s\n", writePath);

  hdfsFile hdfs_has_plate = hdfsOpenFile(fs, writePath, O_WRONLY | O_CREAT, 0, 0, 0);

  while(1) {
    n = read(fd_frame_has_plate, buf, 5000);
    if(n == 0) {
      printf("frame has plate done\n");
      break;
    } else if(n < 0) {
      printf("read 1 error!\n");
      exit(-1);
    }

    //write(fd_frame_has_plate_output, buf, n);
    //tSize num_written_bytes = hdfsWrite(fs, writeFile, (void*)buffer, strlen(buffer)+1);
    hdfsWrite(fs, hdfs_has_plate, (void *)buf, n);
    if (hdfsFlush(fs, hdfs_has_plate)) {
      fprintf(stderr, "Failed to 'flush' %s\n", writePath);
      exit(-1);
    }
  }


  filename[3] = 'm';
  filename[4] = 'p';
  filename[5] = '4';
  filename[6] = 0;

  int fd_video = open(filename, O_RDONLY);  
  if(fd_video < 0) {
    printf("fd 2 error!\n");
    exit(-1);
  }

  writePath[8]  = 'm';
  writePath[9]  = 'p';
  writePath[10] = '4';
  writePath[11] = 0;

  printf("writePath : %s\n", writePath);

  hdfsFile hdfs_video = hdfsOpenFile(fs, writePath, O_WRONLY | O_CREAT, 0, 0, 0);

  /*
  filename[3] = 'i';
  filename[4] = 'n';
  filename[5] = 0;

  int fd_video_output = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if(fd_video_output < 0) {
    printf("fd 4 error!\n");
    exit(-1);
  }*/

  while(1) {
    n = read(fd_video, buf, 10000);
    if(n == 0) {
      printf("video done\n");
      break;
    } else if(n < 0) {
      printf("read 1 error!\n");
      exit(-1);
    }

    //printf("writing video!! %d\n", n);

    write(fd_video_output, buf, n);
    hdfsWrite(fs, hdfs_video, (void *)buf, n);
    if (hdfsFlush(fs, hdfs_video)) {
      fprintf(stderr, "Failed to 'flush' %s\n", writePath);
      exit(-1);
    }

  }

  close(fd_frame_has_plate);
  //close(fd_frame_has_plate_output);
  hdfsCloseFile(fs, hdfs_has_plate);
  close(fd_video);
  close(fd_video_output);
  hdfsCloseFile(fs, hdfs_video);
}


bool detectandshow( Alpr* alpr, Mat frame, std::string region, bool writeJson)
{
  timespec startTime;
  getTimeMonotonic(&startTime);

  Rect rectan = motiondetector.MotionDetect(&frame);
  return rectan.width > 0;
}


void sig_child(int signo){
  pid_t pid;
  int stat;

  while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0);

  return;
}


void process_data(Alpr &alpr, int sockfd) {
  ssize_t n;
  char buf[BUF_SIZE];
  char frame_size_buf[100];
  char filename[10];

  std::vector<char> bytes;

  Mat frame;

  int receive_frame_size = 1;
  int temp_buf_size = 16;
  int total_frame_num = 0;
  int frame_size = 0;

  int start_push_to_data = 0;

  int frame_has_plate = 0;

  memset(frame_size_buf, 0, 100); 
  memset(filename, 0, sizeof(char) * 10);

  FILE *fp;

again:

  n = read(sockfd, filename, 6);
  if(n <= 0) {
    printf("receive file name error!\n");
    exit(-1);
  }
  filename[6] = 0;

  printf("filename : %s\n", filename);
  
  VideoWriter outputVideo(filename, CV_FOURCC('M', 'P', '4', 'V'), 60, Size(1280, 720));


  filename[2] = '.';
  filename[3] = 't';
  filename[4] = 'x';
  filename[5] = 't';
  filename[6] = 0;

  fp = fopen(filename, "w");


  while(1) {
    //每次想要讀取的量
    //n = read(sockfd, buf, temp_buf_size);
    n = read(sockfd, buf, temp_buf_size > BUF_SIZE ? BUF_SIZE : temp_buf_size);
    if(n <= 0) {
      break;
    }

    temp_buf_size -= n; 

    if(receive_frame_size) {

      if(temp_buf_size == 0) {
        receive_frame_size = 0;
  
        int index = 0;
        while(1) {
          if(buf[index]>='1' && buf[index]<='9') {

            temp_buf_size = atoi(buf+index);
	    frame_size = temp_buf_size;
            if(temp_buf_size < 10000) {
              
	    } else {
              break;
	    }
	  }
	  index++;
	}

	temp_buf_size = atoi(buf+index);

        for(int i = 0;i < 16;i++) {
          printf(" %d ", buf[i]);
	}
	printf("\n");

        printf("frame size this time : %d\n", temp_buf_size);

        receive_frame_size = 0; 	
      } else if(temp_buf_size > 0) {
        
      } else {
        printf("receive_frame_size error!\n");
        assert(0);
      } 

    } else {

      for(int i = 0;i < n;i++) {

        //不知道為什麼，前面一堆0x20
	//需要把0x20過濾掉
        if(buf[i] == '\xff' && !start_push_to_data) {
          start_push_to_data = 1;
	  temp_buf_size = frame_size - (n-i); 
	}
  
        if(start_push_to_data) {
          bytes.push_back(static_cast<char>(buf[i]));
	}
      }

       
      if(temp_buf_size == 0) {
	start_push_to_data = 0;

        /*for(int i = 0;i < 10;i++) {
          printf("bytes[%d] = %d\n", i, bytes[i]);
	}*/

        printf("size of bytes : %d\n", bytes.size());

        printf("let's make a jpg!\n");
        frame = imdecode(bytes, CV_LOAD_IMAGE_UNCHANGED); 

        if(frame.data == NULL) {
          printf("imdecode failed!\n");
	  assert(0);
	} else {
          //for openalpr
          std::cout << "frame number : " << total_frame_num << std::endl;
          if (total_frame_num == 0) {
            motiondetector.ResetMotionDetection(&frame); 
	  }
          if(detectandshow(&alpr, frame, "", false)){
	    fprintf(fp, "%d\n", total_frame_num);
	    frame_has_plate++;
	  }

          total_frame_num++;
	}

        outputVideo << frame;

        bytes.clear();
        receive_frame_size = 1;
	temp_buf_size = 16;
      }

    }
  }

  printf("total_frame_num : %d\n", total_frame_num);

  if(n == 0) {
    printf("connection close\n");
    printf("frame has plate : %d\n", frame_has_plate); 
    fclose(fp);

    outputVideo.release();
    
    sleep_ms(10); 

    //system("ls -al");
    //start to upload file to hdfs
    upload2hdfs(filename);

  }

  if(n < 0 && errno == EINTR) {
    goto again;
  } else if (n < 0) {
    printf("read error!\n");
    //close(fd);
    assert(0);
  }
}

int main() {

  std::cout << "OpenCV " << CV_VERSION << std::endl;

  char input_buf[BUF_SIZE];
  int sockfd = 0, forClientSockfd = 0;
  struct sockaddr_in serverInfo,clientInfo;
  int addrlen = sizeof(struct sockaddr_in);

  //for openalpr
  string country("eu");
  string configFile(""); 
  Alpr alpr(country, configFile);

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
    //addrlen這個參數以後可以考慮換個形態
    forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, (socklen_t*)&addrlen);
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

      process_data(alpr, forClientSockfd);
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
