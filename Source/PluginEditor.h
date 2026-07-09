#pragma once

#include "PluginProcessor.h"
#include "CurveEditor.h"
#include "RotaryLookAndFeel.h"

class SyncFilterAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit SyncFilterAudioProcessorEditor (SyncFilterAudioProcessor&);
    ~SyncFilterAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    SyncFilterAudioProcessor& processor;
    RotaryLookAndFeel lnf;

    CurveEditor curveEditor;

    juce::ComboBox filterTypeBox, rateBox, targetBox;
    juce::Slider cutoffKnob, resonanceKnob, depthKnob, inputKnob, outputKnob;
    juce::Label filterTypeLabel, rateLabel, targetLabel,
                cutoffLabel, resonanceLabel, depthLabel, inputLabel, outputLabel;
    juce::ToggleButton bypassButton { "Bypass" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        filterTypeAttachment, rateAttachment, targetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        cutoffAttachment, resonanceAttachment, depthAttachment, inputAttachment, outputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SyncFilterAudioProcessorEditor)
};
