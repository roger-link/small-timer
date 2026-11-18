#pragma once
#include <array>
#include <algorithm>
#include <cmath>

namespace smalltimer
{
constexpr int faderCount = 6;
using Values = std::array<float, faderCount>;
inline constexpr Values loopDefaults { 0.32f, 0.42f, 0.25f, 0.18f, 0.12f, 0.50f };
inline constexpr Values foreverDefaults { 0.72f, 0.78f, 0.62f, 0.34f, 0.28f, 0.65f };

class Motion
{
public:
    Values position = loopDefaults;
    std::array<bool, faderCount> dragging {};
    std::array<float, faderCount> settle {};

    void tick(const Values& target, const std::array<bool, faderCount>& locked,
              bool wander, bool hold, float depth, float seconds)
    {
        if (!hold)
            phase += seconds;
        for (int index = 0; index < faderCount; ++index)
        {
            if (dragging[index])
            {
                position[index] = target[index];
                settle[index] = 1.5f;
                continue;
            }
            settle[index] = std::max(0.0f, settle[index] - seconds);
            if (hold)
                continue;
            const float offset = wander && !locked[index] && settle[index] == 0.0f
                ? depth * 0.16f * std::sin(phase * (0.27f + index * 0.041f) + index * 1.71f)
                  * std::sin(phase * 0.113f + index * 0.83f) : 0.0f;
            const auto destination = std::clamp(target[index] + offset, 0.0f, 1.0f);
            position[index] += (destination - position[index]) * (1.0f - std::exp(-seconds * 9.0f));
        }
    }
private:
    float phase = 0.0f;
};
}
