#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <list>
#include <map>

#include "bmp.h"
#include "cam.h"
#include "cnn.h"
#include "surface.h"

#define TRAINING_BATCHES_SIZE 10

#define FOCAL_LENGTH_UM_OV2640 3600
#define WIDTH_SENSOR_PIXELS_OV2640 320
#define HEIGHT_SENSOR_PIXELS_OV2640 240
#define WIDTH_SENSOR_UM_OV2640 2720

#define DATA_FOLDER "data/"
#define TEST_FILENAME_PREFIX "test_"

#define DISTANCE_CM_MIN 20
#define DISTANCE_CM_MAX 500
#define DISTANCE_CM_STEP 20

#define ANGLE_DEG_MIN 90
#define ANGLE_DEG_MAX 270
#define ANGLE_DEG_STEP 10

#define INPUT_DATA_SIZE WIDTH_SENSOR_PIXELS_OV2640 *HEIGHT_SENSOR_PIXELS_OV2640

#define OUTPUT_DATA_SIZE                               \
    (ANGLE_DEG_MAX - ANGLE_DEG_MIN) / ANGLE_DEG_STEP + \
        (DISTANCE_CM_MAX - DISTANCE_CM_MIN) / DISTANCE_CM_STEP

int output_to_outdata(int deg, int dist, value_t *output_data) {
    memset(output_data, 0, OUTPUT_DATA_SIZE * sizeof(int));
    int deg_idx = (deg - ANGLE_DEG_MIN) / ANGLE_DEG_STEP;
    int dist_idx = (dist - DISTANCE_CM_MIN) / DISTANCE_CM_STEP +
                   (ANGLE_DEG_MAX - ANGLE_DEG_MIN) / ANGLE_DEG_STEP;
    output_data[deg_idx] = MAX_NEURON_VAL;
    output_data[dist_idx] = MAX_NEURON_VAL;
    return 0;
}

int outdata_to_output(value_t *output_data, int &deg, int &dist) {
    int max_deg_id = 0, max_dist_id = 0;
    for (int i = 0; i < OUTPUT_DATA_SIZE; i++) {
        if (max_dist_id < output_data[i]) {
            max_deg_id = max_dist_id;
            max_dist_id = i;
        }
    }
    deg = ANGLE_DEG_MIN + max_deg_id * ANGLE_DEG_STEP;
    dist = DISTANCE_CM_MIN +
           (max_dist_id - (ANGLE_DEG_MAX - ANGLE_DEG_MIN) / ANGLE_DEG_STEP) *
               DISTANCE_CM_STEP;
    return 0;
}

int write_data_set() {
    int focal_length_pixels = FOCAL_LENGTH_UM_OV2640 *
                              WIDTH_SENSOR_PIXELS_OV2640 /
                              WIDTH_SENSOR_UM_OV2640;

    {
        int w_cm = 10;
        int h_cm = 10;
        int distance_cm = 20;
        int x_offset_cm = 0;
        int y_offset_cm = 0;
        int x_tilt_deg = 80;
        int y_tilt_deg = 0;

        for (int x_tilt_deg = ANGLE_DEG_MIN; x_tilt_deg < ANGLE_DEG_MAX;
             x_tilt_deg += ANGLE_DEG_STEP) {
            for (int distance_cm = DISTANCE_CM_MIN;
                 distance_cm < DISTANCE_CM_MAX;
                 distance_cm += DISTANCE_CM_STEP) {
                // int distance_cm = 20;
                // printf("distance: %i, tilt: %i\n", distance_cm, x_tilt_deg);
                // usleep(10000);
                Surface surf(w_cm, h_cm, distance_cm, x_offset_cm, y_offset_cm,
                             x_tilt_deg, 0);
                Cam cam(WIDTH_SENSOR_PIXELS_OV2640, HEIGHT_SENSOR_PIXELS_OV2640,
                        focal_length_pixels);
                cam.project(surf);
                Bmp *b = cam.render();
                char filename[100];
                sprintf(filename, "%s%sdeg_%i_dist_%i.bmp", DATA_FOLDER,
                        TEST_FILENAME_PREFIX, x_tilt_deg, distance_cm);
                b->to_grayscale();
                b->write(filename);
            }
        }
    }
    return 0;
}

