#include "surface.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define PI 3.141592654
#define MAGNIFIER_FACTOR 1000
#define MAX_TAN MAGNIFIER_FACTOR * 1000

// Thanks ChatGPT
#define FOCAL_LENGTH_UM_OV2640 3600
#define WIDTH_SENSOR_PIXELS_OV2640 320
#define HEIGHT_SENSOR_PIXELS_OV2640 240
#define WIDTH_SENSOR_UM_OV2640 2720

// Thanks Google
#define PIXELS_PER_CM 38

Surface::Surface(int w_cm,
                 int h_cm,
                 int distance_cm,
                 int x_offset_cm,
                 int y_offset_cm,
                 int x_tilt_deg,
                 int y_tilt_deg)
    : w_cm(w_cm), h_cm(h_cm), distance_cm(distance_cm), x_offset_cm(x_offset_cm), y_offset_cm(y_offset_cm), x_tilt_deg(x_tilt_deg), y_tilt_deg(y_tilt_deg)
{
}

size_t Surface::points(Point3d *&points) const
{
    PixelColor red{0xFF, 0, 0};
    PixelColor white{0xFF, 0xFF, 0xFF};
    PixelColor &color = red;

    int focal_length_pixels = FOCAL_LENGTH_UM_OV2640 * WIDTH_SENSOR_PIXELS_OV2640 / WIDTH_SENSOR_UM_OV2640;

    int total_w_pixels = PIXELS_PER_CM * w_cm;
    int total_h_pixels = PIXELS_PER_CM * h_cm;
    int x_offset_pixels = PIXELS_PER_CM * x_offset_cm;
    int y_offset_pixels = PIXELS_PER_CM * y_offset_cm;
    int x_tilt_tan = (int)(MAGNIFIER_FACTOR * tan(((double)x_tilt_deg) * PI / 180));
    int x_tilt_cos = (int)(MAGNIFIER_FACTOR * cos(((double)x_tilt_deg) * PI / 180));
    x_tilt_tan = x_tilt_tan > MAX_TAN ? MAX_TAN : x_tilt_tan;
    int y_tilt_tan = (int)(MAGNIFIER_FACTOR * tan(((double)y_tilt_deg) * PI / 180));
    int y_tilt_cos = (int)(MAGNIFIER_FACTOR * cos(((double)y_tilt_deg) * PI / 180));
    y_tilt_tan = y_tilt_tan > MAX_TAN ? MAX_TAN : y_tilt_tan;

    points = (Point3d *)malloc(sizeof(Point3d) * total_h_pixels * total_w_pixels);

    for (int r = 0; r < total_h_pixels; r++)
    {
        for (int c = 0; c < total_w_pixels; c++)
        {
            int id = r * total_w_pixels + c;

            int x = (c - total_w_pixels / 2 + x_offset_pixels) * x_tilt_cos / MAGNIFIER_FACTOR;
            int y = (r - total_h_pixels / 2 + y_offset_pixels) * y_tilt_cos / MAGNIFIER_FACTOR;
            int z = distance_cm * PIXELS_PER_CM + x * x_tilt_tan / MAGNIFIER_FACTOR + y * y_tilt_tan / MAGNIFIER_FACTOR;
            if ((r % (total_h_pixels / 3) < (total_h_pixels / 6)) and
                (c % (total_w_pixels / 3) < (total_w_pixels / 6)))
            {
                points[id] = Point3d(z, x, y, red);
            }
            else
            {
                points[id] = Point3d(z, x, y, white);
            }
        }
    }

    return total_h_pixels * total_w_pixels;
}
