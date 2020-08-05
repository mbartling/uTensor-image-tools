#ifndef IMAGE_INTERPOLATION_HPP
#define IMAGE_INTERPOLATION_HPP
#include "SlicedImage.hpp"
#include <cmath>
#include <cstdio>

// Can be smarter than a regular function
// TODO cache parameters so we dont have to do extra computation on hot runs
template <typename T>
class BilinearInterpolator {
 public:
  uTensor::TensorInterface* interpolate(SlicedImage<T>& slicedImage,
                                        uint16_t target_size,
                                        bool pad_to_square = false);
  uTensor::TensorInterface* interpolate(SlicedImage<T>& slicedImage,
                                        uint16_t target_height,
                                        uint16_t target_width,
                                        uint16_t pad_top = 0,
                                        uint16_t pad_left = 0);
};

template <typename T>
uTensor::TensorInterface* BilinearInterpolator<T>::interpolate(
    SlicedImage<T>& slicedImage, uint16_t target_size, bool pad_to_square) {
  const uint16_t orig_height = slicedImage.sliced_height;
  const uint16_t orig_width = slicedImage.sliced_width;
  bool width_larger = (orig_height < orig_width) ? true : false;
  uint16_t pad_left = 0;
  uint16_t pad_top = 0;
  uint16_t target_height = 0;
  uint16_t target_width = 0;
  if (width_larger) {
    target_width = target_size;
    target_height = static_cast<uint16_t>(
        static_cast<float>(orig_height) *
        (static_cast<float>(target_size) / static_cast<float>(orig_width)));
    if (pad_to_square) {
      pad_top = (target_width - target_height) >> 1;  // Divide by 2
      // TODO assert target_height no greater than target_size
      target_height = target_size;
    }
  } else {
    target_height = target_size;
    target_width = static_cast<uint16_t>(
        static_cast<float>(orig_width) *
        (static_cast<float>(target_size) / static_cast<float>(orig_height)));
    if (pad_to_square) {
      pad_left = (target_height - target_width) >> 1;  // Divide by 2
      target_width = target_size;
    }
  }

  uTensor::TensorInterface* scaledImg =
      new uTensor::RamTensor({1, target_height, target_width, 3}, u8);
  // TODO consider making these float
  // Shift back to original scales if padding
  const float h_scale = static_cast<float>(orig_height) /
                        static_cast<float>(target_height - 2 * pad_top);
  const float w_scale = static_cast<float>(orig_width) /
                        static_cast<float>(target_width - 2 * pad_left);
  const uint16_t target_width_ubound =
      target_width - 2 * pad_left;  // ignore right padding
  const uint16_t target_height_ubound =
      target_height - 2 * pad_top;  // ignore bottom padding

  for (uint16_t out_y = 0; out_y < target_height; out_y++) {
    for (uint16_t out_x = 0; out_x < target_width; out_x++) {
      const uint16_t oy_s = out_y - pad_top;
      const uint16_t ox_s = out_x - pad_left;
      for (uint16_t c = 0; c < 3; c++) {
        T out_val = 0;
        if (oy_s < 0 || ox_s < 0 || oy_s >= target_height_ubound || ox_s >= target_width_ubound){
          out_val = 0;
        } else { // Not in padding 
          // Map outputs to input space
          const float y = h_scale * static_cast<float>(oy_s);
          const float x = w_scale * static_cast<float>(ox_s);
          //printf("%d %d %f %f\n", out_y, out_x, y, x);
          const float y1 = std::floor(y - 1);
          const float x1 = std::floor(x - 1);
          const float y2 = std::ceil(y + 1);
          const float x2 = std::ceil(x + 1);

          const int16_t x1i = ( static_cast<int16_t>(x1)  < 0 ) ? orig_width  + static_cast<int16_t>(x1)  : static_cast<int16_t>(x1) % orig_width;
          const int16_t y1i = ( static_cast<int16_t>(y1)  < 0 ) ? orig_height + static_cast<int16_t>(y1)  : static_cast<int16_t>(y1) % orig_height;
          const int16_t x2i = ( static_cast<int16_t>(x2)  < 0 ) ? orig_width  + static_cast<int16_t>(x2)  : static_cast<int16_t>(x2) % orig_width;
          const int16_t y2i = ( static_cast<int16_t>(y2)  < 0 ) ? orig_height + static_cast<int16_t>(y2)  : static_cast<int16_t>(y2) % orig_height;
          const float fQ11 = static_cast<float>(static_cast<T>(slicedImage(y1i, x1i, c)));
          const float fQ21 = static_cast<float>(static_cast<T>(slicedImage(y1i, x2i, c)));
          const float fQ12 = static_cast<float>(static_cast<T>(slicedImage(y2i, x1i, c)));
          const float fQ22 = static_cast<float>(static_cast<T>(slicedImage(y2i, x2i, c)));

          //interpolate in x dir, handle same val case
          const float xm = ( fabs(x2 - x1) < 0.00001 )? 1.0 : x2 - x1;
          const float tx2 = ( fabs(x2 - x) < 0.00001 )? 1.0 : x2 - x;
          const float tx1 = ( fabs(x - x1) < 0.00001 )? 1.0 : x - x1;
          const float fxy1 = (tx2 / xm)*fQ11 + (tx1 / xm)*fQ21;
          const float fxy2 = (tx2 / xm)*fQ12 + (tx1 / xm)*fQ22;

          //interpolate in y dir
          const float ym = (fabs(y2 - y1) < 0.000001 ) ? 1.0 : y2 - y1;
          const float ty2 = ( fabs(y2 - y) < 0.00001 )? 1.0 : y2 - y;
          const float ty1 = ( fabs(y - y1) < 0.00001 )? 1.0 : y - y1;
          float fxy = (ty2 / ym) * fxy1 + (ty1 / ym) * fxy2;
          fxy = (fxy > 255) ? 255 : fxy;
          fxy = (fxy < 0) ? 0 : fxy;
          out_val = static_cast<T>(fxy);

        }
        (*scaledImg)(0, out_y, out_x, c) = out_val;
      }
    }
  }
  return scaledImg;
}

