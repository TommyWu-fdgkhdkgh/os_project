#include <cstdio>
#include <sstream>
#include <iostream>
#include <iterator>
#include <algorithm>

//#include "opencv2/highgui/highgui.hpp"
//#include "opencv2/imgproc/imgproc.hpp"

#include "/usr/local/include/opencv2/core/core.hpp"
#include "/usr/local/include/opencv2/highgui/highgui.hpp"
#include "/usr/local/include/opencv2/imgproc/imgproc.hpp"

#include "tclap/CmdLine.h"
#include "support/filesystem.h"
#include "support/timing.h"
#include "support/platform.h"
#include "video/videobuffer.h"
#include "motiondetector.h"
#include "alpr.h"

using namespace alpr;

int main() {

  Alpr::getVersion();
  cv::VideoWriter outputVideo;


  return 0;
}
