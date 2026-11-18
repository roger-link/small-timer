#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>

bool registerLocalAU(const char* path);

namespace
{
int checks = 0;
void check(bool condition, const char* message)
{
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
bool near(float actual, float expected) { return std::abs(actual - expected) < 0.002f; }

using Audio = std::vector<std::array<float, 2>>;
Audio render(const smalltimer::Values& settings, bool forever, bool failure, int source = 0, double rate = 48000, double seconds = 2)
{
    smalltimer::SoundEngine engine;
    engine.prepare(rate, settings, forever, failure);
    Audio audio(static_cast<size_t>(rate * seconds));
    for (size_t sample = 0; sample < audio.size(); ++sample)
    {
        const auto time = static_cast<double>(sample) / rate;
        float input = sample == 0 ? 0.5f : 0.0f;
        if (source == 1)
            input = time < 0.6 ? static_cast<float>(0.2 * std::sin(time * 2764.601535) + 0.1 * std::sin(time * 10681.415)) : 0;
        if (source == 2)
            input = time < 1 ? (sample % 24 < 12 ? 8.0f : -8.0f) : 0;
        audio[sample] = engine.process({ input, source == 0 ? 0.0f : input * 0.7f });
    }
    return audio;
}
double energy(const Audio& audio, size_t start, size_t channel = 0)
{
    double sum = 0;
    for (size_t sample = start; sample < audio.size(); ++sample)
        sum += static_cast<double>(audio[sample][channel]) * audio[sample][channel];
    return sum;
}
double difference(const Audio& first, const Audio& second)
{
    double sum = 0;
    for (size_t sample = 0; sample < first.size(); ++sample)
        sum += std::abs(first[sample][0] - second[sample][0]);
    return sum;
}
void testSoundEngine()
{
    const smalltimer::Values plain { 0, 0, 0, 0, 0, 1 };
    for (double rate : { 44100.0, 48000.0, 96000.0 })
    {
        const auto shortDelay = render(plain, false, false, 0, rate, 0.5);
        const auto first = std::find_if(shortDelay.begin(), shortDelay.end(), [](const auto& frame) { return std::abs(frame[0]) > 0.01f; });
        check(std::abs(static_cast<double>(first - shortDelay.begin()) - rate * 0.04) <= 1.0, "TIME minimum must delay the signal by 40 ms at each sample rate");
        auto longer = plain;
        longer[0] = 0.5f;
        const auto longDelay = render(longer, false, false, 0, rate, 0.5);
        const auto later = std::find_if(longDelay.begin(), longDelay.end(), [](const auto& frame) { return std::abs(frame[0]) > 0.01f; });
        check(std::abs(static_cast<double>(later - longDelay.begin()) - rate * 0.04 * std::sqrt(50.0)) <= 2.0, "TIME must set the audible echo length");
    }
    auto repeating = plain;
    repeating[1] = 0.8f;
    const auto once = render(plain, false, false);
    const auto repeats = render(repeating, false, false);
    const auto lingering = render(repeating, true, false);
    check(energy(once, 4000) == 0 && energy(repeats, 4000) > 0.001, "FEED must produce additional repeats");
    check(energy(lingering, 24000) > energy(repeats, 24000) * 2, "Little Forever must retain echoes longer than Loop");
    auto roomy = plain;
    roomy[2] = 1;
    const auto space = render(roomy, false, false);
    check(energy(once, 0, 1) == 0 && energy(space, 0, 1) > 0.000001, "SPACE must diffuse a mono impulse into a stereo tail");
    const auto cleanTone = render(plain, false, false, 1);
    auto drifting = plain;
    drifting[3] = 1;
    check(difference(cleanTone, render(drifting, false, false, 1)) > 1, "DRIFT must audibly modulate delay pitch");
    auto granular = plain;
    granular[4] = 1;
    check(difference(cleanTone, render(granular, false, false, 1)) > 1, "GRAIN must change the delayed texture");
    check(difference(repeats, render(repeating, false, true)) > 0.01, "SOFT FAILURE must change the repeat character");
    auto drySettings = plain;
    drySettings[5] = 0;
    smalltimer::SoundEngine dry;
    dry.prepare(48000, drySettings, false, false);
    bool exactDry = true;
    for (int sample = 0; sample < 48000; ++sample)
    {
        const float input = static_cast<float>(std::sin(sample * 0.17) * 0.7);
        const auto output = dry.process({ input, -input });
        exactDry = exactDry && output[0] == input && output[1] == -input;
    }
    check(exactDry, "MIX zero must provide exact stereo dry audio");
    for (bool forever : { false, true })
    {
        const auto stressed = render({ 0.02f, 1, 1, 1, 1, 1 }, forever, true, 2, 48000, 4);
        bool bounded = true;
        for (const auto& frame : stressed)
            for (float sample : frame)
                bounded = bounded && std::isfinite(sample) && std::abs(sample) <= 1.251f;
        check(bounded, "Maximum feedback, space, drift, grain and hot input must stay finite and bounded");
    }
    smalltimer::SoundEngine reset;
    reset.prepare(48000, repeating, true, true);
    reset.process({ 1, 1 });
    reset.prepare(48000, repeating, true, true);
    bool silent = true;
    for (int sample = 0; sample < 48000; ++sample)
    {
        const auto output = reset.process({ 0, 0 });
        silent = silent && output[0] == 0 && output[1] == 0;
    }
    check(silent, "Preparing a fresh engine must erase all effect audio");
    const auto invalid = reset.process({ std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity() });
    check(std::isfinite(invalid[0]) && std::isfinite(invalid[1]), "Nonfinite input must not poison the engine");
    smalltimer::SoundEngine smooth;
    smooth.prepare(48000, drySettings, false, false);
    float previous = 0;
    float largestStep = 0;
    for (int sample = 0; sample < 48000; ++sample)
    {
        if (sample == 12000)
            smooth.setTargets({ 1, 1, 1, 1, 1, 1 }, true, true);
        const auto output = smooth.process({ 0.1f, 0.1f });
        largestStep = std::max(largestStep, std::abs(output[0] - previous));
        previous = output[0];
    }
    check(largestStep < 0.11f, "Large control changes must be smoothed");
    std::cout << "PASS: audible TIME/FEED/SPACE/DRIFT/GRAIN/MIX, Forever, Soft Failure, sample rates, smoothing and feedback stability\n";
}

void testClosedEditorMotion()
{
    SmallTimerProcessor processor;
    processor.setValue("motion", 1);
    processor.setValue("mutate", 1);
    processor.prepareToPlay(48000, 257);
    juce::AudioBuffer<float> block(2, 257);
    juce::MidiBuffer midi;
    auto advance = [&](int count)
    {
        for (int index = 0; index < count; ++index)
        {
            block.clear();
            processor.processBlock(block, midi);
        }
    };
    advance(1600);
    check(!near(processor.motorPositions()[0], smalltimer::loopDefaults[0]), "Wander must advance in audio with no editor");
    check(near(processor.targets()[0], smalltimer::loopDefaults[0]), "Wander must not overwrite saved targets");
    processor.setValue("lock0", 1);
    advance(200);
    check(near(processor.motorPositions()[0], smalltimer::loopDefaults[0]), "Audio Wander must respect locks");
    processor.setValue("hold", 1);
    const auto held = processor.motorPositions();
    advance(200);
    check(processor.motorPositions() == held, "HOLD must freeze the audible control positions");
    processor.setValue(SmallTimerProcessor::faderID(0, 0), 0.91f);
    processor.manualFaderEdit(0);
    advance(1);
    check(near(processor.motorPositions()[0], 0.91f), "Manual control must override audible HOLD");
    processor.setValue("hold", 0);
    processor.setValue("motion", 0);
    processor.setValue("mode", 1);
    advance(300);
    check(near(processor.motorPositions()[0], smalltimer::foreverDefaults[0]), "Audio motor must recall the other mode");
    check(near(processor.value(SmallTimerProcessor::faderID(0, 0)), 0.91f), "Audible mode recall must preserve the previous bank");
}

void testStreamAndState()
{
    auto stream = [](const std::vector<int>& sizes)
    {
        SmallTimerProcessor processor;
        processor.setValue("motion", 1);
        processor.prepareToPlay(48000, 1024);
        juce::MidiBuffer midi;
        Audio result;
        result.reserve(16000);
        int offset = 0;
        size_t blockIndex = 0;
        while (offset < 16000)
        {
            const auto count = std::min(sizes[blockIndex++ % sizes.size()], 16000 - offset);
            juce::AudioBuffer<float> block(2, count);
            for (int sample = 0; sample < count; ++sample)
            {
                const auto input = static_cast<float>(0.2 * std::sin((offset + sample) * 0.13));
                block.setSample(0, sample, input);
                block.setSample(1, sample, -input);
            }
            processor.processBlock(block, midi);
            for (int sample = 0; sample < count; ++sample)
                result.push_back({ block.getSample(0, sample), block.getSample(1, sample) });
            offset += count;
        }
        return result;
    };
    check(stream({ 256 }) == stream({ 1, 17, 511, 1024 }), "Audio and Wander must be independent of host block size");
    SmallTimerProcessor mono;
    mono.setValue(SmallTimerProcessor::faderID(0, 5), 0);
    mono.setPlayConfigDetails(1, 1, 48000, 128);
    mono.prepareToPlay(48000, 128);
    juce::AudioBuffer<float> monoBlock(1, 128);
    for (int sample = 0; sample < 128; ++sample)
        monoBlock.setSample(0, sample, 0.23f);
    juce::MidiBuffer midi;
    mono.processBlock(monoBlock, midi);
    check(mono.getTotalNumInputChannels() == 1 && near(monoBlock.getSample(0, 50), 0.23f), "Mono must retain dry signal and valid bus layout");
    SmallTimerProcessor original;
    original.prepareToPlay(48000, 256);
    juce::MemoryBlock before;
    original.getStateInformation(before);
    juce::AudioBuffer<float> block(2, 256);
    original.toggleRecord();
    for (int index = 0; index < 100; ++index)
    {
        block.clear();
        if (index == 0)
            block.setSample(0, 0, 0.5f);
        original.processBlock(block, midi);
    }
    juce::MemoryBlock after;
    original.getStateInformation(after);
    check(before == after, "Recorded audio, effect tails and modulation must never enter parameter state");
    SmallTimerProcessor restored;
    restored.setStateInformation(after.getData(), static_cast<int>(after.getSize()));
    restored.prepareToPlay(48000, 256);
    bool silent = true;
    for (int index = 0; index < 200; ++index)
    {
        block.clear();
        restored.processBlock(block, midi);
        silent = silent && block.getMagnitude(0, block.getNumSamples()) == 0;
    }
    check(silent && restored.recordedSamples == 0, "Fresh sessions must contain neither effect tails nor recorded audio");
}

void testWaveformOverlay()
{
    smalltimer::WaveformHistory history;
    for (int index = 0; index < 600; ++index)
        history.push(static_cast<float>(index), -static_cast<float>(index));
    const auto wrapped = history.snapshot();
    bool chronological = true;
    for (size_t index = 0; index < wrapped.size(); ++index)
        chronological = chronological && wrapped[index].input == static_cast<float>(344 + index)
                        && wrapped[index].output == -wrapped[index].input;
    check(chronological, "Input/output waveform pairs must stay aligned and chronological after ring wrap");
    history.clear();
    check(history.snapshot().back().input == 0 && history.snapshot().back().output == 0, "Reset must clear both waveform traces");
    SmallTimerProcessor processor;
    processor.setValue(SmallTimerProcessor::faderID(0, 5), 0);
    processor.prepareToPlay(48000, 128);
    juce::MidiBuffer midi;
    juce::AudioBuffer<float> block(2, 128);
    for (int index = 0; index < 270; ++index)
    {
        for (int sample = 0; sample < 128; ++sample)
        {
            block.setSample(0, sample, static_cast<float>(std::sin((index * 128 + sample) * 0.03) * 0.3));
            block.setSample(1, sample, 0);
        }
        processor.processBlock(block, midi);
    }
    bool aligned = true;
    for (const auto& frame : processor.waveformSnapshot())
        aligned = aligned && frame.input == frame.output;
    check(aligned, "MIX zero must produce overlapping input/output traces");
    processor.toggleRecord();
    for (int sample = 0; sample < 128; ++sample)
        block.setSample(0, sample, 0.25f);
    processor.processBlock(block, midi);
    processor.togglePlay();
    block.clear();
    processor.processBlock(block, midi);
    const auto loop = processor.waveformSnapshot().back();
    check(loop.input == 0.25f && loop.output == 0.25f, "Playback input trace must show the RAM loop instead of live input");
    processor.togglePlay();
    processor.processBlock(block, midi);
    processor.setValue(SmallTimerProcessor::faderID(0, 5), 1);
    for (int index = 0; index < 400; ++index)
    {
        block.clear();
        if (index < 200)
            block.setSample(0, 127, 0.3f);
        processor.processBlock(block, midi);
    }
    bool distinct = false;
    for (const auto& frame : processor.waveformSnapshot())
        distinct = distinct || std::abs(frame.input - frame.output) > 0.00001f;
    check(distinct, "Wet echoes must produce visibly distinct input/output traces");
}

void testMotion()
{
    smalltimer::Motion motor;
    std::array<bool, 6> locks {};
    auto target = smalltimer::foreverDefaults;
    motor.tick(target, locks, false, false, 1, 1.0f / 60);
    check(motor.position[0] > smalltimer::loopDefaults[0] && motor.position[0] < target[0], "Recall must animate, not jump");
    for (int frame = 0; frame < 120; ++frame)
        motor.tick(target, locks, false, false, 1, 1.0f / 60);
    check(near(motor.position[0], target[0]), "Motor must settle at target");
    locks[0] = true;
    for (int frame = 0; frame < 600; ++frame)
    {
        motor.tick(target, locks, true, false, 1, 1.0f / 60);
        for (auto position : motor.position)
            check(position >= 0 && position <= 1, "Wander must stay bounded");
    }
    check(near(motor.position[0], target[0]), "Locked motor must ignore Wander");
    check(!near(motor.position[1], target[1]), "Unlocked motor must wander");
    const auto held = motor.position;
    motor.tick(target, locks, true, true, 1, 0.1f);
    check(motor.position == held, "Hold must freeze motors");
    motor.dragging[1] = true;
    target[1] = 0.91f;
    motor.tick(target, locks, true, true, 1, 0.01f);
    check(near(motor.position[1], 0.91f), "Drag must override motion and hold");
    motor.dragging[1] = false;
    motor.tick(target, locks, true, false, 1, 0.01f);
    check(near(motor.position[1], 0.91f), "Wander must allow a grace period after drag");
}
void testProcessor()
{
    SmallTimerProcessor processor;
    processor.setValue(SmallTimerProcessor::faderID(0, 5), 0);
    processor.prepareToPlay(48000, 256);
    const auto first = SmallTimerProcessor::faderID(0, 0);
    const auto second = SmallTimerProcessor::faderID(1, 0);
    processor.setValue(first, 0.83f);
    processor.setValue("mode", 1);
    check(near(processor.targets()[0], smalltimer::foreverDefaults[0]), "Modes must have independent banks");
    processor.setValue(second, 0.21f);
    processor.setValue("mode", 0);
    check(near(processor.targets()[0], 0.83f), "Returning to Loop must restore its bank");
    processor.storeRecall();
    processor.setValue(first, 0.12f);
    processor.setValue("lock0", 1);
    processor.recall();
    check(near(processor.value(first), 0.12f), "Recall must respect locks");
    processor.setValue("mutate", 1);
    processor.mutate();
    check(near(processor.value(first), 0.12f), "Mutation must respect locks");
    processor.setValue("lock0", 0);
    processor.recall();
    check(near(processor.value(first), 0.83f), "Recall must restore snapshot");
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    auto fill = [&buffer](float value)
    {
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < 256; ++sample)
                buffer.setSample(channel, sample, value * (channel == 0 ? 1.0f : -1.0f));
    };
    fill(0.17f);
    processor.processBlock(buffer, midi);
    check(near(buffer.getSample(0, 128), 0.17f) && near(buffer.getSample(1, 128), -0.17f), "MIX zero must pass stereo audio through unchanged");
    processor.toggleRecord();
    processor.processBlock(buffer, midi);
    check(processor.recording && processor.recordedSamples == 256, "Record must capture RAM audio");
    processor.togglePlay();
    fill(0);
    processor.processBlock(buffer, midi);
    check(processor.playing && !processor.recording && near(buffer.getSample(0, 128), 0.17f), "Play must loop captured audio");
    processor.togglePlay();
    fill(0.31f);
    processor.processBlock(buffer, midi);
    check(!processor.playing && near(buffer.getSample(0, 128), 0.31f), "Stopping playback must restore passthrough");
    processor.setValue("motion", 1);
    processor.setValue("soft", 1);
    processor.setValue("lock3", 1);
    juce::MemoryBlock state;
    processor.getStateInformation(state);
    check(state.getSize() < 16000, "State must contain parameters only");
    SmallTimerProcessor restored;
    restored.prepareToPlay(48000, 256);
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    check(near(restored.value(first), 0.83f) && near(restored.value(second), 0.21f), "Both banks must persist");
    check(restored.locked(3) && restored.value("motion") == 1 && restored.value("soft") == 1, "Switches and locks must persist");
    check(restored.recordedSamples == 0 && !restored.playing && !restored.recording, "Audio and transport must not persist");
    restored.setValue(first, 0.01f);
    restored.recall();
    check(near(restored.value(first), 0.83f), "Recall snapshots must persist");
    restored.setStateInformation("invalid", 7);
    check(near(restored.value(first), 0.83f), "Invalid state must be ignored");
    processor.prepareToPlay(10, 256);
    processor.toggleRecord();
    fill(0.2f);
    processor.processBlock(buffer, midi);
    processor.processBlock(buffer, midi);
    check(!processor.recording && processor.recordedSamples == 300, "Recording must stop cleanly at capacity");
    std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());
    check(editor->getWidth() == 660 && editor->getHeight() == 750, "Editor must retain tall hardware dimensions");
    bool foundDisplayGain = false;
    for (auto* child : editor->getChildren())
        if (auto* button = dynamic_cast<juce::TextButton*>(child))
            if (button->getTitle() == "Waveform display gain")
            {
                foundDisplayGain = true;
                juce::MemoryBlock beforeDisplayEdit, afterDisplayEdit;
                restored.getStateInformation(beforeDisplayEdit);
                check(button->getButtonText() == "VIEW x2", "Waveform display must open at shared x2 gain");
                for (const auto* expected : { "VIEW x4", "VIEW x8", "VIEW x1", "VIEW x2" })
                {
                    button->onClick();
                    check(button->getButtonText() == expected, "Waveform gain must cycle through the four display scales");
                }
                restored.getStateInformation(afterDisplayEdit);
                check(beforeDisplayEdit == afterDisplayEdit, "Display gain must not change processor parameters or saved state");
            }
    check(foundDisplayGain, "Waveform display gain control must be accessible");
    {
        SmallTimerProcessor seedProcessor;
        seedProcessor.setValue("mutate", 0);
        for (int index = 0; index < 6; ++index)
            seedProcessor.setValue("lock" + juce::String(index), 1);
        std::unique_ptr<juce::AudioProcessorEditor> seedEditor(seedProcessor.createEditor());
        juce::TextButton* spark = nullptr;
        for (auto* child : seedEditor->getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*>(child))
                if (button->getButtonText() == "NEW SEED") spark = button;
        check(spark != nullptr, "Spark button must retain the NEW SEED label");
        const auto beforeBurst = seedEditor->createComponentSnapshot(seedEditor->getLocalBounds());
        juce::MemoryBlock beforeSpark, afterSpark;
        seedProcessor.getStateInformation(beforeSpark);
        spark->onClick();
        check(spark->getToggleState(), "NEW SEED must trigger its burst immediately");
        const auto duringBurst = seedEditor->createComponentSnapshot(seedEditor->getLocalBounds());
        check(duringBurst.getPixelAt(521, 618).getBrightness() > beforeBurst.getPixelAt(521, 618).getBrightness(), "Seed burst must ripple through the centre variation lamps");
        seedProcessor.getStateInformation(afterSpark);
        check(beforeSpark == afterSpark, "Spark animation must not change locked parameters or persist animation state");
        spark->onClick();
        check(static_cast<double>(spark->getProperties()["lampLevel"]) == 1.0, "Repeated NEW SEED clicks must restart the burst");
    }
    for (auto* child : editor->getChildren())
        if (auto* button = dynamic_cast<juce::TextButton*>(child))
            if (button->getButtonText() == "HOLD" || button->getButtonText() == "REC"
                || button->getButtonText() == "PLAY" || button->getButtonText() == "RECALL"
                || button->getButtonText() == "LOOP" || button->getButtonText() == "LITTLE FOREVER"
                || button->getButtonText() == "WANDER" || button->getButtonText() == "SOFT FAILURE")
            {
                const auto previous = button->getToggleState();
                const auto previousLevel = button->getProperties().getWithDefault("lampLevel", 1.0);
                button->getProperties().set("lampLevel", 1.0);
                button->setToggleState(false, juce::dontSendNotification);
                const auto unlit = button->createComponentSnapshot(button->getLocalBounds());
                button->setToggleState(true, juce::dontSendNotification);
                const auto lit = button->createComponentSnapshot(button->getLocalBounds());
                float unlitBrightness = 0, litBrightness = 0;
                const auto centreX = button->getY() < 160 ? button->getWidth() / 2 : 19;
                const auto centreY = button->getY() < 160 ? 42 : 18;
                for (int row = centreY - 10; row < centreY + 10; ++row)
                    for (int column = centreX - 10; column < centreX + 11; ++column)
                    {
                        unlitBrightness += unlit.getPixelAt(column, row).getBrightness();
                        litBrightness += lit.getPixelAt(column, row).getBrightness();
                    }
                check(litBrightness > unlitBrightness, "Hardware lighting must brighten its active symbol");
                button->setToggleState(previous, juce::dontSendNotification);
                button->getProperties().set("lampLevel", previousLevel);
            }
    {
        SmallTimerProcessor scopeProcessor;
        scopeProcessor.setValue(SmallTimerProcessor::faderID(0, 5), 0);
        scopeProcessor.prepareToPlay(48000, 128);
        juce::AudioBuffer<float> scopeAudio(2, 128);
        auto feedScope = [&](float value)
        {
            for (int block = 0; block < 256; ++block)
            {
                for (int channel = 0; channel < 2; ++channel)
                    for (int sample = 0; sample < 128; ++sample)
                        scopeAudio.setSample(channel, sample, value);
                scopeProcessor.processBlock(scopeAudio, midi);
            }
        };
        feedScope(0.2f);
        std::unique_ptr<juce::AudioProcessorEditor> scopeEditor(scopeProcessor.createEditor());
        juce::TextButton* freezeButton = nullptr;
        juce::TextButton* gainButton = nullptr;
        for (auto* child : scopeEditor->getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*>(child))
            {
                if (button->getTitle() == "Freeze waveform display") freezeButton = button;
                if (button->getTitle() == "Waveform display gain") gainButton = button;
            }
        check(freezeButton != nullptr && gainButton != nullptr, "Freeze and view controls must be accessible");
        check(!freezeButton->getToggleState(), "Display must open unfrozen");
        auto renderScope = [&] { return scopeEditor->createComponentSnapshot(scopeEditor->getLocalBounds()); };
        auto sameTraces = [](const juce::Image& firstImage, const juce::Image& secondImage)
        {
            for (int row = 160; row < 222; ++row)
                for (int column = 101; column < 582; ++column)
                    if (firstImage.getPixelAt(column, row) != secondImage.getPixelAt(column, row))
                        return false;
            return true;
        };
        const auto liveFrame = renderScope();
        juce::MemoryBlock beforeFreeze, afterFreeze;
        scopeProcessor.getStateInformation(beforeFreeze);
        freezeButton->setToggleState(true, juce::dontSendNotification);
        freezeButton->onClick();
        feedScope(-0.2f);
        check(near(scopeAudio.getSample(0, 0), -0.2f), "Audio must continue unchanged while display is frozen");
        check(near(scopeProcessor.waveformSnapshot().back().input, -0.2f), "Live waveform capture must continue behind freeze");
        check(sameTraces(liveFrame, renderScope()), "Freeze must retain both displayed traces despite new audio");
        gainButton->onClick();
        const auto magnifiedFrame = renderScope();
        check(!sameTraces(liveFrame, magnifiedFrame), "VIEW must magnify the frozen traces");
        freezeButton->setToggleState(false, juce::dontSendNotification);
        freezeButton->onClick();
        const auto resumedFrame = renderScope();
        check(!sameTraces(magnifiedFrame, resumedFrame), "Unfreezing must show the latest live audio at the same display gain");
        scopeProcessor.getStateInformation(afterFreeze);
        check(beforeFreeze == afterFreeze, "Freeze and VIEW must leave saved processor state unchanged");
        freezeButton->setToggleState(true, juce::dontSendNotification);
        scopeEditor.reset();
        scopeEditor.reset(scopeProcessor.createEditor());
        for (auto* child : scopeEditor->getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*>(child))
                if (button->getTitle() == "Freeze waveform display")
                    check(!button->getToggleState(), "Reopening the display must clear freeze");
    }
    const auto monitor = editor->createComponentSnapshot(editor->getLocalBounds());
    check(monitor.getPixelAt(300, 177).getBrightness() > monitor.getPixelAt(300, 164).getBrightness(), "Input signal must cast a visible halo inside its lane");
    check(monitor.getPixelAt(300, 174).getBrightness() > monitor.getPixelAt(300, 177).getBrightness(), "Signal core must remain sharper and brighter than its halo");
    check(monitor.getPixelAt(300, 211).getBrightness() > monitor.getPixelAt(300, 198).getBrightness(), "Output signal must cast a visible halo inside its lane");
    const auto previousDepth = restored.value("mutate");
    restored.setValue("mutate", 0.0f);
    const auto zeroDepth = editor->createComponentSnapshot(editor->getLocalBounds());
    restored.setValue("mutate", 1.0f);
    const auto fullDepth = editor->createComponentSnapshot(editor->getLocalBounds());
    check(fullDepth.getPixelAt(485, 618).getBrightness() > zeroDepth.getPixelAt(485, 618).getBrightness(), "Variation lamps must follow mutation depth");
    check(fullDepth.getPixelAt(569, 618).getBrightness() > zeroDepth.getPixelAt(569, 618).getBrightness(), "Full variation depth must illuminate the final lamp");
    restored.setValue("mutate", previousDepth);
    const auto previousMotion = restored.value("motion");
    const auto previousHold = restored.value("hold");
    restored.setValue("motion", 0);
    restored.setValue("hold", 0);
    const auto wanderOff = editor->createComponentSnapshot(editor->getLocalBounds());
    restored.setValue("motion", 1);
    const auto wanderOn = editor->createComponentSnapshot(editor->getLocalBounds());
    restored.setValue("hold", 1);
    const auto wanderHeld = editor->createComponentSnapshot(editor->getLocalBounds());
    check(wanderOn.getPixelAt(483, 645).getBrightness() > wanderOff.getPixelAt(483, 645).getBrightness(), "Active Wander lamp must illuminate");
    check(wanderOn.getPixelAt(483, 645).getBrightness() > wanderHeld.getPixelAt(483, 645).getBrightness(), "Held motion must not show the active Wander lamp");
    check(wanderHeld.getPixelAt(483, 645).getBrightness() > wanderOff.getPixelAt(483, 645).getBrightness(), "Held motion must retain its subdued status lamp");
    restored.setValue("motion", previousMotion);
    restored.setValue("hold", previousHold);
    bool foundLatch = false;
    for (auto* child : editor->getChildren())
        if (auto* button = dynamic_cast<juce::TextButton*>(child))
            if (button->getTitle() == "Time lock")
            {
                foundLatch = true;
                const auto previousLock = restored.locked(0);
                button->onClick();
                check(restored.locked(0) != previousLock, "Time latch must toggle its existing lock parameter");
                button->onClick();
                check(restored.locked(0) == previousLock, "Time latch must restore its lock parameter on second click");
                button->setToggleState(false, juce::dontSendNotification);
                const auto freeLatch = button->createComponentSnapshot(button->getLocalBounds());
                button->setToggleState(true, juce::dontSendNotification);
                const auto lockedLatch = button->createComponentSnapshot(button->getLocalBounds());
                check(lockedLatch.getPixelAt(32, 9).getBrightness() > freeLatch.getPixelAt(32, 9).getBrightness(), "Locked latch must move its illuminated thumb to the right");
                button->setToggleState(previousLock, juce::dontSendNotification);
            }
    check(foundLatch, "Fader latch must retain a descriptive accessible title");
    for (auto* child : editor->getChildren())
        if (auto* button = dynamic_cast<juce::TextButton*>(child))
            if (button->getButtonText() == "LITTLE FOREVER")
            {
                const auto previous = button->getToggleState();
                button->setToggleState(false, juce::dontSendNotification);
                const auto unlit = button->createComponentSnapshot(button->getLocalBounds());
                button->setToggleState(true, juce::dontSendNotification);
                const auto lit = button->createComponentSnapshot(button->getLocalBounds());
                const auto faceX = button->getWidth() / 2 + 8;
                check(lit.getPixelAt(faceX, 42).getBrightness() > unlit.getPixelAt(faceX, 42).getBrightness(), "Illuminated switch face must be brighter than unlit metal");
                check(lit.getPixelAt(button->getWidth() / 2, 42).getBrightness() > 0.9f, "Amber lamp must have a bright centre");
                button->setToggleState(previous, juce::dontSendNotification);
            }
    auto click = [&editor](const juce::String& title)
    {
        for (auto* child : editor->getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*>(child))
                if (button->getButtonText() == title)
                {
                    button->onClick();
                    return;
                }
        throw std::runtime_error("Missing button: " + title.toStdString());
    };
    click("LITTLE FOREVER");
    check(restored.mode() == 1, "Forever button must select its bank");
    click("LOOP");
    check(restored.mode() == 0, "Loop button must select its bank");
    click("WANDER");
    check(restored.value("motion") == 0, "Wander button must change the existing motion parameter");
    click("HOLD");
    check(restored.value("hold") == 1, "Hold button must change the parameter");
    for (auto* child : editor->getChildren())
        if (auto* fader = dynamic_cast<juce::Slider*>(child))
            if (fader->getTitle() == "Time")
            {
                fader->onDragStart();
                fader->setValue(0.67f, juce::sendNotificationSync);
                fader->onDragEnd();
                check(near(restored.value(first), 0.67f), "Manual fader drag must write the active bank");
                check(near(restored.value(second), 0.21f), "Manual fader drag must leave the other bank alone");
            }
    if (auto* location = std::getenv("SMALL_TIMER_PREVIEW"))
    {
        SmallTimerProcessor preview;
        preview.prepareToPlay(48000, 256);
        for (int block = 0; block < 128; ++block)
        {
            for (int sample = 0; sample < 256; ++sample)
            {
                const auto time = static_cast<float>(block * 256 + sample);
                buffer.setSample(0, sample, 0.45f * std::sin(time * 0.0009f) + 0.17f * std::sin(time * 0.0037f));
            }
            preview.processBlock(buffer, midi);
        }
        preview.setValue("motion", 1);
        preview.setValue("lock1", 1);
        preview.setValue("lock4", 1);
        std::unique_ptr<juce::AudioProcessorEditor> previewEditor(preview.createEditor());
        for (auto* child : previewEditor->getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*>(child))
                if (button->getButtonText() == "NEW SEED") button->onClick();
        auto image = previewEditor->createComponentSnapshot(previewEditor->getLocalBounds(), true, 2.0f);
        juce::File destination = juce::File::getCurrentWorkingDirectory().getChildFile(location);
        auto stream = destination.createOutputStream();
        check(stream != nullptr && juce::PNGImageFormat().writeImageToStream(image, *stream), "Preview must render");
    }
}

