#pragma once
#include <JuceHeader.h>

struct params {
    /*
    this is meant to be used akin to a namespace. the reason it is not a namespace is so that there is a sanctioned way
    to access the relevant elements, but the containers for them are private. this may help in certain instances
    e.g. logging, debugging can be added to the access functionality.
    */
    params() = delete;
    ~params() = delete;
    params(const params &other) = delete;
    params(params &&other) noexcept = delete;
    params & operator=(const params &other) = delete;
    params & operator=(params &&other) noexcept = delete;


    using String = juce::String;
    enum class params_e {
        f0 = 0,
        num_params
    };

    template <typename E>
    [[nodiscard]] constexpr static auto to_idx(E e) noexcept {
        return static_cast<std::underlying_type_t<E>>(e);
    }

    [[nodiscard]] constexpr static params_e from_idx(int idx) {
        assert (idx >= 0 && idx < static_cast<int>(params_e::num_params));
        return static_cast<params_e>(idx);
    }

    [[nodiscard]] static String get_param_name(params_e const p) {
        return params_to_name_map.at(p);
    }
    [[nodiscard]] static String get_param_id(params_e const p) {
        return params_to_id_map.at(p);
    }
private:
    const static std::map<params_e, String> /*const*/ params_to_name_map;
    const static std::map<params_e, String> /*const*/ params_to_id_map;
};

