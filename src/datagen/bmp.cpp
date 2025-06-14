#include "bmp.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

int Bmp::Header::encode(uint8_t *&buff) {
    buff = (uint8_t *)memcpy(buff, file_type, 2) + 2;
    buff = (uint8_t *)memcpy(buff, file_dim, 4) + 4;
    buff = (uint8_t *)memcpy(buff, padding, 4) + 4;
    buff = (uint8_t *)memcpy(buff, data_offset, 4) + 4;
    buff = (uint8_t *)memcpy(buff, dib_hdr_size, 4) + 4;
    buff = (uint8_t *)memcpy(buff, dib_hdr_width, 4) + 4;
    buff = (uint8_t *)memcpy(buff, dib_hdr_height, 4) + 4;
    buff = (uint8_t *)memcpy(buff, dib_hdr_colors_layers, 2) + 2;
    buff = (uint8_t *)memcpy(buff, dib_hdr_depth, 2) + 2;
    buff = (uint8_t *)memcpy(buff, dib_hdr_compression, 4) + 4;
    buff = (uint8_t *)memcpy(buff, dib_hdr_image_size, 4) + 4;
    buff = (uint8_t *)memcpy(buff, dib_hdr_ores, 4) + 4;
    buff = (uint8_t *)memcpy(buff, dib_hdr_vres, 4) + 4;
    buff = (uint8_t *)memcpy(buff, dib_hdr_colors, 4) + 4;
    buff = (uint8_t *)memcpy(buff, dib_hdr_prio_colors, 4) + 4;

    return 0;
}
inline size_t Bmp::Header::size() {
    return 2 + 4 + 4 + 4 + 4 + 4 + 4 + 2 + 2 + 4 + 4 + 4 + 4 + 4 + 4;
}

Bmp::Bmp(size_t width, size_t height) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);
    memcpy(h.dib_hdr_width, &width, 4);
    memcpy(h.dib_hdr_height, &height, 4);
    size_t size = width * height * 3 + 40 + 14 + (4 - width % 4) * height;
    memcpy(h.file_dim, &size, 4);
    buff = (uint8_t *)malloc(size);
    memset(buff, 0x00, size);
}
Bmp::~Bmp() { free(buff); }
const uint8_t *Bmp::get_buff() { return buff; }
int Bmp::write(const char *filename) {
    size_t s = encode();
    const uint8_t *data = get_buff();
    FILE *f = fopen(filename, "w");
    fwrite(data, s, 1, f);
    fclose(f);
    return 0;
}

inline int Bmp::width() { return (*(uint32_t *)h.dib_hdr_width); }
inline int Bmp::height() { return (*(uint32_t *)h.dib_hdr_height); }

size_t Bmp::row_pad() { return (4 - ((width() * 3) % 4)) % 4; }

inline int Bmp::x_y_coord_to_byte_offset(int x, int y, int &byte_offset) {
    int x_coord = width() / 2 + x;
    int y_coord = height() / 2 + y;
    int pad = row_pad();
    byte_offset = y_coord * (pad + (width() * 3)) + x_coord * 3;
    return 0;
}
inline int Bmp::pixel_offset_to_byte_offset(int pixel_offset,
                                            int &byte_offset) {
    int x = pixel_offset % width();
    int y = pixel_offset / width();
    int pad = row_pad();
    byte_offset = y * (pad + (width() * 3)) + x * 3;
    return 0;
}
inline int Bmp::byte_offset_to_x_y_coord(int byte_offset, int &x, int &y) {
    int pad = row_pad();
    int x_coord = (byte_offset % (pad + (width() * 3))) / 3;
    int y_coord = byte_offset / (pad + (width() * 3));
    x = x_coord - width() / 2;
    y = y_coord - height() / 2;
    return 0;
}

int Bmp::set_pixel(const Point2d &p) {
    int byte_offset = 0;
    x_y_coord_to_byte_offset(p.x, p.y, byte_offset);

    uint8_t *ptr = (&buff[byte_offset]) + h.size();
    ptr[0] = p.color.b;
    ptr[1] = p.color.g;
    ptr[2] = p.color.r;
    return 0;
}
int Bmp::set_pixel(int pixel_offset, PixelColor &color) {
    int byte_offset;
    pixel_offset_to_byte_offset(pixel_offset, byte_offset);

    uint8_t *ptr = (&buff[byte_offset]) + h.size();
    ptr[0] = color.b;
    ptr[1] = color.g;
    ptr[2] = color.r;
    return 0;
}

int Bmp::get_pixel(Point2d &p) {
    int offset = 0;
    x_y_coord_to_byte_offset(p.x, p.y, offset);

    uint8_t *ptr = (&buff[offset]) + h.size();
    p.color.b = ptr[0];
    p.color.g = ptr[1];
    p.color.r = ptr[2];
    return 0;
}

int Bmp::to_grayscale() {
    int offset = 0;
    uint8_t *ptr = (&buff[0]) + h.size();
    int w = width() / 2;
    int h = height() / 2;
    for (int y = -h; y < h; y++) {
        for (int x = -w; x < w; x++) {
            x_y_coord_to_byte_offset(x, y, offset);
            uint8_t mean =
                (ptr[offset] + ptr[offset + 1] + ptr[offset + 2]) / 3;
            ptr[offset] = mean;
            ptr[offset + 1] = mean;
            ptr[offset + 2] = mean;
        }
    }
    return 0;
}
const int Bmp::get_raw_grayscale_data(size_t &len, uint8_t *outbuff) {
    int offset = 0;

    uint8_t *ptr = (&buff[0]) + h.size();

    int w = width() / 2;
    int h = height() / 2;
    len = 0;
    for (int y = -h; y < h; y++) {
        for (int x = -w; x < w; x++) {
            x_y_coord_to_byte_offset(x, y, offset);
            uint8_t mean =
                (ptr[offset] + ptr[offset + 1] + ptr[offset + 2]) / 3;
            outbuff[len] = mean;
            len++;
        }
    }
    return 0;
}

inline size_t Bmp::byte_size() {
    return h.size() + (width() * 3 + row_pad()) * height();
}

size_t Bmp::encode() {
    uint8_t *buffptr = buff;
    h.encode(buffptr);
    return byte_size();
}
int Bmp::read(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (1 != fread(buff, byte_size(), 1, f)) {
        return -1;
    }
    fclose(f);
    return 0;
}