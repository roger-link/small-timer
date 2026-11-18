#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cmath>

namespace smalltimer
{
struct WaveformFrame
{
    float input = 0;
    float output = 0;
};

class WaveformHistory
{
public:
    using Snapshot = std::array<WaveformFrame, 256>;
    WaveformHistory() { clear(); }

    void clear()
    {
        count = 0;
        for (auto& frame : frames)
            frame.store(0, std::memory_order_relaxed);
        revision.store(0, std::memory_order_release);
    }

    void push(float input, float output)
    {
        input = std::isfinite(input) ? input : 0.0f;
        output = std::isfinite(output) ? output : 0.0f;
        uint32_t inputBits = 0, outputBits = 0;
        std::memcpy(&inputBits, &input, sizeof(float));
        std::memcpy(&outputBits, &output, sizeof(float));
        const uint64_t pair = static_cast<uint64_t>(inputBits) | (static_cast<uint64_t>(outputBits) << 32);
        revision.store(count * 2 + 1, std::memory_order_release);
        frames[count % frames.size()].store(pair, std::memory_order_release);
        revision.store(++count * 2, std::memory_order_release);
    }

    Snapshot snapshot() const
    {
        Snapshot result {};
        for (int attempt = 0; attempt < 3; ++attempt)
        {
            const auto sequence = revision.load(std::memory_order_acquire);
            if (sequence % 2 != 0)
                continue;
            const auto end = sequence / 2;
            const auto available = end < frames.size() ? static_cast<size_t>(end) : frames.size();
            result = {};
            for (size_t index = 0; index < available; ++index)
            {
                const auto bits = frames[(end - available + index) % frames.size()].load(std::memory_order_acquire);
                const auto inputBits = static_cast<uint32_t>(bits);
                const auto outputBits = static_cast<uint32_t>(bits >> 32);
                auto& frame = result[result.size() - available + index];
                std::memcpy(&frame.input, &inputBits, sizeof(float));
                std::memcpy(&frame.output, &outputBits, sizeof(float));
            }
            if (sequence == revision.load(std::memory_order_acquire))
                return result;
        }
        return {};
    }

private:
    static_assert(std::atomic<uint64_t>::is_always_lock_free);
    std::array<std::atomic<uint64_t>, 256> frames {};
    std::atomic<uint64_t> revision { 0 };
    uint64_t count = 0;
};
}
