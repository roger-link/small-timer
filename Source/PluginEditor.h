#pragma once
#include "PluginProcessor.h"

class HardwareLook final : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
};

class SmallTimerEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SmallTimerEditor(SmallTimerProcessor&);
    ~SmallTimerEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void setMode(int);
    void finishDrag(int);
    SmallTimerProcessor& processor;
    HardwareLook look;
    smalltimer::Motion motor;
    std::array<juce::Slider, 6> faders;
    std::array<juce::TextButton, 6> locks;
    std::array<juce::String, 6> gestureIDs;
    juce::TextButton loop { "LOOP" }, forever { "LITTLE FOREVER" }, motion { "WANDER" }, soft { "SOFT FAILURE" };
    juce::TextButton hold { "HOLD" }, rec { "REC" }, play { "PLAY" }, recall { "RECALL" };
    juce::Slider mutate;
    juce::TextButton seed { "NEW SEED" };
    juce::TextButton undoSeed { "UNDO SEED" };
    juce::TextButton scopeGain { "VIEW x2" };
    juce::TextButton scopeFreeze { "FREEZE" };
    smalltimer::WaveformHistory::Snapshot displayedWaveform {};
    int scopeGainStep = 1;
    juce::TooltipWindow tips { this, 650 };
    int lastMode = 0;
    double lastTick = 0;
    double lastAudioTick = 0;
    double recallLightUntil = 0;
    double seedBurstStarted = 0;
    uint64_t lastAudioBlock = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmallTimerEditor)
};