void testBundles(const juce::File& root)
{
    juce::AudioPluginFormatManager formats;
    formats.addFormat(new juce::AudioUnitPluginFormat());
    formats.addFormat(new juce::VST3PluginFormat());
    check(registerLocalAU(root.getChildFile("AU/Small Timer.component").getFullPathName().toRawUTF8()), "AU factory must register directly from the built bundle");
    const char* paths[] { "AU/Small Timer.component", "VST3/Small Timer.vst3" };
    for (int index = 0; index < 2; ++index)
    {
        juce::OwnedArray<juce::PluginDescription> descriptions;
        formats.getFormat(index)->findAllTypesForFile(descriptions, root.getChildFile(paths[index]).getFullPathName());
        check(descriptions.size() == 1, "Built bundle must scan as one audio effect");
        juce::String error;
        auto instance = formats.createPluginInstance(*descriptions[0], 48000, 256, error);
        if (!instance)
            throw std::runtime_error(error.toStdString());
        check(instance->getPluginDescription().version == "0.2.2", "Host must load the stacked-waveform build");
        auto buses = instance->getBusesLayout();
        check(!buses.inputBuses.isEmpty() && !buses.outputBuses.isEmpty(), "Built effect must expose input and output buses");
        buses.inputBuses.set(0, juce::AudioChannelSet::stereo());
        buses.outputBuses.set(0, juce::AudioChannelSet::stereo());
        check(instance->setBusesLayout(buses), "Built effect must accept the host's stereo layout");
        instance->enableAllBuses();
        check(instance->getTotalNumInputChannels() == 2 && instance->getTotalNumOutputChannels() == 2, "Built effect must activate stereo input and output");
        instance->prepareToPlay(48000, 256);
        juce::AudioBuffer<float> audio(2, 256);
        bool foundMix = false;
        for (auto* parameter : instance->getParameters())
            if (parameter->getName(100) == "Loop Mix")
            {
                parameter->setValueNotifyingHost(0);
                foundMix = true;
            }
        check(foundMix, "Host must expose the Loop Mix parameter");
        juce::MidiBuffer midi;
        for (int block = 0; block < 256; ++block)
        {
            audio.clear();
            instance->processBlock(audio, midi);
        }
        audio.clear();
        audio.setSample(0, 100, 0.375f);
        audio.setSample(1, 120, -0.125f);
        instance->processBlock(audio, midi);
        check(near(audio.getSample(0, 100), 0.375f) && near(audio.getSample(1, 120), -0.125f), "Built plugin must pass stereo audio through the host wrapper");
        const char* controls[] { "Loop Time", "Loop Feed", "Loop Space", "Loop Drift", "Loop Grain", "Loop Mix" };
        for (int control = 0; control < 6; ++control)
            for (auto* parameter : instance->getParameters())
                if (parameter->getName(100) == controls[control])
                    parameter->setValueNotifyingHost(control == 5 ? 1.0f : 0.0f);
        for (int block = 0; block < 256; ++block)
        {
            audio.clear();
            instance->processBlock(audio, midi);
        }
        float echoPeak = 0;
        for (int block = 0; block < 32; ++block)
        {
            audio.clear();
            if (block == 0)
                audio.setSample(0, 0, 0.5f);
            instance->processBlock(audio, midi);
            if (block > 1)
                echoPeak = std::max(echoPeak, audio.getMagnitude(0, audio.getNumSamples()));
        }
        check(echoPeak > 0.2f, "Actual AU/VST3 host wrapper must produce delayed audio");
        juce::MemoryBlock state;
        instance->getStateInformation(state);
        check(state.getSize() > 0, "Built plugin must export state through the host wrapper");
        instance->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(instance->createEditorIfNeeded());
            check(editor != nullptr && editor->getWidth() == 660, "Built plugin must create its custom editor in a host");
        }
        instance->releaseResources();
        std::cout << "PASS: loaded, processed, saved/restored state and opened editor: " << paths[index] << '\n';
    }
}
}
int main()
{
    juce::ScopedJuceInitialiser_GUI initialise;
    try
    {
        testMotion();
        testSoundEngine();
        testClosedEditorMotion();
        testStreamAndState();
        testWaveformOverlay();
        testProcessor();
        if (auto* root = std::getenv("SMALL_TIMER_BUNDLE_ROOT"))
            testBundles(juce::File(root));
        std::cout << "PASS: " << checks << " checks (motor, mode banks, locks, recall, state, passthrough, RAM transport, editor)\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
