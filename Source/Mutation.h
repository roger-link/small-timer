#pragma once
#include "Motion.h"
#include <cstdint>

namespace smalltimer
{
inline constexpr std::array<Values, 3> mutationGestures {{
    { 0.10f, 0.08f, 0.22f, -0.24f, -0.20f, 0.04f },
    { -0.30f, -0.16f, -0.34f, 0.08f, 0.40f, -0.06f },
    { 0.20f, 0.08f, 0.12f, 0.16f, -0.20f, 0.02f }
}};
inline constexpr const char* mutationNames[] { "CLOUD", "FRAGMENTS", "BLOOM" };

struct MutationPlan
{
    Values values;
    int kind;
};

inline MutationPlan planMutation(const Values& current, float depth, bool forever, uint32_t seed)
{
    MutationPlan plan { current, static_cast<int>(seed % mutationGestures.size()) };
    depth = std::clamp(depth, 0.0f, 1.0f);
    auto random = [&seed]
    {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(seed >> 8) / 16777216.0f;
    };
    const auto strength = 0.8f + random() * 0.4f;
    for (int index = 0; index < faderCount; ++index)
    {
        auto direction = mutationGestures[plan.kind][index];
        if (forever && index == 1)
            direction *= 0.6f;
        const auto variation = direction * (0.92f + random() * 0.16f);
        const auto destination = current[index] + variation * strength * depth;
        plan.values[index] = destination < 0 ? -destination : destination > 1 ? 2.0f - destination : destination;
    }
    return plan;
}
}
