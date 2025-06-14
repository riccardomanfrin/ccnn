#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cnn.h"

#define INPUT_SAMPLES 4

int load_data(CNN &cnn) {
    value_t data_inputs[4][2] = {
        {ZERO, ZERO}, {ZERO, ONE}, {ONE, ZERO}, {ONE, ONE}};
    value_t data_outputs[4][1] = {{ZERO}, {ONE}, {ONE}, {ZERO}};
    for (int i = 0; i < INPUT_SAMPLES; i++) {
        cnn.load_training_data(data_inputs[i], data_outputs[i]);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int hidden_layers_nodes_list[] = {8};
    CNN cnn(2, 1, hidden_layers_nodes_list, 1);
    cnn.init();
    cnn.load("weights_and_biases.bin");

    if (argc > 1) {
        if (strcmp(argv[1], "test") == 0) {
            value_t input_data[2] = {ZERO, ZERO};
            value_t output_data[1] = {255};
            if (argc == 4) {
                input_data[0] = atof(argv[2]);
                input_data[1] = atof(argv[3]);
            }
            cnn.run(input_data, output_data);
#ifdef USE_FLOATS
            printf("%f\n", output_data[0]);
#else
            printf("%li\n", output_data[0]);
#endif
            return 0;
        } else if (strcmp(argv[1], "print") == 0) {
            cnn.print();
        }
    } else {
        load_data(cnn);
        printf(
            "oe, hw[0][0], hw[0][1], hw[1][0], hw[1][1], ow[0][0], ow[0][0], v00, "
            "v01, v10, v11\n");
        cnn.train(45000, 1, 1);
        
        cnn.save("weights_and_biases.bin");
    }
    return 0;
}
