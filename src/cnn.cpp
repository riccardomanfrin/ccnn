#include "cnn.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static inline int shuffle(int* array, size_t n) {
    for (int i = 0; i < n; i++) {
        size_t j = rand() % n;
        int tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
    return 0;
}
static inline value_t absval(value_t val) {
    if (val < 0) return -val;
    return val;
}

CNN::CNN(int num_inputs, int num_hidden_layers,
         int* _num_hidden_nodes_per_layer, int num_outputs)
    : num_inputs(num_inputs),
      num_hidden_layers(num_hidden_layers),
      num_outputs(num_outputs) {
    memcpy(num_hidden_nodes_per_layer, _num_hidden_nodes_per_layer,
           num_hidden_layers * sizeof(int));

    for (int i = 0; i < num_inputs; i++) {
        input_layer[i] = new Neuron(0);
    }
    int prev_layer_nodes = num_inputs;
    for (int l = 0; l < num_hidden_layers; l++) {
        int curr_layer_num_hidden_nodes = num_hidden_nodes_per_layer[l];
        hidden_layers[l] =
            (Neuron**)malloc(sizeof(Neuron**) * curr_layer_num_hidden_nodes);

        for (int n = 0; n < curr_layer_num_hidden_nodes; n++) {
            hidden_layers[l][n] = new Neuron(prev_layer_nodes);
        }
        prev_layer_nodes = curr_layer_num_hidden_nodes;
    }
    for (int k = 0; k < num_outputs; k++) {
        output_layer[k] =
            new Neuron(num_hidden_nodes_per_layer[num_hidden_layers - 1]);
    }
#ifndef FIX_RANDOMNESS
    srand(time(NULL));
#endif
}

int CNN::init() {
    int prev_num_inputs = num_inputs;
    for (int l = 0; l < num_hidden_layers; l++) {
        for (int n = 0; n < num_hidden_nodes_per_layer[l]; n++) {
            for (int i = 0; i < prev_num_inputs; i++) {
                hidden_layers[l][n]->weights[i] = rand_weight_or_bias();
            }
            hidden_layers[l][n]->bias = rand_weight_or_bias();
        }
        prev_num_inputs = num_hidden_nodes_per_layer[l];
    }

    for (int k = 0; k < num_outputs; k++) {
        for (int i = 0; i < prev_num_inputs; i++) {
            output_layer[k]->weights[i] = rand_weight_or_bias();
        }
        output_layer[k]->bias = rand_weight_or_bias();
    }
    return 0;
}

CNN::~CNN() {
    for (int i = 0; i < num_inputs; i++) {
        delete input_layer[i];
    }
    for (int l = 0; l < num_hidden_layers; l++) {
        for (int n = 0; n < num_hidden_nodes_per_layer[l]; n++) {
            delete hidden_layers[l][n];
        }
        delete hidden_layers[l];
    }
    for (int k = 0; k < num_outputs; k++) {
        delete output_layer[k];
    }
}

int CNN::load_training_data(value_t* data_inputs, value_t* data_outputs) {
    TrainingData* t =
        new TrainingData(num_inputs, data_inputs, num_outputs, data_outputs);
    training_data[training_data_amount] = t;
    training_data_amount++;
    return 0;
}

static int serialize_cnn_params(Neuron* n, int prev_num_inputs, void** arg) {
    value_t* buff = *(value_t**)arg;
    int i = 0;
    for (; i < prev_num_inputs; i++) {
        buff[i] = n->weights[i];
    }
    buff[i] = n->bias;
    *arg = (void*)&buff[++i];
    return 0;
}

int CNN::save(const char* file) {
    FILE* f = fopen(file, "w");
    value_t* data = (value_t*)calloc(1000000, sizeof(value_t));
    value_t* pointer = data;
    iterate(&serialize_cnn_params, ((void**)&pointer));
    size_t s = (pointer - data) * sizeof(value_t);
    fwrite(data, s, 1, f);
    fclose(f);
    free(data);
    return 0;
}

