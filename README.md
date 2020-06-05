# uTensor Image Tools

A Simple set of image processing tools built using uTensor. It is currently very experimental and is sure to have some bugs.

## Building

```
git clone https://github.com/mbartling/uTensor-image-tools.git
cd uTensor-image-tools
git submodule init && git submodule update
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

The default program allows you to pass the path to a BMPv3 uint8_t 3 channel RGB image, the desired number of row slices, and the desired number of column slices.
```
./image_scaling_ex ../images/7.bmp 4 8
```

### Example converting image to 4x3 aspect ratio using imagemagick 
```
convert -scale 384x512 -background black -gravity center -extent 384x512 -format bmp -define bmp:format=bmp3 7.jpg 7.bmp
```

