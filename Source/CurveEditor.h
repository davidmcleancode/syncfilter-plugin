#pragma once

#include "PluginProcessor.h"

class CurveEditor : public juce::Component
{
public:
    explicit CurveEditor (SyncFilterAudioProcessor& processorToUse);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    SyncFilterAudioProcessor& processor;
    std::vector<juce::Point<float>> points;
    int dragIndex = -1;

    juce::Point<float> toNorm (juce::Point<float> pixelPos) const;
    juce::Point<float> toPixels (juce::Point<float> normPos) const;
    int findNear (juce::Point<float> normPos) const;
    void pushToProcessor();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurveEditor)
};