static int deserialize_cnn_params(Neuron* n, int prev_num_inputs, void** arg) {
    FILE* f = *(FILE**)arg;
    value_t* v = (value_t*)malloc(sizeof(value_t) * prev_num_inputs + 1);

    if (prev_num_inputs + 1 !=
        fread((void*)v, sizeof(value_t), prev_num_inputs + 1, f)) {
        return -1;
    }
    int i = 0;
    for (; i < prev_num_inputs; i++) {
        n->weights[i] = v[i];
    }
    n->bias = v[i];
    free(v);
    return 0;
}

int CNN::load(const char* file) {
    FILE* f = fopen(file, "r");
    if (f == NULL) return -1;
    iterate(&deserialize_cnn_params, ((void**)&f));
    fclose(f);
    return 0;
}

int average_weights_and_bias(Neuron* n, int prev_num_inputs, void** arg) {
    int batches_size = *(int*)(*arg);
    for (int i = 0; i < prev_num_inputs; i++) {
        n->weights[i] /= batches_size;
    }
    n->bias /= batches_size;
    return 0;
}
int clear_minibatch_context(Neuron* n, int prev_num_inputs, void** _arg) {
    n->value = 0;
    n->error = 0;
    n->bias_gradient = 0;
    for (int i = 0; i < n->num_inputs; i++) {
        n->weight_gradient[i] = 0;
    }
    return 0;
}

int CNN::iterate(int (*action)(Neuron* n, int prev_num_inputs, void** arg),
                 void** arg) {
    int prev_num_inputs = num_inputs;
    for (int l = 0; l < num_hidden_layers; l++) {
        for (int n = 0; n < num_hidden_nodes_per_layer[l]; n++) {
            action(hidden_layers[l][n], prev_num_inputs, arg);
        }
        prev_num_inputs = num_hidden_nodes_per_layer[l];
    }
    for (int k = 0; k < num_outputs; k++) {
        action(output_layer[k], prev_num_inputs, arg);
    }
    return 0;
}

int CNN::layer_iterate_fwd(layer_iterate_fwd_cbk_t action, void* arg) {
    Neuron** prev_layer = input_layer;
    int prev_layer_neurons = num_inputs;
    for (int l = 0; l < num_hidden_layers; l++) {
        Neuron** layer = hidden_layers[l];
        int layer_neurons = num_hidden_nodes_per_layer[l];
        action(layer_neurons, layer, prev_layer_neurons, prev_layer, arg);
        prev_layer = layer;
        prev_layer_neurons = layer_neurons;
    }
    action(num_outputs, output_layer, prev_layer_neurons, prev_layer, arg);
    return 0;
}

int CNN::node_iterate_fwd(node_iterate_fwd_cbk_t action, void* arg) {
    Neuron** prev_layer = input_layer;
    int prev_layer_neurons = num_inputs;
    for (int l = 0; l < num_hidden_layers; l++) {
        Neuron** layer = hidden_layers[l];
        int layer_neurons = num_hidden_nodes_per_layer[l];
        for (int k = 0; k < layer_neurons; k++) {
            action(layer[k], prev_layer_neurons, prev_layer, arg);
        }
        prev_layer = layer;
        prev_layer_neurons = layer_neurons;
    }
    for (int k = 0; k < num_outputs; k++) {
        action(output_layer[k], prev_layer_neurons, prev_layer, arg);
    }
    return 0;
}

int CNN::load_inputs(value_t* input_data) {
    Neuron** inputs = input_layer;
    for (int i = 0; i < num_inputs; i++) {
        inputs[i]->value = input_data[i];
    }
    return 0;
}

