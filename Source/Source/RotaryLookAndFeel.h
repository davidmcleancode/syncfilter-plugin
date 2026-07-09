#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class RotaryLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RotaryLookAndFeel()
    {
        // Dropdown boxes
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff16181b));
        setColour (juce::ComboBox::textColourId, juce::Colour (0xfff2ede4));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3a3e46));
        setColour (juce::ComboBox::arrowColourId, juce::Colour (0xffff9540));
        setColour (juce::ComboBox::buttonColourId, juce::Colour (0xff16181b));

        // Popup menu (the list that appears when a dropdown is clicked)
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff23262b));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xfff2ede4));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff3a2a17));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xffff9540));

        // Slider value/readout boxes (the numeric box below each knob)
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xff4fd8c4));
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff16181b));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff3a3e46));
        setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffff9540));
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a3e46));

        // Toggle button (Bypass)
        setColour (juce::ToggleButton::textColourId, juce::Colour (0xfff2ede4));
        setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffff9540));
        setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff3a3e46));

        // Text labels
        setColour (juce::Label::textColourId, juce::Colour (0xff9a9da4));
    }

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

        // Value arc showing current position, matching the web prototype's fill ring
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, radius - 1.5f, radius - 1.5f, 0.0f,
                            rotaryStartAngle, rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle), true);
        g.setColour (juce::Colour (0xffff9540).withAlpha (0.6f));
        g.strokePath (arc, juce::PathStrokeType (2.0f));

        float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        juce::Path pointer;
        float pointerLength = radius * 0.72f;
        pointer.addRectangle (-1.4f, -pointerLength, 2.8f, pointerLength);
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

        auto arrowZone = bounds.removeFromRight (24.0f).reduced (6.0f);
        juce::Path arrow;
        arrow.startNewSubPath (arrowZone.getX(), arrowZone.getCentreY() - 3.0f);
        arrow.lineTo (arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f);
        arrow.lineTo (arrowZone.getRight(), arrowZone.getCentreY() - 3.0f);
        g.setColour (juce::Colour (0xffff9540));
        g.strokePath (arrow, juce::PathStrokeType (1.8f));

        juce::ignoreUnused (box);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setColour (juce::Label::textColourId, juce::Colour (0xfff2ede4));
    }
};
