#include "raster.h"

#include <cassert>

Image::Image(int width, int height, Pixel background)
    : width_(width), height_(height), pixels_(static_cast<std::size_t>(width * height), background) {
  assert(width >= 0 && height >= 0);
}
int Image::width() const { return width_; }
int Image::height() const { return height_; }
Pixel &Image::operator()(int x, int y) {
  assert(x >= 0 && x < width_ && y >= 0 && y < height_);
  return pixels_[static_cast<std::size_t>(y * width_ + x)];
}
const Pixel &Image::operator()(int x, int y) const {
  assert(x >= 0 && x < width_ && y >= 0 && y < height_);
  return pixels_[static_cast<std::size_t>(y * width_ + x)];
}
const std::vector<Pixel> &Image::pixels() const { return pixels_; }
