#include "CurveEditor.h"

CurveEditor::CurveEditor (SyncFilterAudioProcessor& processorToUse) : processor (processorToUse)
{
    points = processor.getCurvePoints();
    if (points.size() < 2)
        points = { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.5f, 0.0f } };

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

int CurveEditor::findNearNode (juce::Point<float> normPos) const
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

int CurveEditor::findNearSegment (juce::Point<float> normPos) const
{
    if (points.size() < 2) return -1;
    if (normPos.x <= points.front().x + 0.02f || normPos.x >= points.back().x - 0.02f)
    {
        // still allow segment grabs near the ends, just not right on top of a node
        if (findNearNode (normPos) != -1)
            return -1;
    }

    float curveY = evaluateCurve (points, normPos.x);
    if (std::abs (curveY - normPos.y) > 0.05f)
        return -1;

    for (size_t j = 0; j + 1 < points.size(); ++j)
        if (normPos.x >= points[j].x && normPos.x <= points[j + 1].x)
            return (int) j;

    return -1;
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
        int idx = findNearNode (norm);
        if (idx > 0 && idx < (int) points.size() - 1)
        {
            points.erase (points.begin() + idx);
            pushToProcessor();
            repaint();
        }
        return;
    }

    int nodeIdx = findNearNode (norm);
    if (nodeIdx != -1)
    {
        dragMode = DragMode::node;
        dragIndex = nodeIdx;
        repaint();
        return;
    }

    int segIdx = findNearSegment (norm);
    if (segIdx != -1)
    {
        dragMode = DragMode::segment;
        curveDragSegment = segIdx;
        curveDragStartValue = points[(size_t) segIdx].curve;
        curveDragStartNormY = norm.y;
        repaint();
        return;
    }

    CurveNode newNode;
    newNode.x = norm.x;
    newNode.y = norm.y;
    newNode.curve = 0.0f;
    points.push_back (newNode);
    std::sort (points.begin(), points.end(), [] (const CurveNode& a, const CurveNode& b) { return a.x < b.x; });

    int idx = -1;
    for (size_t i = 0; i < points.size(); ++i)
        if (points[i].x == newNode.x && points[i].y == newNode.y) { idx = (int) i; break; }

    dragMode = DragMode::node;
    dragIndex = idx;
    repaint();
}

void CurveEditor::mouseDrag (const juce::MouseEvent& e)
{
    auto norm = toNorm (e.position);

    if (dragMode == DragMode::node && dragIndex >= 0 && dragIndex < (int) points.size())
    {
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
    else if (dragMode == DragMode::segment && curveDragSegment >= 0 && curveDragSegment < (int) points.size())
    {
        float delta = (norm.y - curveDragStartNormY) * 2.5f;
        points[(size_t) curveDragSegment].curve = juce::jlimit (-1.0f, 1.0f, curveDragStartValue + delta);
        repaint();
    }
}

void CurveEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragMode != DragMode::none)
        pushToProcessor();
    dragMode = DragMode::none;
    dragIndex = -1;
    curveDragSegment = -1;
}

void CurveEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto norm = toNorm (e.position);

    int nodeIdx = findNearNode (norm);
    if (nodeIdx > 0 && nodeIdx < (int) points.size() - 1)
    {
        points.erase (points.begin() + nodeIdx);
        pushToProcessor();
        repaint();
        return;
    }

    int segIdx = findNearSegment (norm);
    if (segIdx != -1)
    {
        points[(size_t) segIdx].curve = 0.0f;
        pushToProcessor();
        repaint();
    }
}

void CurveEditor::drawFilterBand (juce::Graphics& g, juce::Rectangle<float> b)
{
    int targetIdx = (int) processor.apvts.getRawParameterValue ("target")->load();
    if (targetIdx != 0)
        return; // only meaningful when the curve is targeting the filter cutoff

    float cutoff = processor.apvts.getRawParameterValue ("cutoff")->load();
    float depthPct = processor.apvts.getRawParameterValue ("depth")->load() / 100.0f;

    float lo = std::log (80.0f), hi = std::log (12000.0f);
    float frac = (std::log (juce::jlimit (80.0f, 12000.0f, cutoff)) - lo) / (hi - lo);
    float centre = 1.0f - frac;
    float half = depthPct * 0.5f;
    float top = juce::jlimit (0.0f, 1.0f, centre - half);
    float bottom = juce::jlimit (0.0f, 1.0f, centre + half);

    g.setColour (juce::Colour (0xff4fd8c4).withAlpha (0.15f));
    g.fillRect (juce::Rectangle<float> (b.getX(), b.getY() + top * b.getHeight(),
                                         b.getWidth(), (bottom - top) * b.getHeight()));
    g.setColour (juce::Colour (0xff4fd8c4).withAlpha (0.5f));
    float centreY = b.getY() + centre * b.getHeight();
    g.drawLine (b.getX(), centreY, b.getRight(), centreY, 1.0f);
}

void CurveEditor::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff17191d));
    g.fillRoundedRectangle (b, 6.0f);

    drawFilterBand (g, b);

    juce::Path curve;
    const int steps = 200;
    for (int i = 0; i <= steps; ++i)
    {
        float x = (float) i / (float) steps;
        float y = evaluateCurve (points, x);
        auto px = toPixels ({ x, y });
        if (i == 0) curve.startNewSubPath (px); else curve.lineTo (px);
    }
    g.setColour (juce::Colour (0xffff9540));
    g.strokePath (curve, juce::PathStrokeType (2.5f));

    if (processor.isTransportPlaying())
    {
        float playX = processor.getPlayheadPhase() * b.getWidth();
        float glowHalfWidth = juce::jmax (30.0f, b.getWidth() * 0.06f);

        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (juce::Rectangle<float> (playX - glowHalfWidth, 0.0f,
                                                      glowHalfWidth * 2.0f, b.getHeight()).toNearestInt());

        juce::ColourGradient glow (juce::Colour (0xff4fd8c4).withAlpha (0.0f), playX - glowHalfWidth, 0.0f,
                                    juce::Colour (0xff4fd8c4).withAlpha (0.0f), playX + glowHalfWidth, 0.0f, false);
        glow.addColour (0.5, juce::Colour (0xff4fd8c4).withAlpha (0.95f));
        g.setGradientFill (glow);
        g.strokePath (curve, juce::PathStrokeType (4.0f));
    }

    for (size_t i = 0; i < points.size(); ++i)
    {
        auto px = toPixels ({ points[i].x, points[i].y });
        bool active = (dragMode == DragMode::node && (int) i == dragIndex);
        float r = active ? 7.0f : 5.0f;

        g.setColour (active ? juce::Colour (0xff4fd8c4) : juce::Colour (0xfff2ede4));
        g.fillEllipse (px.x - r, px.y - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colour (0xff1a1c20));
        g.drawEllipse (px.x - r, px.y - r, r * 2.0f, r * 2.0f, 1.5f);
    }
}
