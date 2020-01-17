#ifndef SESSION_HPP
#define SESSION_HPP

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <iostream>
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

using namespace std;
using namespace cv;
using namespace alpr;

class Session
{
public :
	Session();
	virtual ~Session();

	int stage;
	int need_n_bytes;
	int total_frame;
	int total_size;
	char filename[7];	
	vector<char> bytes;
	VideoWriter *outVp;
	FILE *fp;

private :
};

#endif /* SESSION_HPP */
