#ifndef __BMP_H__
#define __BMP_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "point2d.h"

class Bmp {
   public:
    class Header {
       public:
        uint8_t file_type[2] = {'B', 'M'};
        uint8_t file_dim[4] = {};
        uint8_t padding[4] = {};
        uint8_t data_offset[4] = {40 + 14, 0, 0, 0};
        uint8_t dib_hdr_size[4] = {40, 0, 0, 0};
        uint8_t dib_hdr_width[4] = {};
        uint8_t dib_hdr_height[4] = {};
        uint8_t dib_hdr_colors_layers[2] = {1, 0};
        uint8_t dib_hdr_depth[2] = {24, 0};
        uint8_t dib_hdr_compression[4] = {};
        uint8_t dib_hdr_image_size[4] = {};
        uint8_t dib_hdr_ores[4] = {0, 0, 0, 0};
        uint8_t dib_hdr_vres[4] = {0, 0, 0, 0};
        uint8_t dib_hdr_colors[4] = {};
        uint8_t dib_hdr_prio_colors[4] = {};

        int encode(uint8_t *&buff);
        inline size_t size();
    };

   public:
    Bmp(size_t width, size_t height);
    ~Bmp();
    Header h;
    uint8_t *buff = NULL;

   public:
    inline int width();
    inline int height();
    int set_pixel(const Point2d &p);
    int set_pixel(int pixel_offset, PixelColor &color);
    int get_pixel(Point2d &p);
    int to_grayscale();
    inline size_t byte_size();
    size_t encode();
    int read(const char *filename);
    const uint8_t *get_buff();
    const int get_raw_grayscale_data(size_t &len, uint8_t *buff);
    int write(const char *filename);

   private:
    size_t row_pad();
    inline int x_y_coord_to_byte_offset(int x, int y, int &byte_offset);
    inline int pixel_offset_to_byte_offset(int pixel_offset, int &byte_offset);
    inline int byte_offset_to_x_y_coord(int byte_offset, int &x, int &y);
    
};

#endif