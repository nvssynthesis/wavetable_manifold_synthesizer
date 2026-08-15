#pragma once
#include <JuceHeader.h>
#include <sys/stat.h>

namespace params {


    using String = juce::String;
    enum class params_e {
        f0 = 0,
        voicedness,
/* 'cc' stands for Cepstral Coefficient, which could be either Bark (BFCC) or Mel (MFCC).
However, we are currently experimenting with using a dimensionally-reduced set of these, reducing e.g. 11 coefficients
down to 3. So even though these are still named 'cc', each one has some effect over all cepstral coefficients.
*/
        cc0,
        cc1,
        cc2,
        cc3,
        cc4,
        cc5,
        cc6,
        cc7,
        // cc8,
        // cc9,
        // cc10,
        num_params
    };



    [[nodiscard]] constexpr static params_e from_idx(int idx) {
        assert (idx >= 0 && idx < static_cast<int>(params_e::num_params));
        return static_cast<params_e>(idx);
    }

    template <typename E>
    [[nodiscard]] constexpr static std::underlying_type_t<E> to_idx(E e) noexcept {
        return static_cast<std::underlying_type_t<E>>(e);
    }

    static constexpr auto num_params = to_idx(params_e::num_params);
    static constexpr auto cc_offset = static_cast<int>(params_e::cc0);
    static constexpr auto num_cc_coeffs = static_cast<int>(params_e::num_params) - cc_offset;
    static_assert(num_cc_coeffs == 8);



    [[nodiscard]] String get_param_name(params_e p);
    [[nodiscard]] String get_param_id(params_e p);

namespace detail {

    const std::map<params_e, String> params_to_name_map = [](){
        std::map<params_e, String> tmp_map;
        String bfcc_prefix = "BFCC";
        for (int i = to_idx(params_e::cc0); i < to_idx(params_e::num_params); ++i) {
            auto tmp_s = bfcc_prefix;
            tmp_s += String(i);
            tmp_map[from_idx(i)] = tmp_s;
        }
        tmp_map[params_e::f0] = "Fundamental Frequency";
        tmp_map[params_e::voicedness] = "Voicedness";
        return tmp_map;
    }();
    const std::map<params_e, String> params_to_id_map = []() {
        std::map<params_e, String> tmp_map;
        String bfcc_prefix = "bfcc";
        for (int i = to_idx(params_e::cc0); i < to_idx(params_e::num_params); ++i) {
            auto tmp_s = bfcc_prefix;
            tmp_s += String(i);
            tmp_map[from_idx(i)] = tmp_s;
        }
        tmp_map[params_e::f0] = "f0";
        tmp_map[params_e::voicedness] = "voicedness";
        return tmp_map;
    }();

    const std::map<params_e, double> params_to_min_map = []() {
        std::map<params_e, double> tmp;
        for (int i = to_idx(params_e::cc0); i < to_idx(params_e::num_params); ++i) {
            tmp[from_idx(i)] = -5.0;    // common minimum for bfcc params
        }
        tmp[params_e::f0] = 20.0;
        tmp[params_e::voicedness] = 0.0;
        return tmp;
    }();

    const std::map<params_e, double> params_to_max_map = [](){
        std::map<params_e, double> tmp;
        for (int i = to_idx(params_e::cc0); i < to_idx(params_e::num_params); ++i) {
            tmp[from_idx(i)] = 5.0;    // common maximum for bfcc params
        }
        tmp[params_e::f0] = 4000.0;
        tmp[params_e::voicedness] = 1.0;
        return tmp;
    }();
    const std::map<params_e, double> params_to_skew_factor_map = [](){
        std::map<params_e, double> tmp;
        for (int i = to_idx(params_e::cc0); i < to_idx(params_e::num_params); ++i) {
            tmp[from_idx(i)] = 1.0;    // common skew factor for bfcc params
        }
        tmp[params_e::f0] = 0.25;
        tmp[params_e::voicedness] = 1.0;
        return tmp;
    }();
    const std::map<params_e, double> params_to_default_map = [](){
        std::map<params_e, double> tmp;
        for (int i = to_idx(params_e::cc0); i < to_idx(params_e::num_params); ++i) {
            tmp[from_idx(i)] = 0.0;    // common default for bfcc params
        }
        tmp[params_e::f0] = 110;
        tmp[params_e::voicedness] = 0.9;
        return tmp;
    }();
}


    template<typename float_t>
    [[nodiscard]] static juce::NormalisableRange<float_t> get_normalizable_range(params_e const p){
        auto min = static_cast<float_t>(detail::params_to_min_map.at(p));
        auto max = static_cast<float_t>(detail::params_to_max_map.at(p));

        float_t interval = 0.0;
        auto skew_factor = static_cast<float_t>(detail::params_to_skew_factor_map.at(p));
        return juce::NormalisableRange<float_t>(min, max, interval, skew_factor);
    }
    template<typename float_t>
    [[nodiscard]] static float_t get_default(params_e const p) {
        return static_cast<float_t>(detail::params_to_default_map.at(p));
    }

}
