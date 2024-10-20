#pragma once
#include <array>

inline double process_phase_for_track(double phase, size_t const window_idx, size_t const num_windows) {
    double const window_gap = static_cast<double>(window_idx) / static_cast<double>(num_windows);
    phase -= window_gap;
    phase = std::fmod(phase, 1.0);
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

    void allowTransition() {
        // this function should be called at the beginning of each block, since that is where waveforms will be updated
        for (auto &d : trackwise_data_) {
            d.allow_waveform_switch_ = true;
        }
    }
    constexpr static size_t num_waveforms {2};

    struct WindowAndPhase {
        std::array<double, num_waveforms> window_per_waveform_ {};
        double phase_ {};
    };
    std::array<WindowAndPhase, num_overlapping_windows> calculateWindowAndPhase() {
        std::array<WindowAndPhase, num_overlapping_windows> windows_and_phases;
        for (size_t i = 0; i < num_overlapping_windows; ++i) {
            windows_and_phases[i].phase_ = process_phase_for_track(master_phase_, i, num_overlapping_windows);
            // only writing into the window corresponding to the waveform that is supposed to currently sound.
            // the other should remain silent (zero).
            windows_and_phases[i].window_per_waveform_[trackwise_data_[i].which_waveform_] = hanning(windows_and_phases[i].phase_);
            if (windows_and_phases[i].phase_ < trackwise_data_[i].previous_phase_) {
                if (trackwise_data_[i].allow_waveform_switch_) {
                    trackwise_data_[i].which_waveform_ += 1;
                    trackwise_data_[i].which_waveform_ %= num_waveforms;
                    trackwise_data_[i].allow_waveform_switch_ = false;
                }
            }
            trackwise_data_[i].previous_phase_ = windows_and_phases[i].phase_;
        }
        return windows_and_phases;
    }
    void increment_phase(double const frequency) {
        master_phase_ += frequency;
        if (master_phase_ > 1.0) {
            master_phase_ -= 1.0;
        }
        else if (master_phase_ < 0.0) {
            assert(false);
        }
    }

private:
    struct TrackwiseData {
        double previous_phase_; // actually this could be excluded from state, by making master phase include history, then computing per-window phase history
        size_t which_waveform_;  // maybe could just be the float pointer? then the class itself could handle reading out the waveforms
        bool allow_waveform_switch_;
    };
    double master_phase_ {0.0};
    std::array<TrackwiseData, num_overlapping_windows> trackwise_data_;
};

using PhasedFourTrackWindowManager = PhasedMultitrackWindowManager<4>;