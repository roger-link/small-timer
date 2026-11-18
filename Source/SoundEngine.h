#pragma once
#include "Motion.h"
#include <vector>

namespace smalltimer
{
class SoundEngine
{
public:
    void prepare(double rate, const Values& initial, bool forever, bool failure)
    {
        sampleRate = std::max(10.0, rate);
        smoothing = static_cast<float>(1.0 - std::exp(-1.0 / (0.02 * sampleRate)));
        dcPole = static_cast<float>(std::exp(-2.0 * pi * 15.0 / sampleRate));
        current = destination = initial;
        foreverMix = foreverTarget = forever ? 1.0f : 0.0f;
        failureMix = failureTarget = failure ? 1.0f : 0.0f;
        for (auto& channel : delay)
            channel.assign(static_cast<size_t>(sampleRate * 2.2) + 64, 0.0f);
        const double times[] { 0.0371, 0.0533, 0.0719, 0.0893 };
        for (size_t line = 0; line < room.size(); ++line)
        {
            room[line].assign(std::max<size_t>(2, static_cast<size_t>(sampleRate * times[line])), 0.0f);
            roomHead[line] = 0;
        }
        head = 0;
        phase = grainPhase = 0;
        tone = dcInput = dcOutput = {};
        roomTone = {};
    }

    void setTargets(const Values& values, bool forever, bool failure)
    {
        destination = values;
        foreverTarget = forever ? 1.0f : 0.0f;
        failureTarget = failure ? 1.0f : 0.0f;
    }

    std::array<float, 2> process(std::array<float, 2> input)
    {
        for (size_t index = 0; index < current.size(); ++index)
            current[index] += smoothing * (destination[index] - current[index]);
        foreverMix += smoothing * (foreverTarget - foreverMix);
        failureMix += smoothing * (failureTarget - failureMix);
        phase += (0.13 + current[3] * 0.31) / sampleRate;
        grainPhase += (3.0 + current[4] * 9.0) / sampleRate;
        phase -= std::floor(phase);
        grainPhase -= std::floor(grainPhase);
        const float drift = current[3];
        const float grain = current[4];
        const float space = current[2];
        const auto delaySamples = static_cast<float>(sampleRate * 0.04 * std::pow(50.0, current[0]));
        const auto feedback = (1.0f - foreverMix) * current[1] * 0.94f
                            + foreverMix * (0.70f + current[1] * 0.295f);
        const auto toneCoefficient = static_cast<float>(1.0 - std::exp(-2.0 * pi * (6500.0 - failureMix * 4900.0) / sampleRate));
        std::array<float, 2> echo {};
        for (size_t channel = 0; channel < 2; ++channel)
        {
            if (!std::isfinite(input[channel]))
                input[channel] = 0;
            const auto wobble = static_cast<float>(std::sin(2 * pi * phase + channel * 0.71)
                                + 0.2 * std::sin(2 * pi * phase * 7.0 + channel));
            const float offset = static_cast<float>(sampleRate * 0.008) * drift * wobble;
            const float base = read(channel, delaySamples + offset);
            float grains = 0;
            for (int voice = 0; voice < 2; ++voice)
            {
                double position = grainPhase + voice * 0.5;
                position -= std::floor(position);
                const auto window = static_cast<float>(0.5 - 0.5 * std::cos(2 * pi * position));
                const auto shift = static_cast<float>((position - 0.5) * sampleRate * (0.025 + grain * 0.075) * (voice == 0 ? 1 : -1));
                grains += window * read(channel, delaySamples + offset + shift);
            }
            echo[channel] = base + grain * 0.8f * (grains - base);
            tone[channel] += toneCoefficient * (echo[channel] - tone[channel]);
            const float blocked = tone[channel] - dcInput[channel] + dcPole * dcOutput[channel];
            dcInput[channel] = tone[channel];
            dcOutput[channel] = blocked;
            const auto damage = 1.0f - failureMix * static_cast<float>(0.18 + 0.17 * std::sin(2 * pi * phase * 2.0 + channel));
            const float driven = std::clamp(input[channel], -8.0f, 8.0f) + feedback * blocked * damage;
            delay[channel][head] = std::tanh(driven * (1.0f + failureMix * 0.6f)) / (1.0f + failureMix * 0.6f);
        }
        std::array<float, 4> taps {};
        for (size_t line = 0; line < room.size(); ++line)
        {
            roomTone[line] += 0.35f * (room[line][roomHead[line]] - roomTone[line]);
            taps[line] = roomTone[line];
        }
        const float sum = (taps[0] + taps[1] + taps[2] + taps[3]) * 0.5f;
        for (size_t line = 0; line < room.size(); ++line)
        {
            room[line][roomHead[line]] = echo[line % 2] * 0.28f + (sum - taps[line]) * (0.55f + space * 0.30f);
            roomHead[line] = (roomHead[line] + 1) % room[line].size();
        }
        head = (head + 1) % delay[0].size();
        const std::array<float, 2> ambience { (taps[0] + taps[2]) * 0.5f, (taps[1] + taps[3]) * 0.5f };
        std::array<float, 2> output {};
        for (size_t channel = 0; channel < 2; ++channel)
        {
            const float wet = echo[channel] * (1.0f - space * 0.35f) + ambience[channel] * space;
            const float protectedWet = 1.25f * std::tanh(wet / 1.25f);
            output[channel] = input[channel] * (1.0f - current[5]) + protectedWet * current[5];
        }
        return output;
    }

private:
    float read(size_t channel, float samples) const
    {
        samples = std::clamp(samples, 1.0f, static_cast<float>(delay[channel].size() - 2));
        double position = static_cast<double>(head) - samples;
        if (position < 0)
            position += static_cast<double>(delay[channel].size());
        const auto first = static_cast<size_t>(position);
        const auto second = (first + 1) % delay[channel].size();
        const auto fraction = static_cast<float>(position - static_cast<double>(first));
        return delay[channel][first] + fraction * (delay[channel][second] - delay[channel][first]);
    }
    static constexpr double pi = 3.14159265358979323846;
    std::array<std::vector<float>, 2> delay;
    std::array<std::vector<float>, 4> room;
    std::array<size_t, 4> roomHead {};
    std::array<float, 4> roomTone {};
    std::array<float, 2> tone {}, dcInput {}, dcOutput {};
    Values current {}, destination {};
    size_t head = 0;
    double sampleRate = 48000, phase = 0, grainPhase = 0;
    float smoothing = 0, dcPole = 0;
    float foreverMix = 0, foreverTarget = 0, failureMix = 0, failureTarget = 0;
};
}
