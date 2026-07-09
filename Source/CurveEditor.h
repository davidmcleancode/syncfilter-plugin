#pragma once

#include "PluginProcessor.h"
#include "CurveMath.h"

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
    enum class DragMode { none, node, segment };

    SyncFilterAudioProcessor& processor;
    std::vector<CurveNode> points;

    DragMode dragMode = DragMode::none;
    int dragIndex = -1;              // node being dragged
    int curveDragSegment = -1;       // segment (left node index) whose bend is being dragged
    float curveDragStartValue = 0.0f;
    float curveDragStartNormY = 0.0f;

    juce::Point<float> toNorm (juce::Point<float> pixelPos) const;
    juce::Point<float> toPixels (juce::Point<float> normPos) const;
    int findNearNode (juce::Point<float> normPos) const;
    int findNearSegment (juce::Point<float> normPos) const;
    void pushToProcessor();
    void drawFilterBand (juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurveEditor)
};
