#include "PluginEditor.h"

namespace
{
    // Minimum value = 7 o'clock, maximum value = 5 o'clock, sweeping clockwise
    // through 12 o'clock. In JUCE's rotary angle convention 0 radians points
    // straight up and increases clockwise, so 7 o'clock = 210deg and
    // 5 o'clock (one lap further round) = 510deg.
    void styleKnob (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setRotaryParameters (juce::degreesToRadians (210.0f),
                                juce::degreesToRadians (510.0f),
                                true);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
    }
}

SyncFilterAudioProcessorEditor::SyncFilterAudioProcessorEditor (SyncFilterAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p), curveEditor (p)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (curveEditor);

    auto setupBox = [this] (juce::ComboBox& box, juce::Label& label, const juce::String& text)
    {
        addAndMakeVisible (box);
        addAndMakeVisible (label);
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
        label.setFont (juce::Font (11.0f));
        label.setColour (juce::Label::textColourId, juce::Colour (0xff9a9da4));
        label.attachToComponent (&box, false);
    };

    setupBox (filterTypeBox, filterTypeLabel, "Filter");
    setupBox (rateBox, rateLabel, "Rate");
    setupBox (targetBox, targetLabel, "Target");

    for (auto* name : { "Lowpass", "Highpass", "Bandpass", "Notch" })
        filterTypeBox.addItem (name, filterTypeBox.getNumItems() + 1);
    for (auto* name : { "x1", "x2", "x4", "x8", "x16", "x32" })
        rateBox.addItem (name, rateBox.getNumItems() + 1);
    for (auto* name : { "Filter cutoff", "Volume", "Pan" })
        targetBox.addItem (name, targetBox.getNumItems() + 1);

    auto setupKnob = [this] (juce::Slider& knob, juce::Label& label, const juce::String& text)
    {
        styleKnob (knob);
        addAndMakeVisible (knob);
        addAndMakeVisible (label);
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (11.0f));
        label.setColour (juce::Label::textColourId, juce::Colour (0xff9a9da4));
        label.attachToComponent (&knob, false);
    };

    setupKnob (cutoffKnob, cutoffLabel, "Cutoff");
    setupKnob (resonanceKnob, resonanceLabel, "Reso");
    setupKnob (depthKnob, depthLabel, "Depth");
    setupKnob (inputKnob, inputLabel, "Input");
    setupKnob (outputKnob, outputLabel, "Output");

    cutoffKnob.setTextValueSuffix (" Hz");
    depthKnob.setTextValueSuffix (" %");
    inputKnob.setTextValueSuffix (" dB");
    outputKnob.setTextValueSuffix (" dB");

    auto& state = processor.apvts;
    filterTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, "filterType", filterTypeBox);
    rateAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, "rate", rateBox);
    targetAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, "target", targetBox);

    cutoffAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, "cutoff", cutoffKnob);
    resonanceAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, "resonance", resonanceKnob);
    depthAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, "depth", depthKnob);
    inputAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, "inputGain", inputKnob);
    outputAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, "outputGain", outputKnob);

    setSize (900, 640);
    startTimerHz (30);
}

SyncFilterAudioProcessorEditor::~SyncFilterAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void SyncFilterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0e0f11));

    auto bounds = getLocalBounds().reduced (12);
    g.setColour (juce::Colour (0xff23262b));
    g.fillRoundedRectangle (bounds.toFloat(), 10.0f);
    g.setColour (juce::Colour (0xff3a3e46));
    g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), 10.0f, 1.0f);

    auto header = bounds.removeFromTop (44).withTrimmedLeft (16);
    g.setColour (juce::Colour (0xfff2ede4));
    g.setFont (juce::Font (18.0f, juce::Font::bold));
    g.drawText ("SYNCFILTER", header, juce::Justification::centredLeft);
}

void SyncFilterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (44); // header

    auto bottom = area.removeFromBottom (110).reduced (16, 12);
    curveEditor.setBounds (area.reduced (16, 4));

    // Left: the three dropdowns (Filter type, Rate, Target)
    auto leftCluster = bottom.removeFromLeft (330);
    leftCluster.removeFromTop (16);
    auto boxRow = leftCluster.removeFromTop (24);
    filterTypeBox.setBounds (boxRow.removeFromLeft (110));
    boxRow.removeFromLeft (8);
    rateBox.setBounds (boxRow.removeFromLeft (90));
    boxRow.removeFromLeft (8);
    targetBox.setBounds (boxRow);

    bottom.removeFromLeft (24);

    // Right: all five knobs together, in order
    auto knobCluster = bottom;
    knobCluster.removeFromTop (16);
    auto knobRow = knobCluster.removeFromTop (48);
    for (auto* knob : { &cutoffKnob, &resonanceKnob, &depthKnob, &inputKnob, &outputKnob })
    {
        knob->setBounds (knobRow.removeFromLeft (70));
        knobRow.removeFromLeft (10);
    }
}

void SyncFilterAudioProcessorEditor::timerCallback()
{
    curveEditor.repaint();
}
