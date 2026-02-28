

#include "cnn_state_controller.h"

char buffer[100000];

CNNStateController::CNNStateController(CNN& _cnn) : cnn(_cnn) {
    config["inputs"] = cnn.num_inputs;
    config["hidden_layers"] = cnn.num_hidden_layers;
    config["nodes_per_hidden"] = cnn.num_hidden_nodes_per_layer;
    config["outputs"] = cnn.num_outputs;
    config["ranges"] = {
        {"value_min", MIN_NEURON_VAL},  {"value_max", MAX_NEURON_VAL},
        {"bias_min", MIN_NEURON_VAL},   {"bias_max", MAX_NEURON_VAL},
        {"weight_min", MIN_NEURON_VAL}, {"weight_max", MAX_NEURON_VAL}};

    svr.Get("/config", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(config.dump(), "application/json");
    });

    svr.Post("/run",
             [this](const httplib::Request& req, httplib::Response& res) {
                 auto j = json::parse(req.body);
                 value_t* input_data =
                     (value_t*)malloc(sizeof(value_t) * cnn.num_inputs);
                 for (int i = 0; i < cnn.num_inputs; i++) {
                     input_data[i] = j["inputs"][i];
                 }

                 value_t* output_data =
                     (value_t*)malloc(sizeof(value_t) * cnn.num_outputs);

                 cnn.run(input_data, output_data);
                 json state = {{"outputs", json::array()}};
                 for (int i = 0; i < cnn.num_outputs; i++) {
                     state["outputs"].push_back(output_data[i]);
                 }
                 free(input_data);
                 free(output_data);
                 res.set_content(state.dump(), "application/json");
             });

    svr.Post("/train",
             [this](const httplib::Request& req, httplib::Response& res) {
                 auto j = json::parse(req.body);
                 cnn.train(j["epochs"], j["batch_size"], 1);
                 json state = get_state();
                 res.set_content(state.dump(), "application/json");
             });

    svr.Post("/save",
             [this](const httplib::Request& req, httplib::Response& res) {
                 cnn.save("weights_and_biases.bin");
                 json resp = {{"ok", true}};
                 res.set_content(resp.dump(), "application/json");
             });

    svr.Post("/load",
             [this](const httplib::Request& req, httplib::Response& res) {
                 cnn.load("weights_and_biases.bin");
                 json state = get_state();
                 res.set_content(state.dump(), "application/json");
             });

    svr.Post("/reset",
             [this](const httplib::Request& req, httplib::Response& res) {
                 cnn.init();
                 json state = get_state();
                 res.set_content(state.dump(), "application/json");
             });

    svr.Get("/", [this](const httplib::Request&, httplib::Response& res) {
        printf("Loading static content\n");
        load_static(buffer);
        res.set_content(buffer, "text/html");
    });

    fprintf(stderr, "Starting server at http://0.0.0.0:8080\n");
    svr.listen("0.0.0.0", 8080);
}

static void load_layer_state(int prev_layer_neurons, int curr_layer_neurons,
                             Neuron** layer_neurons, json& layer) {
    for (int n = 0; n < curr_layer_neurons; n++) {
        json node_weights = json::array();
        for (int w = 0; w < prev_layer_neurons; w++) {
            node_weights.push_back(layer_neurons[n]->weights[w]);
        }
        layer.push_back({{"value", layer_neurons[n]->value},
                         {"bias", layer_neurons[n]->bias},
                         {"weights", node_weights}});
    }
}
json CNNStateController::get_state() {
    json state;
    state["overall_mean_out_error"] = cnn.overall_mean_out_error;
    state["inputs"] = json::array();
    for (int i = 0; i < cnn.num_inputs; i++) {
        state["inputs"].push_back(cnn.input_layer[i]->value);
    }
    state["layers"] = json::array();
    int prev_layer_neurons = cnn.num_inputs;
    for (int l = 0; l < cnn.num_hidden_layers; l++) {
        int curr_layer_neurons = cnn.num_hidden_nodes_per_layer[l];
        Neuron** layer_neurons = cnn.hidden_layers[l];

        json layer = json::array();
        load_layer_state(prev_layer_neurons, curr_layer_neurons, layer_neurons,
                         layer);
        prev_layer_neurons = curr_layer_neurons;
        state["layers"].push_back(layer);
    }
    json layer = json::array();
    load_layer_state(prev_layer_neurons, cnn.num_outputs, cnn.output_layer,
                     layer);
    state["layers"].push_back(layer);
    return state;
}

int CNNStateController::load_static(char* buffer) {
    FILE* file;

    // Open the file in read mode
    file = fopen("index.html", "r");

    // Check if the file opened successfully
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // Read and print the file line by line
    while (fread(buffer, 100, 1, file) == 1) {
        buffer += 100;
    }

    // Close the file
    fclose(file);

    return 0;
}