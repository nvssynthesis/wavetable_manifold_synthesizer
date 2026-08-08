#pragma once
#include <array>

template<typename T>
T wrap01(T x) {
    /*
     this comes from my nvs_libraries/include/nvs_gen.h. if i end up needing more from it, just link it.
     */
    T y = x - static_cast<long long int>(x);
    if (x < 0){
        y = static_cast<T>(1) + y;
    }
    return y;
}

inline double process_phase_for_track(double phase, size_t const window_idx, size_t const num_windows) {
    double const window_gap = static_cast<double>(window_idx) / static_cast<double>(num_windows);
    phase -= window_gap;
    phase = wrap01<double>(phase);
    return phase;
}
inline double hanning(double const phase) {
    return 0.5 * (1.0 - std::cos(2.0 * M_PI * phase));
}

template<size_t num_overlapping_windows>
class PhasedMultitrackWindowManager {
/*
     A class responsible for keeping track of the phases of multiple overlapping windows. It is necessary to keep a 1st order
     history per window because we only want to allow a newly computed waveform to be heard after fading in from 0.
     This class allows the processor to simply switch pointers to waveforms (newly calculated and previously calculated)
     and use the associated Hanning windows to take care of smoothing out these changes.
 */
public:
    PhasedMultitrackWindowManager() = default;
    ~PhasedMultitrackWindowManager() = default;
    PhasedMultitrackWindowManager(const PhasedMultitrackWindowManager &other) = delete;
    PhasedMultitrackWindowManager(PhasedMultitrackWindowManager &&other) noexcept = delete;
    PhasedMultitrackWindowManager & operator=(const PhasedMultitrackWindowManager &other) = delete;
    PhasedMultitrackWindowManager & operator=(PhasedMultitrackWindowManager &&other) noexcept = delete;

    [[nodiscard]] static size_t getNumOverlappingWindows() {
        return num_overlapping_windows;
    }

    void allowTransition(); // this function should be called at the beginning of each block, since that is where waveforms will be updated

    constexpr static size_t num_waveforms {2};

    struct WindowAndPhase {
        std::array<double, num_waveforms> window_per_waveform_ {};
        double phase_ {};
    };

    std::array<WindowAndPhase, num_overlapping_windows> calculateWindowAndPhase();

    void increment_phase(double const frequency);

private:
    struct TrackwiseData {
        double previous_phase_; // actually this could be excluded from state, by making master phase include history, then computing per-window phase history
        size_t which_waveform_;  // maybe could just be the float pointer? then the class itself could handle reading out the waveforms
        bool allow_waveform_switch_;
    };
    double master_phase_ {0.0};
    constexpr static double num_wins_inv_ { 1.0 / num_overlapping_windows };
    std::array<TrackwiseData, num_overlapping_windows> trackwise_data_;
};

using PhasedFourTrackWindowManager = PhasedMultitrackWindowManager<4>;



#include "PhasedMultitrackWindowManager.tpp"