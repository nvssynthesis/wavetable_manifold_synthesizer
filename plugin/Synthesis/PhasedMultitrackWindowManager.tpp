
template<size_t num_overlapping_windows>
void PhasedMultitrackWindowManager<num_overlapping_windows>::allowTransition() {
    // to be called at the beginning of each block, since that is where waveforms will be updated
    for (auto &d : trackwise_data_) {
        d.allow_waveform_switch_ = true;
    }
}

template<size_t num_overlapping_windows>
auto PhasedMultitrackWindowManager<num_overlapping_windows>::calculateWindowAndPhase() -> std::array<WindowAndPhase, num_overlapping_windows> {
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

template<size_t num_overlapping_windows>
void PhasedMultitrackWindowManager<num_overlapping_windows>::increment_phase(double const frequency) {
    master_phase_ += frequency * num_wins_inv_;
    if (master_phase_ > 1.0) {
        master_phase_ -= 1.0;
    }
    else if (master_phase_ < 0.0) {
        assert(false);
    }
}