#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout SmallTimerProcessor::layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    const char* names[] { "Time", "Feed", "Space", "Drift", "Grain", "Mix" };
    result.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "mode", 1 }, "Mode", juce::StringArray { "Loop", "Little Forever" }, 0));
    for (int bank = 0; bank < 2; ++bank)
        for (int index = 0; index < 6; ++index)
        {
            const auto initial = (bank == 0 ? smalltimer::loopDefaults : smalltimer::foreverDefaults)[index];
            const auto prefix = bank == 0 ? "Loop " : "Forever ";
            result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { faderID(bank, index), 1 }, prefix + juce::String(names[index]), 0.0f, 1.0f, initial));
            result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "recall_" + faderID(bank, index), 1 }, "Recall " + juce::String(prefix) + names[index], 0.0f, 1.0f, initial));
        }
    for (int index = 0; index < 6; ++index)
        result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "lock" + juce::String(index), 1 }, "Lock " + juce::String(names[index]), false));
    for (auto name : { "motion", "soft", "hold" })
        result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { name, 1 }, name, false));
    result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "mutate", 1 }, "Mutate", 0.0f, 1.0f, 0.35f));
    return result;
}

SmallTimerProcessor::SmallTimerProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "SmallTimer", layout())
{
    for (int bank = 0; bank < 2; ++bank)
        for (int index = 0; index < 6; ++index)
            bankPointers[bank][index] = parameters.getRawParameterValue(faderID(bank, index));
    for (int index = 0; index < 6; ++index)
    {
        lockPointers[index] = parameters.getRawParameterValue("lock" + juce::String(index));
        faderDragging[index].store(false);
        faderEdited[index].store(false);
        publishedMotor[index].store(smalltimer::loopDefaults[index]);
    }
    modePointer = parameters.getRawParameterValue("mode");
    motionPointer = parameters.getRawParameterValue("motion");
    holdPointer = parameters.getRawParameterValue("hold");
    mutatePointer = parameters.getRawParameterValue("mutate");
    softPointer = parameters.getRawParameterValue("soft");
}

double SmallTimerProcessor::getTailLengthSeconds() const
{
    const auto settings = targets();
    const auto depth = value("motion") > 0.5f ? value("mutate") * 0.16f : 0.0f;
    const auto time = 0.04 * std::pow(50.0, std::min(1.0f, settings[0] + depth));
    const auto feed = std::min(1.0f, settings[1] + depth);
    const auto feedback = mode() == 0 ? feed * 0.94 : 0.70 + feed * 0.295;
    return 5.0 + (feedback > 0.0 ? time * std::log(0.001) / std::log(feedback) : time);
}

void SmallTimerProcessor::setFaderDragging(int index, bool dragging) { faderDragging[index].store(dragging); }
void SmallTimerProcessor::manualFaderEdit(int index) { faderEdited[index].store(true); }
smalltimer::Values SmallTimerProcessor::motorPositions() const
{
    smalltimer::Values positions;
    for (int index = 0; index < 6; ++index)
        positions[index] = publishedMotor[index].load(std::memory_order_relaxed);
    return positions;
}
void SmallTimerProcessor::updateAudioControls()
{
    const auto bank = modePointer->load(std::memory_order_relaxed) >= 0.5f ? 1 : 0;
    smalltimer::Values settings;
    std::array<bool, 6> locks {};
    for (int index = 0; index < 6; ++index)
    {
        settings[index] = bankPointers[bank][index]->load(std::memory_order_relaxed);
        locks[index] = lockPointers[index]->load(std::memory_order_relaxed) > 0.5f;
        const bool edited = faderEdited[index].exchange(false, std::memory_order_relaxed);
        audioMotor.dragging[index] = faderDragging[index].load(std::memory_order_relaxed) || edited;
    }
    audioMotor.tick(settings, locks, motionPointer->load(std::memory_order_relaxed) > 0.5f,
                    holdPointer->load(std::memory_order_relaxed) > 0.5f,
                    mutatePointer->load(std::memory_order_relaxed), static_cast<float>(64.0 / engineSampleRate));
    engine.setTargets(audioMotor.position, bank == 1, softPointer->load(std::memory_order_relaxed) > 0.5f);
    for (int index = 0; index < 6; ++index)
        publishedMotor[index].store(audioMotor.position[index], std::memory_order_relaxed);
}

