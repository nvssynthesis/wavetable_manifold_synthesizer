#include "params.h"

const std::map<params::params_e, String> params::params_to_name_map = {
     {params::params_e::f0, "fundamental_frequency"},
     {params::params_e::cc0, "BFCC_0"},
     {params::params_e::cc1, "BFCC_1"},
     {params::params_e::cc2, "BFCC_2"},
     {params::params_e::cc3, "BFCC_3"},
     {params::params_e::cc4, "BFCC_4"}
};

const std::map<params::params_e, String> params::params_to_id_map = {
     {params::params_e::f0, "f0"},
     {params::params_e::cc0, "bfcc0"},
     {params::params_e::cc1, "bfcc1"},
     {params::params_e::cc2, "bfcc2"},
     {params::params_e::cc3, "bfcc3"},
     {params::params_e::cc4, "bfcc4"},
};

// const std::map<params::params_e, double> params::params_to_min_map = {
//      {params::params_e::f0, 12.0},
//      {params::params_e::cc0, -2.0},
//      {params::params_e::cc1, -2.0},
//      {params::params_e::cc2, -2.0},
//      {params::params_e::cc3, -2.0},
//      {params::params_e::cc4, -2.0}
// };
const std::map<params::params_e, double> params::params_to_min_map = []() {
     std::map<params::params_e, double> temp_map;
     // Default value for most parameters
     double default_value = -2.0;
     // Insert all `cc` parameters with the default value
     for (int i = to_idx(params_e::cc0); i <= to_idx(params_e::cc4); ++i) {
          temp_map[static_cast<params::params_e>(i)] = default_value;
     }
     // Customize specific parameter values
     temp_map[params::params_e::f0] = 12.0;
     return temp_map;
}();

const std::map<params::params_e, double> params::params_to_max_map = {
     {params::params_e::f0, 2400.0},
     {params::params_e::cc0, 2.0},
     {params::params_e::cc1, 2.0},
     {params::params_e::cc2, 2.0},
     {params::params_e::cc3, 2.0},
     {params::params_e::cc4, 2.0},
};
const std::map<params::params_e, double> params::params_to_skew_factor_map = {
     {params::params_e::f0, 0.25},
     {params::params_e::cc0, 1.0},
     {params::params_e::cc1, 1.0},
     {params::params_e::cc2, 1.0},
     {params::params_e::cc3, 1.0},
     {params::params_e::cc4, 1.0},
};
const std::map<params::params_e, double> params::params_to_default_map = {
     {params::params_e::f0, 55.0},
     {params::params_e::cc0, 0.0},
     {params::params_e::cc1, 0.0},
     {params::params_e::cc2, 0.0},
     {params::params_e::cc3, 0.0},
     {params::params_e::cc4, 0.0},
};


