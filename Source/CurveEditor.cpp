#include "CurveEditor.h"

CurveEditor::CurveEditor (SyncFilterAudioProcessor& processorToUse) : processor (processorToUse)
{
    points = processor.getCurvePoints();
    if (points.size() < 2)
        points = { { 0.0f, 0.5f }, { 1.0f, 0.5f } };

    setInterceptsMouseClicks (true, false);
}

juce::Point<float> CurveEditor::toNorm (juce::Point<float> pixelPos) const
{
    auto b = getLocalBounds().toFloat();
    if (b.getWidth() <= 0.0f || b.getHeight() <= 0.0f)
        return { 0.0f, 0.5f };
    return { juce::jlimit (0.0f, 1.0f, pixelPos.x / b.getWidth()),
             juce::jlimit (0.0f, 1.0f, 1.0f - pixelPos.y / b.getHeight()) };
}

juce::Point<float> CurveEditor::toPixels (juce::Point<float> normPos) const
{
    auto b = getLocalBounds().toFloat();
    return { normPos.x * b.getWidth(), (1.0f - normPos.y) * b.getHeight() };
}

int CurveEditor::findNear (juce::Point<float> normPos) const
{
    int best = -1;
    float bestD = 0.03f;
    for (size_t i = 0; i < points.size(); ++i)
    {
        float d = std::abs (points[i].x - normPos.x);
        if (d < bestD)
        {
            bestD = d;
            best = (int) i;
        }
    }
    return best;
}

void CurveEditor::pushToProcessor()
{
    processor.setCurvePoints (points);
}

void CurveEditor::mouseDown (const juce::MouseEvent& e)
{
    auto norm = toNorm (e.position);

    if (e.mods.isPopupMenu())
    {
        int idx = findNear (norm);
        if (idx > 0 && idx < (int) points.size() - 1)
        {
            points.erase (points.begin() + idx);
            pushToProcessor();
            repaint();
        }
        return;
    }

    int idx = findNear (norm);
    if (idx == -1)
    {
        points.push_back (norm);
        std::sort (points.begin(), points.end(),
                   [] (const juce::Point<float>& a, const juce::Point<float>& b) { return a.x < b.x; });
        for (size_t i = 0; i < points.size(); ++i)
            if (points[i] == norm) { idx = (int) i; break; }
    }
    dragIndex = idx;
    repaint();
}

void CurveEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragIndex < 0 || dragIndex >= (int) points.size())
        return;

    auto norm = toNorm (e.position);
    bool isEndpoint = (dragIndex == 0 || dragIndex == (int) points.size() - 1);

    points[(size_t) dragIndex].y = norm.y;
    if (! isEndpoint)
    {
        float minX = points[(size_t) dragIndex - 1].x + 0.01f;
        float maxX = points[(size_t) dragIndex + 1].x - 0.01f;
        points[(size_t) dragIndex].x = juce::jlimit (minX, maxX, norm.x);
    }
    repaint();
}

void CurveEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragIndex >= 0)
        pushToProcessor();
    dragIndex = -1;
}

void CurveEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    int idx = findNear (toNorm (e.position));
    if (idx > 0 && idx < (int) points.size() - 1)
    {
        points.erase (points.begin() + idx);
        pushToProcessor();
        repaint();
    }
}

void CurveEditor::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff17191d));
    g.fillRoundedRectangle (b, 6.0f);

    g.setColour (juce::Colour (0xff1f2226));
    for (int i = 1; i < 8; ++i)
    {
        float x = b.getWidth() * (float) i / 8.0f;
        g.drawVerticalLine ((int) x, 0.0f, b.getHeight());
    }
    for (int i = 1; i < 4; ++i)
    {
        float y = b.getHeight() * (float) i / 4.0f;
        g.drawHorizontalLine ((int) y, 0.0f, b.getWidth());
    }

    g.setColour (juce::Colours::orange.withAlpha (0.15f));
    g.drawHorizontalLine ((int) (b.getHeight() * 0.5f), 0.0f, b.getWidth());

    juce::Path curve;
    for (size_t i = 0; i < points.size(); ++i)
    {
        auto px = toPixels (points[i]);
        if (i == 0) curve.startNewSubPath (px); else curve.lineTo (px);
    }
    g.setColour (juce::Colour (0xffff9540));
    g.strokePath (curve, juce::PathStrokeType (2.5f));

    for (size_t i = 0; i < points.size(); ++i)
    {
        auto px = toPixels (points[i]);
        bool active = ((int) i == dragIndex);
        float r = active ? 7.0f : 5.0f;

        g.setColour (active ? juce::Colour (0xff4fd8c4) : juce::Colour (0xfff2ede4));
        g.fillEllipse (px.x - r, px.y - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colour (0xff1a1c20));
        g.drawEllipse (px.x - r, px.y - r, r * 2.0f, r * 2.0f, 1.5f);
    }
}
