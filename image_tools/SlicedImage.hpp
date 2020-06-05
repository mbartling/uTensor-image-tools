#ifndef SLICED_IMAGE_HPP
#define SLICED_IMAGE_HPP
#include "uTensor.h"

template <typename T>
class SlicedImage {
 public:
  SlicedImage(const uTensor::Tensor& t, unsigned int num_row_slices,
              unsigned int num_col_slices);

  void set_current_slice(uint8_t slice_row, uint8_t slice_col);
  // const T operator()(uint16_t y, uint16_t x, uint16_t c) const;
  // Readonly
  T operator()(uint16_t y, uint16_t x, uint16_t c);

 private:
  const uTensor::Tensor& image;
  uint8_t cur_slice_r;
  uint8_t cur_slice_c;

 public:
  const unsigned int num_row_slices;
  const unsigned int num_col_slices;
  const uint16_t num_slices;
  const uint16_t sliced_height;
  const uint16_t sliced_width;
};

template <typename T>
SlicedImage<T>::SlicedImage(const uTensor::Tensor& t,
                            unsigned int num_row_slices,
                            unsigned int num_col_slices)
    : image(t),
      cur_slice_r(0),
      cur_slice_c(0),
      num_row_slices(num_row_slices),
      num_col_slices(num_col_slices),
      num_slices(static_cast<uint16_t>(num_row_slices * num_col_slices)),
      sliced_height(static_cast<uint16_t>(t->get_shape()[1] / num_row_slices)),
      sliced_width(static_cast<uint16_t>(t->get_shape()[2] / num_col_slices)) {
  // TODO
  // Do asserts to make sure there is no remainder from slicing h/w
  // Do assert to make sure num_channels == 3
}

template <typename T>
void SlicedImage<T>::set_current_slice(uint8_t slice_row, uint8_t slice_col) {
  cur_slice_r = slice_row;
  cur_slice_c = slice_col;
}
// const T operator()(uint16_t y, uint16_t x, uint16_t c) const {
//  return static_cast<T>(image(0, y + num_row_slices*sliced_height, x +
//  num_col_slices*sliced_width, c));
//}
// Readonly
template <typename T>
T SlicedImage<T>::operator()(uint16_t y, uint16_t x, uint16_t c) {
  return static_cast<T>(image(0, y + cur_slice_r * sliced_height,
                              x + cur_slice_c * sliced_width, c));
}

#endif
