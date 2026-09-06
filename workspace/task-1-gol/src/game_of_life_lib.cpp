#include "game_of_life_lib.h"

void update_step(int width, int height, uint8_t *buffer) {
  if (width == 0 || height == 0) return;

  const int cell_count = width * height;
  // Bit 0 stores the old generation. Bit 1 will store the new generation.
  // Normalizing first also handles arbitrary non-zero input byte values.
  for (int i = 0; i < cell_count; ++i) buffer[i] = buffer[i] != 0;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int neighbors = 0;
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          const int nx = x + dx, ny = y + dy;
          if ((dx != 0 || dy != 0) && nx >= 0 && nx < width && ny >= 0 && ny < height)
            neighbors += buffer[ny * width + nx] & 1;
        }
      }
      const int index = y * width + x;
      const bool alive = neighbors == 3 || (neighbors == 2 && (buffer[index] & 1));
      buffer[index] |= static_cast<uint8_t>(alive) << 1;
    }
  }

  for (int i = 0; i < cell_count; ++i) buffer[i] >>= 1;
}