int CNN::forward_pass() {
    int prev_num_inputs = num_inputs;
    Neuron** inputs = input_layer;
    int l = 0;
    // and for each hidden layer
    // compute Hidden layers activation
    for (; l < num_hidden_layers; l++) {
        calc_layer(prev_num_inputs, inputs, num_hidden_nodes_per_layer[l],
                   hidden_layers[l]);

        prev_num_inputs = num_hidden_nodes_per_layer[l];
        inputs = hidden_layers[l];
    }

    // now for the output layer
    calc_layer(prev_num_inputs, inputs, num_outputs, output_layer);
    return 0;
}
int CNN::train(int epochs, int batches_size, int max_error_percent) {
    fprintf(stderr,
            "Creating CNN with %i inputs, %i hidden layers and %i outputs\n",
            num_inputs, num_hidden_layers, num_outputs);
    fprintf(stderr, "Min neuron val: %li, Max neuron val: %li\n",
            MIN_NEURON_VAL, MAX_NEURON_VAL);
    fprintf(stderr, "Quantized values: %li\n", QUANTIZED_VALUES);

    // Define array to pick random data from dataset
    int order[training_data_amount];
    for (int i = 0; i < training_data_amount; i++) {
        order[i] = i;
    }

    bool converged = false;
    // For each epoch (that is, a complete iteration on the data set),
    for (int e = 0; e < epochs; e++) {
        if ((e % 100) == 0) {
            calc_overall_mean_out_error();
        }
        if (overall_mean_out_error <=
                MAX_NEURON_VAL * max_error_percent / 100 &&
            !converged) {
            converged = true;
            epochs = e * 105 / 100;
            fprintf(stderr,
                    "\033[32mConverged below %i percent error in %i "
                    "epochs\033[0m\n",
                    max_error_percent, e);
        }
        shuffle(order, training_data_amount);

        // for each data set element,
        for (int d = 0; d < training_data_amount;) {
            iterate(clear_minibatch_context, NULL);
            int b = 0;
            // and picking from a smaller number of mini batches,
            for (; b < batches_size && d + b < training_data_amount; b++) {
                /**********************/
                /**** Forward pass ****/
                /**********************/

                // initialize the network input values from a training data
                // sample
                TrainingData* td = training_data[order[d + b]];
                load_inputs(td->inputs);

                forward_pass();

                /******************/
                /**** Backprop ****/
                /******************/

                // Compute error in output weights
                calc_output_error(num_outputs, output_layer, td->outputs);

                // Compute error in backard hidden layers
                Neuron** layer = output_layer;
                int layer_neurons = num_outputs;
                for (int l = num_hidden_layers - 1; l >= 0; l--) {
                    Neuron** prev_layer = hidden_layers[l];
                    int prev_layer_neurons = num_hidden_nodes_per_layer[l];
                    back_propagate_errors(layer_neurons, layer,
                                          prev_layer_neurons, prev_layer);
                    layer = prev_layer;
                    layer_neurons = prev_layer_neurons;
                }

                // Apply changes in output & hidden nodes weights and biases
                node_iterate_fwd(&accumilate_gradients, NULL);

            }  // minibatch

            layer_iterate_fwd(apply_and_clear_gradients, (void*)&batches_size);

            d += b;
            if (e % (100) == 0) {
                print();
            }
        }  // training data
    }  // epochs
    return 0;
}

int CNN::accumilate_gradients(Neuron* n, int prev_layer_neurons,
                              Neuron** prev_layer, void* _arg) {
    n->bias_gradient += n->error;
    for (int j = 0; j < prev_layer_neurons; j++) {
        n->weight_gradient[j] += n->error * prev_layer[j]->value;
    }
    return 0;
}
static void acc(value_t& base, value_t val) {
    val = absval(val);
    if (base < val) base = val;
}

int CNN::apply_and_clear_gradients(int layer_neurons, Neuron** layer,
                                   int prev_layer_neurons, Neuron** prev_layer,
                                   void* arg) {
    int batches_size = *(int*)arg;

    for (int k = 0; k < layer_neurons; k++) {
        layer[k]->bias += layer[k]->bias_gradient * LEARNING_RATE_PER_1000 /
                          1000 / batches_size;

        layer[k]->bias_gradient = 0;
        
        for (int j = 0; j < prev_layer_neurons; j++) {
            layer[k]->weights[j] += layer[k]->weight_gradient[j] *
                                    LEARNING_RATE_PER_1000 / 1000 /
                                    batches_size;
            layer[k]->weight_gradient[j] = 0;
        }
    }

    return 0;
}

int CNN::back_propagate_errors(int layer_neurons, Neuron** layer,
                               int prev_layer_neurons, Neuron** prev_layer) {
    value_t prev_layer_neurons_delta_error[prev_layer_neurons];

    // backprop goes from "layer" to "prev_layer"
    for (int j = 0; j < prev_layer_neurons; j++) {
        prev_layer_neurons_delta_error[j] = 0;

        for (int k = 0; k < layer_neurons; k++) {
            prev_layer_neurons_delta_error[j] +=
                layer[k]->error * layer[k]->weights[j];
        }
    }

    for (int j = 0; j < prev_layer_neurons; j++) {
        prev_layer[j]->error = prev_layer_neurons_delta_error[j] /
                               drelu_reciprocal(prev_layer[j]->value);
    }

    return 0;
}

