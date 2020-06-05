#include <iostream>
#include <stdexcept>
#include <string>

#include "bitmap_image.hpp"
#include "uTensor.h"
#include "SlicedImage.hpp"
#include "Interpolation.hpp"

using std::cout;
using std::endl;
using namespace uTensor;

int str2int(const char* str);

/*
 * uTensor module instantiation
 */
localCircularArenaAllocator<2048> meta_allocator;
localCircularArenaAllocator<4000000, uint32_t>
    ram_allocator;  // 4MB should be plenty
SimpleErrorHandler mErrHandler(10);
BilinearInterpolator<uint8_t> bilinear;

int main(int argc, char* argv[]) {
  if (argc != 4) {
    cout << "Unexpected number of command line args" << endl
         << "Please provide image_path num_row_slices num_col_slices, in that "
            "order"
         << endl;
  }

  bitmap_image image(argv[1]);
  image.save_image("dummy.bmp");
  if (!image) {
    printf("Error - Failed to open: %s\n", argv[1]);
    return 1;
  }

  unsigned int total_number_of_pixels = 0;

  const uint16_t height = image.height();
  const uint16_t width = image.width();
  const unsigned int num_row_slices = str2int(argv[2]);
  const unsigned int num_col_slices = str2int(argv[3]);

  cout << "Image Height: " << height << endl;
  cout << "Image Width:  " << width << endl;
  cout << "Num Rows:     " << num_row_slices << endl;
  cout << "Num Cols:     " << num_col_slices << endl;

  uint8_t* image_buffer = image.data();  // Stored internally as a vector

  // From here on, things will look closer to what it's like on an embedded
  // system
  Context::get_default_context()->set_metadata_allocator(&meta_allocator);
  Context::get_default_context()->set_ram_data_allocator(&ram_allocator);
  Context::get_default_context()->set_ErrorHandler(&mErrHandler);

  // Keep the buffer around as we may want tu use it for something
  Tensor image_t = new BufferTensor(
      {1, height, width, 3}, u8, image_buffer);
  SlicedImage<uint8_t> slicedImage(image_t, num_row_slices, num_col_slices);

  // Output the sliced image
  bitmap_image oimage(slicedImage.sliced_width, slicedImage.sliced_height);
  slicedImage.set_current_slice(0 , 3);
  for (int y = 0; y < slicedImage.sliced_height; y++) {
    for (int x = 0; x < slicedImage.sliced_width; x++) {
      // uint8_t r,g,b;
      const uint8_t r = slicedImage(y, x, 2);
      const uint8_t g = slicedImage(y, x, 1);
      const uint8_t b = slicedImage(y, x, 0);
      oimage.set_pixel(x, y, r, g, b);
    }
  }
  cout << "Saving image" << endl;
  oimage.save_image("output.bmp");

  // Bilinear interpolation
  slicedImage.set_current_slice(0 , 3);
  int target_size = 32;
  Tensor scaledImage =  bilinear.interpolate(slicedImage, target_size, true);
  bitmap_image simage(target_size, target_size);
  for (int y = 0; y < scaledImage->get_shape()[1]; y++) {
    for (int x = 0; x < scaledImage->get_shape()[2]; x++) {
      // uint8_t r,g,b;
      const uint8_t r = scaledImage(0, y, x, 2);
      const uint8_t g = scaledImage(0, y, x, 1);
      const uint8_t b = scaledImage(0, y, x, 0);
      simage.set_pixel(x, y, r, g, b);
    }
  }
  cout << "Saving image" << endl;
  simage.save_image("scaled_output.bmp");

  return 0;
}

// Extra stuff

int str2int(const char* str) {
  std::string arg = str;
  int x;
  try {
    std::size_t pos;
    x = std::stoi(arg, &pos);
    if (pos < arg.size()) {
      std::cerr << "Trailing characters after number: " << arg << '\n';
    }
  } catch (std::invalid_argument const& ex) {
    std::cerr << "Invalid number: " << arg << '\n';
  } catch (std::out_of_range const& ex) {
    std::cerr << "Number out of range: " << arg << '\n';
  }
  return x;
}
