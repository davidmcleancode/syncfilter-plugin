#include "PluginProcessor.h"
#include "PluginEditor.h"

SyncFilterAudioProcessor::SyncFilterAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    curvePoints = { { 0.0f, 0.5f }, { 1.0f, 0.5f } };
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

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "bypass", "Bypass", false));

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

void SyncFilterAudioProcessor::setCurvePoints (const std::vector<juce::Point<float>>& pts)
{
    {
        const juce::ScopedLock sl (pointsLock);
        curvePoints = pts;
    }
    rebuildTableFromPoints();
}

std::vector<juce::Point<float>> SyncFilterAudioProcessor::getCurvePoints() const
{
    const juce::ScopedLock sl (pointsLock);
    return curvePoints;
}

void SyncFilterAudioProcessor::rebuildTableFromPoints()
{
    std::vector<juce::Point<float>> pts;
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
        size_t seg = 0;
        for (size_t j = 0; j + 1 < pts.size(); ++j)
        {
            seg = j;
            if (x >= pts[j].x && x <= pts[j + 1].x)
                break;
        }
        auto a = pts[seg];
        auto b = pts[juce::jmin (seg + 1, pts.size() - 1)];
        float span = b.x - a.x;
        float t = span > 0.0f ? (x - a.x) / span : 0.0f;
        inactive[(size_t) i] = a.y + (b.y - a.y) * t;
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

    const bool bypassed = apvts.getRawParameterValue ("bypass")->load() > 0.5f;
    if (bypassed)
        return; // true bypass: audio passes through completely unprocessed

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    const float inGain  = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("inputGain")->load());
    const float outGain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("outputGain")->load());

    buffer.applyGain (inGain);

    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
            if (auto hostBpm = position->getBpm())
                bpm = *hostBpm;
    }

    static const float rateMultipliers[] = { 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f };
    const int rateIndex = juce::jlimit (0, 5, (int) apvts.getRawParameterValue ("rate")->load());
    const double freq = (bpm / 240.0) * (double) rateMultipliers[rateIndex];
    const double phaseInc = freq / currentSampleRate;

    const int targetIndex   = (int) apvts.getRawParameterValue ("target")->load();
    const float depth       = apvts.getRawParameterValue ("depth")->load() / 100.0f;
    const float baseCutoff  = apvts.getRawParameterValue ("cutoff")->load();
    const float resonance   = apvts.getRawParameterValue ("resonance")->load();
    const int filterTypeIdx = (int) apvts.getRawParameterValue ("filterType")->load();

    const int controlRate = 32; // recompute filter coefficients every N samples
    int samplesSinceUpdate = controlRate;

    for (int i = 0; i < numSamples; ++i)
    {
        const float raw    = lookup ((float) phase);
        const float shaped = juce::jlimit (-1.0f, 1.0f, raw * 2.0f - 1.0f) * depth;

        float modulatedCutoff = baseCutoff;
        float tremGain = 1.0f;
        float pan = 0.0f;

        if (targetIndex == 0)
            modulatedCutoff = juce::jlimit (20.0f, (float) (currentSampleRate * 0.45), baseCutoff + shaped * 6000.0f);
        else if (targetIndex == 1)
            tremGain = juce::jlimit (0.0f, 1.5f, 1.0f + shaped * 0.5f);
        else if (targetIndex == 2)
            pan = shaped;

        if (samplesSinceUpdate >= controlRate)
        {
            juce::dsp::IIR::Coefficients<float>::Ptr coeffs;
            switch (filterTypeIdx)
            {
                case 0:  coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass  (currentSampleRate, modulatedCutoff, resonance); break;
                case 1:  coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, modulatedCutoff, resonance); break;
                case 2:  coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (currentSampleRate, modulatedCutoff, resonance); break;
                default: coeffs = juce::dsp::IIR::Coefficients<float>::makeNotch    (currentSampleRate, modulatedCutoff, resonance); break;
            }
            *filterL.coefficients = *coeffs;
            *filterR.coefficients = *coeffs;
            samplesSinceUpdate = 0;
        }
        ++samplesSinceUpdate;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float x = buffer.getReadPointer (ch)[i];
            float y = (ch == 0) ? filterL.processSample (x) : filterR.processSample (x);
            y *= tremGain;

            if (numChannels == 2)
            {
                float panGain = (ch == 0) ? juce::jmin (1.0f, 1.0f - pan) : juce::jmin (1.0f, 1.0f + pan);
                y *= panGain;
            }

            buffer.getWritePointer (ch)[i] = y;
        }

        phase += phaseInc;
        if (phase >= 1.0)
            phase -= 1.0;
    }

    buffer.applyGain (outGain);
}

void SyncFilterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    juce::ValueTree curveTree ("CURVE");
    {
        const juce::ScopedLock sl (pointsLock);
        for (auto& p : curvePoints)
        {
            juce::ValueTree node ("POINT");
            node.setProperty ("x", p.x, nullptr);
            node.setProperty ("y", p.y, nullptr);
            curveTree.addChild (node, -1, nullptr);
        }
    }
    state.addChild (curveTree, -1, nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SyncFilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr)
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid())
        return;

    apvts.replaceState (state);

    auto curveTree = state.getChildWithName ("CURVE");
    if (curveTree.isValid())
    {
        std::vector<juce::Point<float>> pts;
        for (int i = 0; i < curveTree.getNumChildren(); ++i)
        {
            auto node = curveTree.getChild (i);
            pts.push_back ({ (float) node.getProperty ("x"), (float) node.getProperty ("y") });
        }
        if (pts.size() >= 2)
            setCurvePoints (pts);
    }
}

juce::AudioProcessorEditor* SyncFilterAudioProcessor::createEditor()
{
    return new SyncFilterAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SyncFilterAudioProcessor();
}