int CNN::calc_output_error(int layer_neurons, Neuron** layer,
                           Neuron** expected_values) {
    // Compute change in output weights
    for (int k = 0; k < layer_neurons; k++) {
        layer[k]->error = (expected_values[k]->value - layer[k]->value) /
                          drelu_reciprocal(layer[k]->value);
    }
    return 0;
}

int CNN::calc_layer(int prev_layer_neurons, Neuron** prev_layer,
                    int layer_neurons, Neuron** layer) {
    // and each of its nodes
    for (int n = 0; n < layer_neurons; n++) {
        Neuron* curr = layer[n];
        curr->value = curr->bias;
        for (int i = 0; i < prev_layer_neurons; i++) {
            if (prev_layer[i]->value != 0) {
                curr->value +=
                    curr->weights[i] * prev_layer[i]->value / MAX_NEURON_VAL;
            }

            /* (1<<31) - 1 - (256*256) */
            if (curr->value > OVERFLOW_BOUND ||
                curr->value < (value_t)(-OVERFLOW_BOUND)) {
                throw "Potential overflow!!!!";
            }
        }
    }

    for (int n = 0; n < layer_neurons; n++) {
        Neuron* curr = layer[n];
        curr->value = relu(curr->value);
    }
    return 0;
}

int CNN::run(value_t* data_inputs, value_t* data_outputs) {
    load_inputs(data_inputs);
    forward_pass();
    for (int i = 0; i < num_outputs; i++) {
        data_outputs[i] = output_layer[i]->value;
    }
    return 0;
}
int CNN::calc_overall_mean_out_error() {
    overall_mean_out_error = 0;
    value_t output_data[num_outputs];
    for (int d = 0; d < training_data_amount; d++) {
        run(training_data[d]->inputs, output_data);
        for (int i = 0; i < num_outputs; i++) {
            overall_mean_out_error +=
                absval(output_data[i] - training_data[d]->outputs[i]->value);
        }
    }
    overall_mean_out_error /= (training_data_amount * num_outputs);
    return overall_mean_out_error;
}

int CNN::print() {
    value_t data_inputs[4][2] = {
        {ZERO, ZERO}, {ZERO, ONE}, {ONE, ZERO}, {ONE, ONE}};
    value_t data_outputs[4][1] = {{ZERO}, {ONE}, {ONE}, {ZERO}};
    for (int i = 0; i < 4; i++) {
        run(data_inputs[i], data_outputs[i]);
    }
#ifdef USE_FLOATS
    printf("%f, %f, %f, %f, %f, %f, %f, ", overall_mean_out_error,
           hidden_layers[0][0]->weights[0], hidden_layers[0][0]->weights[1],
           hidden_layers[0][1]->weights[0], hidden_layers[0][1]->weights[1],
           output_layer[0]->weights[0], output_layer[0]->weights[1]);
    printf("%f, %f, %f, %f\n", data_outputs[0][0], data_outputs[1][0],
           data_outputs[2][0], data_outputs[3][0]);
#else
    printf("%li, %li, %li, %li, %li, %li, %li, ", overall_mean_out_error,
           hidden_layers[0][0]->weights[0], hidden_layers[0][0]->weights[1],
           hidden_layers[0][1]->weights[0], hidden_layers[0][1]->weights[1],
           output_layer[0]->weights[0], output_layer[0]->weights[1]);
    printf("%li, %li, %li, %li\n", data_outputs[0][0], data_outputs[1][0],
           data_outputs[2][0], data_outputs[3][0]);
#endif
    return 0;
}

int CNN::rand_weight_or_bias() {
    return ((rand() % QUANTIZED_VALUES) - MAX_NEURON_VAL) /
           WEIGHT_BIAS_SCALE_DOWN_FACTOR;
}

value_t CNN::relu(value_t x) {
    if (x < 0) return x / RELU_LEAK_FACTOR_PERCENT;
    return x;
}

value_t CNN::drelu_reciprocal(value_t x) {
    if (x < 0) return RELU_LEAK_FACTOR_PERCENT;
    return 1;
}
