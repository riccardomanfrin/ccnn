#ifndef __PIXEL_COLOR_H__
#define __PIXEL_COLOR_H__
#include <stdint.h>
class PixelColor {
   public:
    uint8_t r = 0xFF;
    uint8_t g = 0xFF;
    uint8_t b = 0xFF;

   public:
    PixelColor(uint8_t val) {
        r = val;
        g = val;
        b = val;
    }
    PixelColor(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
    PixelColor() {}
    int clear() {
        r = 0;
        g = 0;
        b = 0;
        return 0;
    }
};
#endif