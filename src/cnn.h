#ifndef __CNN_H__
#define __CNN_H__

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#define MAX_HIDDEN_LAYERS 100
#define MAX_OUTPUTS 100
#define MAX_INPUTS 100000
#define MAX_TRAINING_DATA 100000
#define QUANTIZED_BITS 10
#define QUANTIZED_VALUES (((long int)1) << QUANTIZED_BITS)
#define MAX_NEURON_VAL ((((long int)1) << (QUANTIZED_BITS - 1)) - 1)
#define MIN_NEURON_VAL (-(((long int)1) << (QUANTIZED_BITS - 1)))
#define RELU_LEAK_FACTOR_PERCENT 4
#define LEARNING_RATE_PER_1000 1
#define WEIGHT_BIAS_SCALE_DOWN_FACTOR 2
// Makes the CNN training reproducible
//#define FIX_RANDOMNESS

#define ZERO 0
#define ONE (MAX_NEURON_VAL / 8)

// Normalize to the quantized admitted bounds within each individual step layer
// You want the CNN layers values to NOT exceed the quantized space (e.g. 8bits)
#define LAYERS_QUANTIZED_NORMALIZATION

// Use integers/floats for the weights and biases
// #define USE_FLOATS

#ifdef USE_FLOATS
typedef float value_t;
#else
typedef long int value_t;
#endif

/* e.g (1<<31) - 1 - (256*256) */
#define OVERFLOW_BOUND                                                      \
    ((value_t)(((unsigned long int)1 << ((sizeof(value_t) * 8) - 1)) - 1) - \
     (MAX_NEURON_VAL * MAX_NEURON_VAL))

class Neuron {
   public:
    value_t value = 0;
    value_t bias = 0;
    value_t* weights = NULL;
    value_t error = 0;
    value_t bias_gradient = 0;
    value_t* weight_gradient = NULL;
    int num_inputs = 0;
    Neuron(int num_inputs) : num_inputs(num_inputs) {
        if (num_inputs > 0) {
            weights = (value_t*)calloc(1, sizeof(value_t) * num_inputs);
            weight_gradient = (value_t*)calloc(1, sizeof(value_t) * num_inputs);
        }
    }
    ~Neuron() { free(weights); }
};

class TrainingData {
   public:
    value_t* inputs;
    Neuron* outputs[MAX_OUTPUTS];
    TrainingData(int input_size, value_t* _inputs, int output_size,
                 value_t* _outputs) {
        inputs = (value_t*)malloc(sizeof(value_t) * input_size);
        memcpy(inputs, _inputs, sizeof(value_t) * input_size);
        for (int i = 0; i < output_size; i++) {
            outputs[i] = new Neuron(0);
            outputs[i]->value = _outputs[i];
        }
    };
};

class CNN {
   public:
    typedef int (*node_iterate_fwd_cbk_t)(Neuron* n, int prev_layer_neurons,
                                          Neuron** prev_layer, void* arg);
    typedef int (*layer_iterate_fwd_cbk_t)(int layer_neurons, Neuron** layer,
                                           int prev_layer_neurons,
                                           Neuron** prev_layer, void* arg);

   public:
    int num_inputs = 0;
    int num_hidden_layers = 0;
    int num_hidden_nodes_per_layer[MAX_HIDDEN_LAYERS] = {};
    int num_outputs = 0;
    int training_data_amount = 0;
    Neuron* input_layer[MAX_INPUTS] = {};
    Neuron** hidden_layers[MAX_HIDDEN_LAYERS] = {};
    Neuron* output_layer[MAX_OUTPUTS] = {};
    TrainingData* training_data[MAX_TRAINING_DATA] = {};
    value_t overall_mean_out_error = 0;

   public:
    CNN(int num_inputs, int num_hidden_layers, int* num_hidden_nodes_per_layer,
        int num_outputs);
    ~CNN();
    int init();
    int load_training_data(value_t* data_inputs, value_t* data_outputs);
    int save(const char* file);
    int load(const char* file);
    int load_inputs(value_t* inputs);
    int forward_pass();
    /**
     * Train for number of epochs
     * An epoch is an iteration over all the elements of the dataset.
     * During an epoch the entire dataset is iterated in batches of
     * size `batches_size` until error converges under max_error_percent.
     * The dataset is shuffled to pick from it randomly.
     * Inputs context (value_batch_sum) are cleared
     */
    int train(int epochs, int batches_size, int max_error_percent);
    int run(value_t* data_inputs, value_t* data_outputs);
    int calc_overall_mean_out_error();
    int print();
    int rand_weight_or_bias();

   private:
    int clear_inputs_context();

    int iterate(int (*action)(Neuron* n, int prev_num_inputs, void** arg),
                void** arg);
    int layer_iterate_fwd(layer_iterate_fwd_cbk_t action, void* arg);
    int node_iterate_fwd(node_iterate_fwd_cbk_t action, void* arg);

    value_t relu(value_t x);
    value_t drelu_reciprocal(value_t x);
    int calc_layer(int prev_layer_inputs, Neuron** prev_layer, int layer_inputs,
                   Neuron** layer);
    int calc_output_error(int layer_neurons, Neuron** layer,
                          Neuron** expected_values);
    int back_propagate_errors(int layer_neurons, Neuron** layer,
                              int prev_layer_neurons, Neuron** prev_layer);
    static int accumilate_gradients(Neuron* n, int prev_layer_neurons,
                                    Neuron** prev_layer, void* arg);
    static int apply_and_clear_gradients(int layer_neurons, Neuron** layer,
                                         int prev_layer_neurons,
                                         Neuron** prev_layer, void * batches_size);
};

#endif