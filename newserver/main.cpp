#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <fstream>
#include <map>

#include "/usr/local/include/opencv2/core/core.hpp"        // Basic OpenCV structures (cv::Mat)
#include "/usr/local/include/opencv2/highgui/highgui.hpp"  // Video write
#include "/usr/local/include/opencv2/opencv.hpp"

#include "server.hpp"
#include "session.hpp"

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


using namespace std;
using namespace cv;
using namespace alpr;

map<int, Session> fd2bytes;
int num_of_clients_done = 0;

//openalpr
MotionDetector motiondetector;
bool do_motiondetection = true;
bool measureProcessingTime = false;
string templatePattern;
Alpr *alpr_pointer;

void upload_file(uint16_t fd) {
  char filename[7];

  memcpy(filename, fd2bytes[fd].filename, 6);
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

    hdfsWrite(fs, hdfs_video, (void *)buf, n);
    if (hdfsFlush(fs, hdfs_video)) {
      fprintf(stderr, "Failed to 'flush' %s\n", writePath);
      exit(-1);
    }

  }

  close(fd_frame_has_plate);
  hdfsCloseFile(fs, hdfs_has_plate);
  close(fd_video);
  hdfsCloseFile(fs, hdfs_video);

}

bool detectandshow( Alpr* alpr, Mat frame, std::string region, bool writeJson)
{
  //timespec startTime;
  //getTimeMonotonic(&startTime);

  Rect rectan = motiondetector.MotionDetect(&frame);
  return rectan.width > 0;
}

void newclient_callback(uint16_t fd) {
  
}

void receive_callback(uint16_t fd, char *buf, int nbytes){

  int index = 0;

  //cout << "-----" << endl;
  //cout << "fd : " << fd << endl;

  //cout << "receive : " << nbytes << "bytes" << endl;

  while(index < nbytes) {

    //cout << "fd2bytes.stage : " << fd2bytes[fd].stage << endl; 

    //originally, the fd is not existed in the map
    if(fd2bytes.find(fd) == fd2bytes.end()) {

      Session newSession;
      fd2bytes[fd] = newSession;

    } 
    // filename
    else if(fd2bytes[fd].stage == 0) {

      memcpy(fd2bytes[fd].filename, buf, 6);
      fd2bytes[fd].stage = 1;
      fd2bytes[fd].need_n_bytes = 16;
      index += 6; 
     
      char openFilename[7];
      memcpy(openFilename, fd2bytes[fd].filename, 6);
      openFilename[3] = 't';  
      openFilename[4] = 'x';  
      openFilename[5] = 't';  
      openFilename[6] = 0;  

      fd2bytes[fd].fp = fopen(openFilename, "w");
      fd2bytes[fd].outVp = new VideoWriter(fd2bytes[fd].filename, CV_FOURCC('M', 'P', '4', 'V'), 60, Size(1280, 720));
      /*openFilename[3] = 'a';  
      openFilename[4] = 'v';  
      openFilename[5] = 'i';  
      openFilename[6] = 0;  

      fd2bytes[fd].outVp = new VideoWriter(openFilename, CV_FOURCC('M', 'J', 'P', 'G'), 60, Size(1280, 720));*/
    } 
    // frame size
    else if(fd2bytes[fd].stage == 1) {

      if((nbytes - index) >= fd2bytes[fd].need_n_bytes) {
        for(int i = 0;i < fd2bytes[fd].need_n_bytes;i++) {
          fd2bytes[fd].bytes.push_back(buf[index]);
	  index++;
	}

        char intbuf[16];
	for(int i = 0;i < 16;i++){
          intbuf[i] = fd2bytes[fd].bytes[i];
	}

        fd2bytes[fd].bytes.clear();
        fd2bytes[fd].need_n_bytes = atoi(intbuf);
	fd2bytes[fd].total_size += fd2bytes[fd].need_n_bytes;
        fd2bytes[fd].stage = 2;

	//cout << "frame size : " << fd2bytes[fd].need_n_bytes << endl;
      } else {
	fd2bytes[fd].need_n_bytes -= (nbytes - index);
        for(;index < nbytes;index++){
          fd2bytes[fd].bytes.push_back(buf[index]); 
	}

	//cout << "fd2bytes[fd].need_n_bytes : " << fd2bytes[fd].need_n_bytes << endl;  
      }

    } 
    // frame
    else if(fd2bytes[fd].stage == 2) {
      
      if((nbytes - index) >= fd2bytes[fd].need_n_bytes) {
        for(int i = 0;i < fd2bytes[fd].need_n_bytes;i++) {
          fd2bytes[fd].bytes.push_back(buf[index]);
	  index++;
	}

        
	Mat frame;
	frame = imdecode(fd2bytes[fd].bytes, CV_LOAD_IMAGE_UNCHANGED);

	if(frame.data == NULL) {
          cout << "imdecode error!" << endl;
	  exit(-1);
	} else {

          if(fd2bytes[fd].total_frame == 0) {
            motiondetector.ResetMotionDetection(&frame);
	  }

          fd2bytes[fd].outVp->write(frame);

	  if(detectandshow(alpr_pointer, frame, "", false)){
            fprintf(fd2bytes[fd].fp, "%d\n", fd2bytes[fd].total_frame);
          }

          fd2bytes[fd].total_frame++;
          //cout << "imdecode suc!!"  << endl;
	}

        fd2bytes[fd].bytes.clear();
        fd2bytes[fd].need_n_bytes = 16;
        fd2bytes[fd].stage = 1;
      } else {
	fd2bytes[fd].need_n_bytes -= (nbytes - index);
        for(;index < nbytes;index++){
          fd2bytes[fd].bytes.push_back(buf[index]); 
	}

	//cout << "fd2bytes[fd].need_n_bytes : " << fd2bytes[fd].need_n_bytes << endl;  
      }

    } else {
      cout << "fd2bytes[fd].stage error!!!" << endl;
      exit(-1);
    } 

  }

}

void disconnect_callback(uint16_t fd) {

  //send file to hdfs
  
  fd2bytes[fd].outVp->release();
  fclose(fd2bytes[fd].fp);

  upload_file(fd);
  cout << "total size : " << fd2bytes[fd].total_size << endl;
  cout << "total frame :" <<  fd2bytes[fd].total_frame << endl;

  fd2bytes.erase(fd);
  num_of_clients_done++;
}

int main() {

  cout << "OpenCV " << CV_VERSION << endl;

  //for openalpr
  string country("eu");
  string configFile("");
  alpr_pointer = new Alpr(country, configFile);
  //Alpr alpr

  Server s(50000);
  s.init();
  s.onConnect(newclient_callback);
  s.onInput(receive_callback);
  s.onDisconnect(disconnect_callback);
  while(1) {
    if(num_of_clients_done == 5) {
      return 0;
    }
    s.loop();
  }


  return 0;
}
