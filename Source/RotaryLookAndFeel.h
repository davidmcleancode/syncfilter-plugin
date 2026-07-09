#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class RotaryLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                            juce::Slider&) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();

        g.setColour (juce::Colour (0xff1a1c20));
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour (juce::Colour (0xff3a3e46));
        g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        juce::Path pointer;
        float pointerLength = radius * 0.75f;
        pointer.addRectangle (-1.0f, -pointerLength, 2.0f, pointerLength);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));

        g.setColour (juce::Colour (0xffff9540));
        g.fillPath (pointer);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat();
        g.setColour (juce::Colour (0xff16181b));
        g.fillRoundedRectangle (bounds, 5.0f);
        g.setColour (juce::Colour (0xff3a3e46));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);
        juce::ignoreUnused (box);
    }
};
