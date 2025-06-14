#ifndef __SURFACE_H__
#define __SURFACE_H__
#include "graph_obj.h"

class Surface : public GraphObj
{
public:
    int w_cm = 0;
    int h_cm = 0;
    int distance_cm = 0;
    int x_offset_cm = 0;
    int y_offset_cm = 0;
    int x_tilt_deg = 0;
    int y_tilt_deg = 0;

public:
    Surface(
        int w_cm,
        int h_cm,
        int distance_cm,
        int x_offset_cm,
        int y_offset_cm,
        int x_tilt_deg,
        int y_tilt_deg);
    virtual size_t points(Point3d *&points) const;
};
#endif