template <typename T>
uTensor::TensorInterface* BilinearInterpolator<T>::interpolate(
    SlicedImage<T>& slicedImage, 
    uint16_t target_height, 
    uint16_t target_width,
    uint16_t pad_top,
    uint16_t pad_left) {
  const uint16_t orig_height = slicedImage.sliced_height;
  const uint16_t orig_width = slicedImage.sliced_width;

  const uint16_t scaled_height = target_height + 2 * pad_top;
  const uint16_t scaled_width  = target_width  + 2 * pad_left;
  uTensor::TensorInterface* scaledImg =
      new uTensor::RamTensor({1, scaled_height, scaled_width, 3}, u8);

  const float h_scale = static_cast<float>(orig_height) / static_cast<float>(target_height);
  const float w_scale = static_cast<float>(orig_width) / static_cast<float>(target_width);
  //printf("h scale %f, w scale %f\n", h_scale, w_scale);

  for (uint16_t out_y = 0; out_y < scaled_height; out_y++) {
    for (uint16_t out_x = 0; out_x < scaled_width; out_x++) {
      const int16_t oy_s = out_y - pad_top;
      const int16_t ox_s = out_x - pad_left;
      for(uint16_t c = 0; c < 3; c++ ){
      T out_val = 0;
      if (oy_s < 0 || ox_s < 0 || oy_s >= target_height || ox_s >= target_width){
        out_val = 0;
      } else { // Not in padding 
        // Map outputs to input space
        const float y = h_scale * static_cast<float>(oy_s);
        const float x = w_scale * static_cast<float>(ox_s);
        //printf("%d %d %f %f\n", out_y, out_x, y, x);
        const float y1 = std::floor(y - 1);
        const float x1 = std::floor(x - 1);
        const float y2 = std::ceil(y + 1);
        const float x2 = std::ceil(x + 1);

        const int16_t x1i = ( static_cast<int16_t>(x1)  < 0 ) ? orig_width  + static_cast<int16_t>(x1)  : static_cast<int16_t>(x1) % orig_width;
        const int16_t y1i = ( static_cast<int16_t>(y1)  < 0 ) ? orig_height + static_cast<int16_t>(y1)  : static_cast<int16_t>(y1) % orig_height;
        const int16_t x2i = ( static_cast<int16_t>(x2)  < 0 ) ? orig_width  + static_cast<int16_t>(x2)  : static_cast<int16_t>(x2) % orig_width;
        const int16_t y2i = ( static_cast<int16_t>(y2)  < 0 ) ? orig_height + static_cast<int16_t>(y2)  : static_cast<int16_t>(y2) % orig_height;
        const float fQ11 = static_cast<float>(static_cast<T>(slicedImage(y1i, x1i, c)));
        const float fQ21 = static_cast<float>(static_cast<T>(slicedImage(y1i, x2i, c)));
        const float fQ12 = static_cast<float>(static_cast<T>(slicedImage(y2i, x1i, c)));
        const float fQ22 = static_cast<float>(static_cast<T>(slicedImage(y2i, x2i, c)));

        //interpolate in x dir, handle same val case
        const float xm = ( fabs(x2 - x1) < 0.00001 )? 1.0 : x2 - x1;
        const float tx2 = ( fabs(x2 - x) < 0.00001 )? 1.0 : x2 - x;
        const float tx1 = ( fabs(x - x1) < 0.00001 )? 1.0 : x - x1;
        const float fxy1 = (tx2 / xm)*fQ11 + (tx1 / xm)*fQ21;
        const float fxy2 = (tx2 / xm)*fQ12 + (tx1 / xm)*fQ22;

        //interpolate in y dir
        const float ym = (fabs(y2 - y1) < 0.000001 ) ? 1.0 : y2 - y1;
        const float ty2 = ( fabs(y2 - y) < 0.00001 )? 1.0 : y2 - y;
        const float ty1 = ( fabs(y - y1) < 0.00001 )? 1.0 : y - y1;
        float fxy = (ty2 / ym) * fxy1 + (ty1 / ym) * fxy2;
        fxy = (fxy > 255) ? 255 : fxy;
        fxy = (fxy < 0) ? 0 : fxy;
        out_val = static_cast<T>(fxy);

      }
      (*scaledImg)(0, out_y, out_x, c) = out_val;
      } // Per channel
    } // out_x
  } // out_y

  return scaledImg;
}
#endif
