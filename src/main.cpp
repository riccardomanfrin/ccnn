#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cnn.h"
#include "cnn_state_controller.h"

int load_data(CNN& cnn) {
    value_t data_inputs[4][2] = {
        {ZERO, ZERO}, {ZERO, ONE}, {ONE, ZERO}, {ONE, ONE}};
    value_t data_outputs[4][1] = {{ZERO}, {ONE}, {ONE}, {ZERO}};
    for (int i = 0; i < 4; i++) {
        cnn.load_training_data(data_inputs[i], data_outputs[i]);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    int hidden_layers_nodes_list[] = {10};
    CNN cnn(2, 1, hidden_layers_nodes_list, 1);
    cnn.init();
    load_data(cnn);
    printf(
        "oe, hw[0][0], hw[0][1], hw[1][0], hw[1][1], ow[0][0], ow[0][1], v00, "
        "v01, v10, v11\n");
    CNNStateController exporter(cnn);

    return 0;
}