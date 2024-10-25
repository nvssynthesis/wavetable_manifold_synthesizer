#include "params.h"

const std::map<params::params_e, String> params::params_to_name_map = {
     {params::params_e::f0, "fundamental_frequency"}
};

const std::map<params::params_e, String> params::params_to_id_map = {
     {params::params_e::f0, "f0"}
};