juce::String SmallTimerProcessor::faderID(int bank, int index) { return "bank" + juce::String(bank) + "_" + juce::String(index); }
float SmallTimerProcessor::value(const juce::String& id) const { return parameters.getRawParameterValue(id)->load(); }
int SmallTimerProcessor::mode() const { return value("mode") >= 0.5f ? 1 : 0; }
bool SmallTimerProcessor::locked(int index) const { return value("lock" + juce::String(index)) > 0.5f; }
void SmallTimerProcessor::setValue(const juce::String& id, float newValue)
{
    if (auto* parameter = parameters.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(newValue));
}
smalltimer::Values SmallTimerProcessor::targets() const
{
    smalltimer::Values result;
    const auto bank = mode();
    for (int index = 0; index < 6; ++index)
        result[index] = value(faderID(bank, index));
    return result;
}
void SmallTimerProcessor::storeRecall()
{
    const auto bank = mode();
    for (int index = 0; index < 6; ++index)
        setValue("recall_" + faderID(bank, index), value(faderID(bank, index)));
}
void SmallTimerProcessor::recall()
{
    const juce::ScopedLock guard(seedLock);
    const auto bank = mode();
    seedUndo[bank].valid = false;
    seedKinds[bank] = -1;
    for (int index = 0; index < 6; ++index)
        if (!locked(index))
        {
            auto* parameter = parameters.getParameter(faderID(bank, index));
            parameter->beginChangeGesture();
            setValue(faderID(bank, index), value("recall_" + faderID(bank, index)));
            parameter->endChangeGesture();
        }
}
void SmallTimerProcessor::mutate()
{
    const juce::ScopedLock guard(seedLock);
    const auto bank = mode();
    const auto before = targets();
    const auto plan = smalltimer::planMutation(before, value("mutate"), bank == 1,
                                              static_cast<uint32_t>(juce::Random::getSystemRandom().nextInt()));
    SeedUndo edit;
    edit.before = before;
    edit.after = before;
    edit.previousKind = seedKinds[bank];
    for (int index = 0; index < 6; ++index)
        if (!locked(index) && std::abs(plan.values[index] - before[index]) > 0.000001f)
        {
            const auto id = faderID(bank, index);
            auto* parameter = parameters.getParameter(id);
            parameter->beginChangeGesture();
            setValue(id, plan.values[index]);
            parameter->endChangeGesture();
            edit.after[index] = value(id);
            edit.changed[index] = std::abs(edit.after[index] - before[index]) > 0.000001f;
            edit.valid = edit.valid || edit.changed[index];
        }
    if (edit.valid)
    {
        seedUndo[bank] = edit;
        seedKinds[bank] = plan.kind;
    }
}
bool SmallTimerProcessor::canUndoMutation() const
{
    const juce::ScopedLock guard(seedLock);
    const auto bank = mode();
    const auto& edit = seedUndo[bank];
    for (int index = 0; index < 6; ++index)
        if (edit.valid && edit.changed[index] && !locked(index)
            && std::abs(value(faderID(bank, index)) - edit.after[index]) < 0.000001f)
            return true;
    return false;
}
bool SmallTimerProcessor::undoMutation()
{
    const juce::ScopedLock guard(seedLock);
    const auto bank = mode();
    auto& edit = seedUndo[bank];
    if (!edit.valid)
        return false;
    bool restored = false;
    for (int index = 0; index < 6; ++index)
        if (edit.changed[index] && !locked(index)
            && std::abs(value(faderID(bank, index)) - edit.after[index]) < 0.000001f)
        {
            auto* parameter = parameters.getParameter(faderID(bank, index));
            parameter->beginChangeGesture();
            setValue(faderID(bank, index), edit.before[index]);
            parameter->endChangeGesture();
            restored = true;
        }
    if (restored)
    {
        edit.valid = false;
        seedKinds[bank] = edit.previousKind;
    }
    return restored;
}
juce::String SmallTimerProcessor::mutationName() const
{
    const juce::ScopedLock guard(seedLock);
    const auto kind = seedKinds[mode()];
    return kind >= 0 ? smalltimer::mutationNames[kind] : "";
}
void SmallTimerProcessor::prepareToPlay(double sampleRate, int)
{
    tape.setSize(2, static_cast<int>(sampleRate * 30.0));
    tape.clear();
    engineSampleRate = std::max(10.0, sampleRate);
    audioMotor = {};
    audioMotor.position = targets();
    engine.prepare(engineSampleRate, audioMotor.position, mode() == 1, value("soft") > 0.5f);
    controlCountdown = 0;
    for (int index = 0; index < 6; ++index)
        publishedMotor[index].store(audioMotor.position[index]);
    writeHead = playHead = scopeDivider = 0;
    waveform.clear();
    recording = false;
    playing = false;
    recordedSamples = 0;
    transportRequest = 0;
}
void SmallTimerProcessor::releaseResources()
{
    recording = false;
    playing = false;
    recordedSamples = 0;
}
bool SmallTimerProcessor::isBusesLayoutSupported(const BusesLayout& buses) const
{
    return buses.getMainInputChannelSet() == buses.getMainOutputChannelSet()
        && (buses.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
            || buses.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}
void SmallTimerProcessor::toggleRecord() { transportRequest.store(1); }
void SmallTimerProcessor::togglePlay() { transportRequest.store(2); }
void SmallTimerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int channel = getTotalNumInputChannels(); channel < buffer.getNumChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
    const auto request = transportRequest.exchange(0);
    if (request == 1)
    {
        const bool wasRecording = recording.load();
        recording = !wasRecording;
        playing = false;
        if (!wasRecording)
        {
            writeHead = 0;
            recordedSamples = 0;
        }
    }
    if (request == 2)
    {
        recording = false;
        playing = recordedSamples.load() > 0 && !playing.load();
        playHead = 0;
    }
    auto isRecording = recording.load();
    const auto capturedThisBlock = isRecording;
    const auto isPlaying = playing.load();
    const auto length = recordedSamples.load();
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        if (--controlCountdown <= 0)
        {
            controlCountdown = 64;
            updateAudioControls();
        }
        if (isRecording && writeHead >= tape.getNumSamples())
        {
            isRecording = false;
            recording = false;
        }
        std::array<float, 2> source {};
        const auto channels = juce::jmin(2, buffer.getNumChannels());
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto input = buffer.getSample(channel, sample);
            if (isRecording)
                tape.setSample(channel, writeHead, input);
            source[static_cast<size_t>(channel)] = isPlaying && length > 0 ? tape.getSample(channel, playHead) : input;
        }
        if (channels == 1)
            source[1] = source[0];
        const auto output = engine.process(source);
        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample(channel, sample, channels == 1 ? (output[0] + output[1]) * 0.5f : output[static_cast<size_t>(channel)]);
        if (isRecording)
            ++writeHead;
        if (isPlaying && length > 0)
            playHead = (playHead + 1) % length;
        if (++scopeDivider >= 128 && buffer.getNumChannels() > 0)
        {
            scopeDivider = 0;
            waveform.push(source[0], buffer.getSample(0, sample));
        }
    }
    if (capturedThisBlock || request == 1)
        recordedSamples = writeHead;
    processedBlocks.fetch_add(1, std::memory_order_relaxed);
}
void SmallTimerProcessor::getStateInformation(juce::MemoryBlock& data)
{
    auto state = parameters.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, data);
}
void SmallTimerProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName("SmallTimer"))
        {
            const juce::ScopedLock guard(seedLock);
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
            seedUndo = {};
            seedKinds = { -1, -1 };
        }
}
juce::AudioProcessorEditor* SmallTimerProcessor::createEditor() { return new SmallTimerEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SmallTimerProcessor(); }
