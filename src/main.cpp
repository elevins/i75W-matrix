#include "pico/stdlib.h"

#include "libraries/pico_graphics/pico_graphics.hpp"
#include "libraries/interstate75/interstate75.hpp"

using namespace pimoroni;


// Display size in pixels, note: the i75W RP2350 kit includes two 128x64
// pixel displays which form a chain that's 256x64 pixels in side.
Hub75 hub75(256, 64, nullptr, PANEL_GENERIC, false);

// Our PicoGraphics surface. If we want a square output then use 128x128
// as the width and height, which Hub75 will automatically remap to 256x64.
PicoGraphics_PenRGB888 graphics(128, 128, nullptr);

// extra row of pixels for sourcing flames and averaging
const int buffer_zone = 4;
const int width = 128;
const int height = 128 + buffer_zone;

// a buffer that's at least big enough to store width + 2 * height values 
// (to accommodate both horizontal and vertical orientation)
float heat[width * height] = {0.0f};

// Bounds checked "set" and "get" methods so we can process our frame
// without having to worry about fetching/setting pixels out of bounds.
void set(int x, int y, float v) {
  x = x < 0 ? 0 : x;
  x = x >= width ? width - 1 : x;
  
  y = y < 0 ? 0 : y;
  y = y >= height ? height - 1 : y;

  heat[x + y * width] = v;
}

float get(int x, int y) {
  x = x < 0 ? 0 : x;
  x = x >= width ? width - 1 : x;
  
  y = y < 0 ? 0 : y;
  y = y >= height ? height - 1 : y;

  return heat[x + y * width];
}

// Interrupt callback required function 
void __isr dma_complete() {
  hub75.dma_complete();
}


int main() {
  //stdio_init_all();

  // Start hub75 driver
  hub75.start(dma_complete);

  while(true) {

    for(int y = 0; y < height; y++) {
      for(int x = 0; x < width; x++) {
        float value = get(x, y);

        // Only draw the visible pixels,
        // The two extra bottom rows are for seeding flames
        if(y < height - buffer_zone) {
          if(value > 0.5f) {
            graphics.set_pen(255, 255, 180);
          }
          else if(value > 0.4f) {
            graphics.set_pen(220, 160, 0);
          }
          else if(value > 0.3f) {
            graphics.set_pen(180, 30, 0);
          }
          else if(value > 0.22f) {
            graphics.set_pen(20, 20, 20);
          } else {
            graphics.set_pen(0, 0, 0);
          }
          graphics.pixel(Point(x, y));
        }

        // update this pixel by averaging the below pixels
        float average = (get(x, y) + get(x, y + 2) + get(x, y + 1) + get(x - 1, y + 1) + get(x + 1, y + 1)) / 5.0f;

        // damping factor to ensure flame tapers out towards the top of the displays
        average *= 0.99f;

        // update the heat map with our newly averaged value
        set(x, y, average);
      }
    }

    hub75.update(&graphics);

    // clear the bottom row and then add a new fire seed to it
    for(int x = 0; x < width; x++) {
      set(x, height - 1, 0.0f);
    }

    // add a new random heat source
    for(int c = 0; c < 5; c++) {
      int px = (rand() % (width - 4)) + 2;
      set(px, height - 1, 10.0f);
    }

    sleep_ms(10);
  }

  return 0;
}