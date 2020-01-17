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
void newclient_callback(uint16_t fd) {
  
}

void receive_callback(uint16_t fd, char *buf, int nbytes){

  int index = 0;

  cout << "-----" << endl;
  cout << "fd : " << fd << endl;

  cout << "receive : " << nbytes << "bytes" << endl;

  while(index < nbytes) {

    cout << "fd2bytes.stage : " << fd2bytes[fd].stage << endl; 

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
        fd2bytes[fd].stage = 2;

	cout << "frame size : " << fd2bytes[fd].need_n_bytes << endl;
      } else {
	fd2bytes[fd].need_n_bytes -= (nbytes - index);
        for(;index < nbytes;index++){
          fd2bytes[fd].bytes.push_back(buf[index]); 
	}

	cout << "fd2bytes[fd].need_n_bytes : " << fd2bytes[fd].need_n_bytes << endl;  
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
          cout << "imdecode suc!!"  << endl;
	}

        fd2bytes[fd].bytes.clear();
        fd2bytes[fd].need_n_bytes = 16;
        fd2bytes[fd].stage = 1;
      } else {
	fd2bytes[fd].need_n_bytes -= (nbytes - index);
        for(;index < nbytes;index++){
          fd2bytes[fd].bytes.push_back(buf[index]); 
	}

	cout << "fd2bytes[fd].need_n_bytes : " << fd2bytes[fd].need_n_bytes << endl;  
      }

    } else {
      cout << "fd2bytes[fd].stage error!!!" << endl;
      exit(-1);
    } 

  }

}

void disconnect_callback(uint16_t fd) {

  //send file to hdfs
  //upload_file(fd);

  fd2bytes.erase(fd);

}

int main() {

  cout << "OpenCV " << CV_VERSION << endl;

  Server s(50000);
  s.init();
  s.onConnect(newclient_callback);
  s.onInput(receive_callback);
  s.onDisconnect(disconnect_callback);
  while(1) {
    s.loop();
  }

  /*cout << "hi" << endl;

  Server *s = new Server();

  s->init();
  s->onConnect(newclient_callback);
  s->onInput(receive_callback);
  s->onDisconnect(disconnect_callback);
  while(1) {
    s->loop();
  }

  vector<char> data;
  data.push_back(0);
  data.push_back(10);

  ofstream outfile("./aa", ios::out | ios::binary);
  outfile.write(&data[0], data.size());
  outfile.write(&data[0], data.size());
  outfile.write(&data[0], data.size());
  outfile.write(&data[0], data.size());
  outfile.close();
  */

}
