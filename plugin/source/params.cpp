#include "params.h"

const std::map<params::params_e, String> params::params_to_name_map = {
     {params::params_e::f0, "fundamental_frequency"}
};

const std::map<params::params_e, String> params::params_to_id_map = {
     {params::params_e::f0, "f0"}
};

const std::map<params::params_e, double> params::params_to_min_map = {
     {params::params_e::f0, 12.0}
};

const std::map<params::params_e, double> params::params_to_max_map = {
     {params::params_e::f0, 12000.0}
};

const std::map<params::params_e, double> params::params_to_default_map = {
     {params::params_e::f0, 110.0}
};
