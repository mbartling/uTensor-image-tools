#include <iostream>
#include "uTensor.h"
#include "bitmap_image.hpp"

using std::cout;
using std::endl;

int main (int argc, char *argv[]) {
  if (argc != 2) {
    cout << "Unexpected number of command line args" << endl << "Please provide a target bmp file to load" << endl;
  }
  return 0;
}
