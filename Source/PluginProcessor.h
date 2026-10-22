#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

class TR909AudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kVoices = 11;
    static constexpr int kSteps = 16;

    TR909AudioProcessor();
    ~TR909AudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TR-909"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> playStep { -1 };

    static int houseCell(int voice, int step);
    static bool houseAccent(int step);

private:
    struct Shot
    {
        const float* data = nullptr;
        int length = 0;
        double pos = 0.0;
        double inc = 1.0;
        float gain = 0.0f;
        int voice = -1;
        bool on = false;
    };

    void loadSamples();
    void trigger(int voice, float amp, int delaySamples);
    int voiceForNote(int note) const;
    float param(const juce::String& id) const;

    std::array<std::vector<float>, kVoices> samples;
    std::array<Shot, 32> shots {};
    double sampleRate = 44100.0;
    double lastBeat = 0.0;
    bool hasLastBeat = false;

    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TR909AudioProcessor)
};
