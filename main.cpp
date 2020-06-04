#include <iostream>
#include <stdexcept>
#include <string>

#include "uTensor.h"
#include "bitmap_image.hpp"

using std::cout;
using std::endl;

int str2int(const char* str);

int main (int argc, char *argv[]) {
  if (argc != 4) {
    cout << "Unexpected number of command line args" << endl << "Please provide image_path num_row_slices num_col_slices, in that order" << endl;
  }

  bitmap_image image(argv[1]);

  if (!image)
  {
    printf("Error - Failed to open: %s\n", argv[1]);
    return 1;
  }

  unsigned int total_number_of_pixels = 0;

  const unsigned int height = image.height();
  const unsigned int width  = image.width();
  const unsigned int num_row_slices = str2int(argv[2]);
  const unsigned int num_col_slices = str2int(argv[3]);

  return 0;
}

// Extra stuff

int str2int(const char* str){
  std::string arg = str;
  int x;
  try {
    std::size_t pos;
    x = std::stoi(arg, &pos);
    if (pos < arg.size()) {
      std::cerr << "Trailing characters after number: " << arg << '\n';
    }
  } catch (std::invalid_argument const &ex) {
    std::cerr << "Invalid number: " << arg << '\n';
  } catch (std::out_of_range const &ex) {
    std::cerr << "Number out of range: " << arg << '\n';
  }
  return x;
}
