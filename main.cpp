#include <iostream>
#include <stdexcept>
#include <string>

#include "bitmap_image.hpp"
#include "uTensor.h"

using std::cout;
using std::endl;
using namespace uTensor;

int str2int(const char* str);

class SlicedImage {
  public:
    SlicedImage(const Tensor& t, unsigned int num_row_slices,
                      unsigned int num_col_slices)
        : image(t), cur_slice_r(0), cur_slice_c(0),
          num_row_slices(num_row_slices),
          num_col_slices(num_col_slices),
          num_slices(static_cast<uint16_t>(num_row_slices * num_col_slices)),
          sliced_height(static_cast<uint16_t>(t->get_shape()[1] / num_row_slices)),
          sliced_width(static_cast<uint16_t>(t->get_shape()[2] / num_col_slices)) {
      // TODO
      // Do asserts to make sure there is no remainder from slicing h/w      
      // Do assert to make sure num_channels == 3
      cout << "Num Slices:    " << num_slices << endl;
      cout << "Sliced Height: " << sliced_height << endl;
      cout << "Sliced Width:  " << sliced_width << endl;
    }

    void set_current_slice(uint8_t slice_row, uint8_t slice_col) { 
      cur_slice_r = slice_row; 
      cur_slice_c = slice_col;
    }
  //const uint8_t operator()(uint16_t y, uint16_t x, uint16_t c) const {
  //  return static_cast<uint8_t>(image(0, y + num_row_slices*sliced_height, x + num_col_slices*sliced_width, c));
  //}
  // Readonly
  uint8_t operator()(uint16_t y, uint16_t x, uint16_t c){
    return static_cast<uint8_t>(image(0, y + cur_slice_r*sliced_height, x + cur_slice_c*sliced_width, c));
  }

 private:
  const Tensor& image;
  uint8_t cur_slice_r;
  uint8_t cur_slice_c;

 public:
  const unsigned int num_row_slices;
  const unsigned int num_col_slices;
  const uint16_t num_slices;
  const uint16_t sliced_height;
  const uint16_t sliced_width;
};

Tensor&& bilinear_interpolate(SlicedImage& slicedImage, uint16_t target_height, uint16_t target_width, bool pad_to_square=false) {
  bool width_larger = (target_height < target_width) ? true : false;
  const uint16_t orig_height = (slicedImage.num_row_slices)*(slicedImage.sliced_height);
  const uint16_t orig_width = (slicedImage.num_col_slices)*(slicedImage.sliced_width);
  uint16_t pad_left = 0;
  uint16_t pad_top = 0;
  if(width_larger){
    assert(("Invalid rescale shapes", orig_width > orig_height));
    if(pad_to_square){
      pad_top = (target_width - target_height) >> 1; // Divide by 2
      target_height += 2*pad_top;
    }
  } else {
    assert(("Invalid rescale shapes", orig_width <= orig_height));
    if(pad_to_square) {
      pad_left = (target_height - target_width) >> 1; // Divide by 2
      target_width += 2*pad_left;
    }
  }
  
  Tensor scaledImg = new RamTensor({1, target_height, target_width, 3}, u8);
  // TODO consider making these float
  const uint16_t h_scale = orig_height / target_height;
  const uint16_t w_scale = orig_width / target_width;

  for(uint16_t out_y = 0; out_y < target_height;  out_y++) {
    for(uint16_t out_x = 0; out_x < target_width;  out_x++) {
      const uint16_t out_y_origin = out_y - pad_top;
      const uint16_t out_x_origin = out_x - pad_left;
      for(uint16_t c = 0; c < 3; c++) {
        uint8_t out_val = 0;
        // If in padded region
        if (!((out_x_origin >= 0) && (out_x_origin < target_width) && (out_y_origin >= 0) &&
            (out_y_origin < target_height))) {
          out_val = 0;
        } else { // We are not in padded region
          // TODO consider doing float scaling then back to int post
          // Note these are int on purpose so we can check bounds more easily
          const int16_t in_y = out_y_origin * h_scale;
          const int16_t in_x = out_x_origin * w_scale;
          const int16_t x1 = ( (in_x - 1) > 0 ) ? in_x - 1 : 0; 
          const int16_t y1 = ( (in_y - 1) > 0 ) ? in_y - 1 : 0; 
          const int16_t x2 = ( (in_x + 1) < target_width  ) ? in_x + 1 : target_width-1; 
          const int16_t y2 = ( (in_y + 1) < target_height ) ? in_y + 1 : target_height-1; 
          
          const uint8_t Q11 = slicedImage(y1, x1, c);
          const uint8_t Q21 = slicedImage(y1, x2, c);
          const uint8_t Q12 = slicedImage(y2, x1, c);
          const uint8_t Q22 = slicedImage(y2, x2, c);

          // Interpolate in x dir
          const float fxy1 = 
            static_cast<float>(Q11)*static_cast<float>(x2-in_x)/static_cast<float>(x2-x1) + 
            static_cast<float>(Q21)*static_cast<float>(in_x - x1)/static_cast<float>(x2-x1);
          const float fxy2 = 
            static_cast<float>(Q12)*static_cast<float>(x2-in_x)/static_cast<float>(x2-x1) + 
            static_cast<float>(Q22)*static_cast<float>(in_x - x1)/static_cast<float>(x2-x1);
          
          // Interpolate in y dir
          const float out_f = 
            fxy1*static_cast<float>(y2-in_y)/static_cast<float>(y2-y1) +
            fxy2*static_cast<float>(in_y - y1)/static_cast<float>(y2-y1);
          
          out_val = static_cast<uint8_t>(out_f);
        } // else not in padding
        scaledImg(0, out_y, out_x, c) = out_val;
      }
    }
  }
  return std::move(scaledImg);
}

/*
 * uTensor module instantiation
 */
localCircularArenaAllocator<2048> meta_allocator;
localCircularArenaAllocator<4000000, uint32_t>
    ram_allocator;  // 4MB should be plenty
SimpleErrorHandler mErrHandler(10);

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
  SlicedImage slicedImage(image_t, num_row_slices, num_col_slices);

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
