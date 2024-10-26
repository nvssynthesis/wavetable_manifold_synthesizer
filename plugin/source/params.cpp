#include "params.h"

const std::map<params::params_e, String> params::params_to_name_map = [](){
     std::map<params::params_e, String> tmp_map;
     String bfcc_prefix = "BFCC";
     for (int i = to_idx(params_e::cc0); i <= to_idx(params_e::cc10); ++i) {
          auto tmp_s = bfcc_prefix;
          tmp_s += String(i);
          tmp_map[params::from_idx(i)] = tmp_s;
     }
     tmp_map[params_e::f0] = "Fundamental Frequency";
     return tmp_map;
}();
const std::map<params::params_e, String> params::params_to_id_map = []() {
     std::map<params::params_e, String> tmp_map;
     String bfcc_prefix = "bfcc";
     for (int i = to_idx(params_e::cc0); i <= to_idx(params_e::cc10); ++i) {
          auto tmp_s = bfcc_prefix;
          tmp_s += String(i);
          tmp_map[params::from_idx(i)] = tmp_s;
     }
     tmp_map[params_e::f0] = "f0";
     return tmp_map;
}();

const std::map<params::params_e, double> params::params_to_min_map = []() {
     std::map<params::params_e, double> tmp;
     for (int i = to_idx(params_e::cc0); i <= to_idx(params_e::cc10); ++i) {
          tmp[params::from_idx(i)] = -2.0;    // common minimum for bfcc params
     }
     tmp[params::params_e::f0] = 12.0;
     return tmp;
}();

const std::map<params::params_e, double> params::params_to_max_map = [](){
     std::map<params::params_e, double> tmp;
     for (int i = to_idx(params_e::cc0); i <= to_idx(params_e::cc10); ++i) {
          tmp[params::from_idx(i)] = 2.0;    // common maximum for bfcc params
     }
     tmp[params::params_e::f0] = 2400.0;
     return tmp;
}();
const std::map<params::params_e, double> params::params_to_skew_factor_map = [](){
     std::map<params::params_e, double> tmp;
     for (int i = to_idx(params_e::cc0); i <= to_idx(params_e::cc10); ++i) {
          tmp[params::from_idx(i)] = 1.0;    // common skew factor for bfcc params
     }
     tmp[params::params_e::f0] = 0.25;
     return tmp;
}();
const std::map<params::params_e, double> params::params_to_default_map = [](){
     std::map<params::params_e, double> tmp;
     for (int i = to_idx(params_e::cc0); i <= to_idx(params_e::cc10); ++i) {
          tmp[params::from_idx(i)] = 0.0;    // common default for bfcc params
     }
     tmp[params::params_e::f0] = 55.0;
     return tmp;
}();


