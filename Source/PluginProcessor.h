#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>

class SyncFilterAudioProcessor : public juce::AudioProcessor
{
public:
    SyncFilterAudioProcessor();
    ~SyncFilterAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SyncFilter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static constexpr int kTableSize = 512;

    // Called from the message thread when the user edits the curve in the UI.
    void setCurvePoints (const std::vector<juce::Point<float>>& pointsSortedByX);
    std::vector<juce::Point<float>> getCurvePoints() const;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Lock-free double-buffered lookup table for the modulation shape.
    // The UI (message) thread writes a fresh table then atomically swaps the
    // pointer; the audio thread only ever reads through the atomic pointer.
    std::array<float, kTableSize> tableA {};
    std::array<float, kTableSize> tableB {};
    std::atomic<std::array<float, kTableSize>*> activeTable { &tableA };

    juce::CriticalSection pointsLock;
    std::vector<juce::Point<float>> curvePoints;

    void rebuildTableFromPoints();
    float lookup (float phase01) const;

    juce::dsp::IIR::Filter<float> filterL, filterR;
    double currentSampleRate = 44100.0;
    double phase = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SyncFilterAudioProcessor)
};
