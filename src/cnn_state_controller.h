#include <string>

#include "deps/httplib.h"
#include "deps/json.hpp"
#include "cnn.h"

using nlohmann::json_abi_v3_12_0::json;

class CNNStateController {
   public:
    httplib::Server svr;
    json config;
    CNN &cnn;

    CNNStateController(CNN &_cnn);
private:
    json get_state();
    static int load_static(char * buffer);
};