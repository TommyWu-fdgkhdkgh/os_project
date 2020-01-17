#include "session.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <memory.h>

Session::Session() {
  stage = 0;
  total_frame = 0;
  total_size = 0;
  need_n_bytes = 6;
  memset(filename, 0, 7);
}

Session::~Session() {

}
