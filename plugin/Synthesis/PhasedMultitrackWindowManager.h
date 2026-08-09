#pragma once
#include <array>



template<std::floating_point T>
constexpr T mspWrap(T f) noexcept
{
    f = (f > std::numeric_limits<int>::max() || f < std::numeric_limits<int>::min()) ? 0. : f;
    int k = static_cast<int>(f);
    T val;
    if (k <= f)
        val = f-k;
    else
        val = f - (k-1);
    return val;
}

template<std::floating_point float_t>
struct Phasor final {
private:
    float_t phase { 0.f };
    float_t phaseDelta {0.f};	// frequency / samplerate
    float_t _sampleRate {0.f};
public:
    void setSampleRate(float_t sampleRate){
        assert (sampleRate > 0.f);
        _sampleRate = sampleRate;
    }
    void setPhase(float_t phi){
        phase = phi;
    }
    float_t getPhase() const {
        return phase;
    }
    void reset(){
        phase = 0.f;
    }
    void setPhaseDelta(float_t pd){
        phaseDelta = pd;
    }
    // may be called every sample, so no check for aliasing or divide by 0
    void setFrequency(float_t frequency){
        assert(_sampleRate > static_cast<float_t>(0.f));
        phaseDelta = frequency / _sampleRate;
    }
    float_t getFrequency() const {
        return phaseDelta * _sampleRate;
    }

    double tick(){
        assert(phaseDelta == phaseDelta);
        phase += phaseDelta;
        phase = mspWrap(phase);
        return phase;
    }
    double tick(bool& crossedOver){
        assert(phaseDelta == phaseDelta);
        phase += phaseDelta;
        const auto unwrappedPhase = phase;
        phase = mspWrap(phase);
        if (phase != unwrappedPhase) {
            crossedOver = true;
        } else {
            crossedOver = false;
        }
        return phase;
    }

};


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