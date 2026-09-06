#pragma once

#include <cstdint>
#include <vector>

struct Pixel {
  std::uint8_t r{}, g{}, b{}, a{255};
};

class Image {
 public:
  Image(int width, int height, Pixel background = {});
  int width() const;
  int height() const;
  Pixel &operator()(int x, int y);
  const Pixel &operator()(int x, int y) const;
  const std::vector<Pixel> &pixels() const;

 private:
  int width_{}, height_{};
  std::vector<Pixel> pixels_;
};

void DrawPixel(Image &image, int x, int y, Pixel color);
void DrawLine(Image &image, int x0, int y0, int x1, int y1, Pixel color);
void FillRectangle(Image &image, int x0, int y0, int x1, int y1, Pixel color);
void FillCircle(Image &image, int center_x, int center_y, int radius, Pixel color);
void FillTriangle(Image &image, int x0, int y0, int x1, int y1,
                  int x2, int y2, Pixel color);
