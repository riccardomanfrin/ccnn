#ifndef __SCENE_H__
#define __SCENE_H__

#include "graph_obj.h"
#include "bmp.h"

class Cam
{
private:
    int px_w;
    int px_h;
    int focal_length_pixels;
    Bmp bmp;

public:
    Cam(int px_w, int px_h, int focal_length_pixels);
    int project(const GraphObj &obj);
    Bmp *render();
};
#endif