int load_data(CNN &cnn) {
    Bmp bmp(WIDTH_SENSOR_PIXELS_OV2640, HEIGHT_SENSOR_PIXELS_OV2640);
    DIR *d;
    struct dirent *dir;
    d = opendir(DATA_FOLDER);
    char filename[500];
    int deg, dist;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strncmp(dir->d_name, TEST_FILENAME_PREFIX, 5) == 0) {
                sscanf(dir->d_name, "test_deg_%i_dist_%i", &deg, &dist);
                snprintf(filename, 500, "%s%s", DATA_FOLDER, dir->d_name);
                bmp.read(filename);
                size_t len;
                value_t output_data[OUTPUT_DATA_SIZE] = {};
                uint8_t input_data_u8[INPUT_DATA_SIZE] = {};
                value_t input_data[INPUT_DATA_SIZE] = {};
                bmp.get_raw_grayscale_data(len, input_data_u8);
                output_to_outdata(deg, dist, output_data);
                deg = 0, dist = 0;
                outdata_to_output(output_data, deg, dist);
                for (int i = 0; i < INPUT_DATA_SIZE; i++) {
                    input_data[i] = input_data_u8[i];
                }
                cnn.load_training_data(input_data, output_data);
            }
        }
        closedir(d);
    }
    return 0;
}
int dump(CNN *cnn, const char *filename) {
    value_t * avg_weight = (value_t *)malloc(sizeof(value_t) * cnn->num_inputs);
    Bmp bmp(320, 240);
    for (int i = 0; i< cnn->num_inputs; i++)  {
        avg_weight[i] = 0;
        for (int n = 0; n < cnn->num_hidden_nodes_per_layer[0]; n++) {
            avg_weight[i] += cnn->hidden_layers[0][n]->weights[i];
        }
        avg_weight[i] /= cnn->num_hidden_nodes_per_layer[0];
        PixelColor pixel_color((uint8_t) avg_weight[i]);
        bmp.set_pixel(i, pixel_color);
    }

    bmp.write(filename);
    return 0;
}

int main() {
    // Create artificial data set for training
    // write_data_set();

    // Create CNN
    int hidden_layers_nodes_list[] = {10};
    CNN cnn(WIDTH_SENSOR_PIXELS_OV2640 * HEIGHT_SENSOR_PIXELS_OV2640, 1,
            hidden_layers_nodes_list, OUTPUT_DATA_SIZE);
    cnn.init();

    // Load it
    load_data(cnn);

    if (cnn.load("weights_and_biases.bin") == -1) {
        cnn.train(10, 10);
        cnn.save("weights_and_biases.bin");
    }
    dump(&cnn, "result.bmp");

    Bmp bmp(WIDTH_SENSOR_PIXELS_OV2640, HEIGHT_SENSOR_PIXELS_OV2640);
    bmp.read("data/test_deg_230_dist_180.bmp");

    size_t len;
    value_t output_data[OUTPUT_DATA_SIZE] = {};
    uint8_t input_data_u8[INPUT_DATA_SIZE] = {};
    value_t input_data[INPUT_DATA_SIZE] = {};
    bmp.get_raw_grayscale_data(len, input_data_u8);
    
    
    for (int i = 0; i < INPUT_DATA_SIZE; i++) {
        input_data[i] = input_data_u8[i];
    }
    cnn.run(input_data, output_data);
    int deg = 0, dist = 0;
    outdata_to_output(output_data, deg, dist);
    
    return 0;
    /*
    int w = 320;
    int h = 240;
    w = 10;
    h = 10;

    Bmp bmp(w, h);
    PixelColor c;
    c.r = 0;
    c.b = 0;
    bmp.set_pixel(Point2d(0, 0, c));
    size_t len = 0;
    uint8_t buff[w*h];
    bmp.get_raw_grayscale_data(len, buff);
    bmp.to_grayscale();
    bmp.write("test.bmp");
    Bmp read(320, 240);
    read.read("test.bmp");
    Point2d p(0,0);
    p.color.clear();
    read.get_pixel(p);
    */
}