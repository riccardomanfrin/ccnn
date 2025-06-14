#ifndef __POINT2D_H__
#define __POINT2D_H__

#include <stdint.h>

#include "pixel_color.h"

class Point2d
{
public:
    int x = 0;
    int y = 0;
    PixelColor color;

public:
    Point2d(int x, int y) : x(x), y(y) {}
    Point2d(int x, int y, PixelColor c) : x(x), y(y), color(c) {}
};
#endif