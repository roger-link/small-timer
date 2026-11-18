#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Motion.h"
#include "Mutation.h"
#include "SoundEngine.h"
#include "WaveformHistory.h"

class SmallTimerEditor;
class SmallTimerProcessor final : public juce::AudioProcessor
{
public:
    SmallTimerProcessor();
    const juce::String getName() const override { return "Small Timer"; }
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override;
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    int mode() const;
    float value(const juce::String&) const;
    void setValue(const juce::String&, float);
    static juce::String faderID(int bank, int index);
    smalltimer::Values targets() const;
    bool locked(int index) const;
    void storeRecall();
    void recall();
    void mutate();
    bool undoMutation();
    bool canUndoMutation() const;
    juce::String mutationName() const;
    void toggleRecord();
    void togglePlay();
    void setFaderDragging(int index, bool dragging);
    void manualFaderEdit(int index);
    smalltimer::Values motorPositions() const;
    std::atomic<uint64_t> processedBlocks { 0 };
    std::atomic<bool> recording { false }, playing { false };
    std::atomic<int> recordedSamples { 0 };
    smalltimer::WaveformHistory::Snapshot waveformSnapshot() const { return waveform.snapshot(); }
    juce::AudioProcessorValueTreeState parameters;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    struct SeedUndo
    {
        smalltimer::Values before {}, after {};
        std::array<bool, 6> changed {};
        bool valid = false;
        int previousKind = -1;
    };
    mutable juce::CriticalSection seedLock;
    std::array<SeedUndo, 2> seedUndo {};
    std::array<int, 2> seedKinds { -1, -1 };
    juce::AudioBuffer<float> tape;
    smalltimer::WaveformHistory waveform;
    smalltimer::SoundEngine engine;
    smalltimer::Motion audioMotor;
    std::array<std::array<std::atomic<float>*, 6>, 2> bankPointers {};
    std::array<std::atomic<float>*, 6> lockPointers {};
    std::atomic<float>* modePointer = nullptr;
    std::atomic<float>* motionPointer = nullptr;
    std::atomic<float>* holdPointer = nullptr;
    std::atomic<float>* mutatePointer = nullptr;
    std::atomic<float>* softPointer = nullptr;
    std::array<std::atomic<bool>, 6> faderDragging {}, faderEdited {};
    std::array<std::atomic<float>, 6> publishedMotor {};
    double engineSampleRate = 48000;
    int controlCountdown = 0;
    void updateAudioControls();
    int writeHead = 0, playHead = 0, scopeDivider = 0;
    std::atomic<int> transportRequest { 0 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmallTimerProcessor)
};
