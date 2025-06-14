#include "cam.h"

Cam::Cam(int px_w, int px_h, int focal_length_pixels) : px_w(px_w),
                                                     px_h(px_h),
                                                     focal_length_pixels(focal_length_pixels),
                                                     bmp(px_w, px_h)
{
}

int Cam::project(const GraphObj &obj)
{
    Point3d *points;
    size_t amount = obj.points(points);
    for (int i = 0; i < amount; i++)
    {

        /* object_distance / focal_length_pixels = object_width  / projected_width_in_pixels

            Eg.
            W_real = 17cm
            focal_length_pixels = 423px
            W_image_pixels = 100px
            => d ~= 72cm

            w_cm / distance_cm = px_w / focal_length_pixels
        */

        Point3d &p = points[i];
        /*
                ___p.z
               |    ___focal_length
              _|_ _|__
             |   |   /
             |  _|  /
             | | | /
          p.x| | |/
             | | /
             | |/
             | /\
             |/  \_projected_x


        p.x / projected_x = (p.z + focal_length) /focal_length;
        */
        int projected_x = p.x * focal_length_pixels / (p.z + focal_length_pixels);
        int projected_y = p.y * focal_length_pixels / (p.z + focal_length_pixels);
        Point2d p2d(projected_x, projected_y);
        p2d.color = p.color;
        bmp.set_pixel(p2d);
    }
    free(points);
    return 0;
}
Bmp *Cam::render() {
    return &bmp;
}
