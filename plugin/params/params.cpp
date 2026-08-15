#include "params.h"

namespace params {


[[nodiscard]] String get_param_name(params_e const p) {
    return detail::params_to_name_map.at(p);
}
[[nodiscard]] String get_param_id(params_e const p) {
    return detail::params_to_id_map.at(p);
}



}   // namespace params


