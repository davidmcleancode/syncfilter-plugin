#pragma once

#include <vector>
#include <cmath>
#include <juce_core/juce_core.h>

// A single breakpoint node. `curve` bends the segment that starts at this
// node (i.e. the segment running from this node to the next one), in the
// range -1..1. 0 is a straight line; positive bows the segment toward the
// top-left (fast rise), negative bows it toward the bottom-right (slow rise) —
// the same idea as dragging a bezier handle in Illustrator, just constrained
// to stay a single-valued function of x so it remains valid as an audio
// modulation curve.
struct CurveNode
{
    float x = 0.0f;
    float y = 0.5f;
    float curve = 0.0f;
};

inline float evaluateCurve (const std::vector<CurveNode>& pts, float phase)
{
    phase = phase - std::floor (phase);
    if (pts.size() < 2)
        return 0.5f;

    size_t seg = 0;
    for (size_t j = 0; j + 1 < pts.size(); ++j)
    {
        seg = j;
        if (phase >= pts[j].x && phase <= pts[j + 1].x)
            break;
    }

    const auto& a = pts[seg];
    const auto& b = pts[juce::jmin (seg + 1, pts.size() - 1)];

    float span = b.x - a.x;
    float t = span > 0.0f ? (phase - a.x) / span : 0.0f;
    t = juce::jlimit (0.0f, 1.0f, t);

    // Power-curve shaping: exponent of 1 is a straight line; curve < 0 makes
    // the exponent > 1 (slow start), curve > 0 makes it < 1 (fast start).
    // Always passes exactly through both endpoints, and stays monotonic in x.
    float exponent = std::pow (2.0f, -a.curve * 4.0f);
    float shaped = std::pow (t, exponent);

    return a.y + (b.y - a.y) * shaped;
}
