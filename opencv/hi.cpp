#include <iostream> // for standard I/O
#include <string>   // for strings

#include "/usr/local/include/opencv2/core/core.hpp"        // Basic OpenCV structures (cv::Mat)
#include "/usr/local/include/opencv2/highgui/highgui.hpp"  // Video write
#include "alpr.h"

using namespace std;
using namespace cv;
using namespace alpr;

//https://www.learnopencv.com/how-to-find-frame-rate-or-frames-per-second-fps-in-opencv-python-cpp/
//fps

//Size_(_Tp _width, _Tp _height);

int main()
{
  Alpr gg();

  Mat frame;
  VideoCapture cap = VideoCapture();
  cap.open("./01.mp4");
  //cap.set(CV_CAP_PROP_POS_MSEC, seektoms);

  cap.read(frame);

  cout << "width :" << frame.cols << endl; 
  cout << "height :" << frame.rows << endl; 
  cout << "fps :" << cap.get(CV_CAP_PROP_FPS) << endl; 

  //VideoWriter outputVideo("VideoTest.avi", CV_FOURCC('M', 'J', 'P', 'G'), cap.get(CV_CAP_PROP_FPS), Size(frame.cols, frame.rows));	
  //VideoWriter outputVideo("VideoTest.avi", CV_FOURCC('M', 'P', '4', '2'), cap.get(CV_CAP_PROP_FPS), Size(frame.cols, frame.rows));	
  //VideoWriter outputVideo("VideoTest.mp4", CV_FOURCC('M', 'P', '4', '2'), cap.get(CV_CAP_PROP_FPS), Size(frame.cols, frame.rows));	
  VideoWriter outputVideo("t2.avi", CV_FOURCC('M', 'J', 'P', 'G'), cap.get(CV_CAP_PROP_FPS), Size(frame.cols, frame.rows));	
  //VideoWriter outputVideo("t2.avi", CV_FOURCC('M', 'J', 'P', 'G'), 60, Size(1920, 1080));	
  //好扯，檔名真的會影響格式。。。。，只要把副檔名改成mp4，格式就會變成跟助教差不多

//CV_FOURCC('M', 'J', 'P', 'G') = motion-jpeg codec

  //影片莫名的很大一包
   
  outputVideo << frame;

  int fcount = 1;

  while(1) {
    fcount++;
    cout << "fcount : " << fcount << endl;

    if(fcount == 3) {
      break;
    }

    cap >> frame;
    if(frame.empty()){
      break;
    }
 
    outputVideo << frame;
  }  
 
  //輸出的影片變很大一包是怎樣QQ

/*
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
 
using namespace cv;
 
void main()
{
	VideoCapture capture(0);
	VideoWriter writer("VideoTest.avi", CV_FOURCC('M', 'J', 'P', 'G'), 25.0, Size(640, 480));
	Mat frame;
	
	while (capture.isOpened())
	{
		capture >> frame;
		writer << frame;
		imshow("video", frame);
		if (cvWaitKey(20) == 27)
		{
			break;
		}
	}
}
*/
  return 0;
}
