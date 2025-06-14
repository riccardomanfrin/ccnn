#ifndef __POINT3D_H__
#define __POINT3D_H__

#include <stdint.h>
#include "point2d.h"

class Point3d : public Point2d
{
public:
    int z = 0;

public:
    Point3d(int z, int x, int y) : Point2d(x, y), z(z) {}
    Point3d(int z, int x, int y, PixelColor &c) : Point2d(x, y), z(z) { color = c; }
};
#endif