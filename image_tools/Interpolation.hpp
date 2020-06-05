#ifndef IMAGE_INTERPOLATION_HPP
#define IMAGE_INTERPOLATION_HPP
#include "SlicedImage.hpp"

// Can be smarter than a regular function
// TODO cache parameters so we dont have to do extra computation on hot runs
template<typename T>
class BilinearInterpolator {
  public:
    uTensor::TensorInterface* interpolate(SlicedImage<T>& slicedImage, uint16_t target_size, bool pad_to_square=false);
};

template<typename T>    
uTensor::TensorInterface* BilinearInterpolator<T>::interpolate(SlicedImage<T>& slicedImage, uint16_t target_size, bool pad_to_square) {
      const uint16_t orig_height = slicedImage.sliced_height;
      const uint16_t orig_width = slicedImage.sliced_width;
      bool width_larger = (orig_height < orig_width) ? true : false;
      uint16_t pad_left = 0;
      uint16_t pad_top = 0;
      uint16_t target_height = 0;
      uint16_t target_width = 0;
      if(width_larger){
        target_width = target_size;
        target_height = static_cast<uint16_t>(static_cast<float>(orig_height)*
            ( static_cast<float>(target_size) / static_cast<float>(orig_width) ));
        if(pad_to_square){
          pad_top = (target_width - target_height) >> 1; // Divide by 2
          // TODO assert target_height no greater than target_size
          target_height = target_size;
        }
      } else {
        target_height = target_size;
        target_width = static_cast<uint16_t>(static_cast<float>(orig_width)*
            ( static_cast<float>(target_size) / static_cast<float>(orig_height) ));
        if(pad_to_square) {
          pad_left = (target_height - target_width) >> 1; // Divide by 2
          target_width = target_size;
        }
      }

      uTensor::TensorInterface* scaledImg = new uTensor::RamTensor({1, target_height, target_width, 3}, u8);
      // TODO consider making these float
      // Shift back to original scales if padding
      const float h_scale = static_cast<float>(orig_height) / static_cast<float>(target_height - 2*pad_top); 
      const float w_scale = static_cast<float>(orig_width) / static_cast<float>(target_width - 2*pad_left);
      const uint16_t target_width_ubound = target_width - 2*pad_left; // ignore right padding
      const uint16_t target_height_ubound = target_height - 2*pad_top; // ignore bottom padding

      for(uint16_t out_y = 0; out_y < target_height;  out_y++) {
        for(uint16_t out_x = 0; out_x < target_width;  out_x++) {
          const uint16_t out_y_origin = out_y - pad_top;
          const uint16_t out_x_origin = out_x - pad_left;
          for(uint16_t c = 0; c < 3; c++) {
            T out_val = 0;
            // If in padded region
            if (!((out_x_origin >= 0) && (out_x_origin < target_width_ubound) && (out_y_origin >= 0) &&
                  (out_y_origin < target_height_ubound))) {
              out_val = 0;
            } else { // We are not in padded region
              // TODO consider doing float scaling then back to int post
              // Note these are int on purpose so we can check bounds more easily
              const int16_t in_y = static_cast<int16_t>(static_cast<float>(out_y_origin) * h_scale);
              const int16_t in_x = static_cast<int16_t>(static_cast<float>(out_x_origin) * w_scale);
              const int16_t x1 = ( (in_x - 1) > 0 ) ? in_x - 1 : 0; 
              const int16_t y1 = ( (in_y - 1) > 0 ) ? in_y - 1 : 0; 
              const int16_t x2 = ( (in_x + 1) < target_width_ubound  ) ? in_x + 1 : target_width_ubound-1; 
              const int16_t y2 = ( (in_y + 1) < target_height_ubound ) ? in_y + 1 : target_height_ubound-1; 

              const T Q11 = slicedImage(y1, x1, c);
              const T Q21 = slicedImage(y1, x2, c);
              const T Q12 = slicedImage(y2, x1, c);
              const T Q22 = slicedImage(y2, x2, c);

              // Interpolate in x dir
              const float fxy1 = 
                static_cast<float>(Q11)*static_cast<float>(x2-in_x)/static_cast<float>(x2-x1) + 
                static_cast<float>(Q21)*static_cast<float>(in_x - x1)/static_cast<float>(x2-x1);
              const float fxy2 = 
                static_cast<float>(Q12)*static_cast<float>(x2-in_x)/static_cast<float>(x2-x1) + 
                static_cast<float>(Q22)*static_cast<float>(in_x - x1)/static_cast<float>(x2-x1);

              // Interpolate in y dir
              float out_f = 
                fxy1*static_cast<float>(y2-in_y)/static_cast<float>(y2-y1) +
                fxy2*static_cast<float>(in_y - y1)/static_cast<float>(y2-y1);
              out_f = (out_f > 255) ? 255 : out_f;
              out_f = (out_f < 0) ? 0 : out_f;

              out_val = static_cast<T>(out_f);
            } // else not in padding
            (*scaledImg)(0, out_y, out_x, c) = out_val;
          }
        }
      }
      return scaledImg;
    }

#endif
