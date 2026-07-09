#include "PluginProcessor.h"
#include "PluginEditor.h"

SyncFilterAudioProcessor::SyncFilterAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    curvePoints = { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.5f, 0.0f } };
    rebuildTableFromPoints();
}

SyncFilterAudioProcessor::~SyncFilterAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout SyncFilterAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "filterType", "Filter Type",
        juce::StringArray { "Lowpass", "Highpass", "Bandpass", "Notch" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "cutoff", "Cutoff",
        juce::NormalisableRange<float> (80.0f, 12000.0f, 1.0f, 0.3f), 1200.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "resonance", "Resonance",
        juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.3f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "rate", "Rate",
        juce::StringArray { "x1", "x2", "x4", "x8", "x16", "x32" }, 2));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "target", "Target",
        juce::StringArray { "Filter cutoff", "Volume", "Pan" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "depth", "Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "inputGain", "Input",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "outputGain", "Output",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    return { params.begin(), params.end() };
}

void SyncFilterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    phase = 0.0;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 1 };
    filterL.prepare (spec);
    filterR.prepare (spec);
    filterL.reset();
    filterR.reset();
}

bool SyncFilterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void SyncFilterAudioProcessor::setCurvePoints (const std::vector<CurveNode>& pts)
{
    {
        const juce::ScopedLock sl (pointsLock);
        curvePoints = pts;
    }
    rebuildTableFromPoints();
}

std::vector<CurveNode> SyncFilterAudioProcessor::getCurvePoints() const
{
    const juce::ScopedLock sl (pointsLock);
    return curvePoints;
}

void SyncFilterAudioProcessor::rebuildTableFromPoints()
{
    std::vector<CurveNode> pts;
    {
        const juce::ScopedLock sl (pointsLock);
        pts = curvePoints;
    }
    if (pts.size() < 2)
        return;

    auto* active = activeTable.load();
    auto& inactive = (active == &tableA) ? tableB : tableA;

    for (int i = 0; i < kTableSize; ++i)
    {
        float x = (float) i / (float) (kTableSize - 1);
        inactive[(size_t) i] = evaluateCurve (pts, x);
    }

    activeTable.store (&inactive);
}

float SyncFilterAudioProcessor::lookup (float phase01) const
{
    auto* table = activeTable.load();
    float pos = juce::jlimit (0.0f, 1.0f, phase01) * (float) (kTableSize - 1);
    int i0 = (int) pos;
    int i1 = juce::jmin (i0 + 1, kTableSize - 1);
    float frac = pos - (float) i0;
    return (*table)[(size_t) i0] * (1.0f - frac) + (*table)[(size_t) i1] * frac;
}

void SyncFilterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    const float inGain  = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("inputGain")->load());
    const float outGain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("outputGain")->load());

    buffer.applyGain (inGain);

    double bpm = 120.0;
    bool isPlaying = false;
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto hostBpm =
