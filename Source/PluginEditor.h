#pragma once

#include "PluginProcessor.h"

class TR909AudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TR909AudioProcessorEditor(TR909AudioProcessor&);
    ~TR909AudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void editAt(juce::Point<float> pos, bool drag);
    juce::Rectangle<float> grid() const;
    int stepAt(juce::Point<float> pos) const;
    int rowAt(juce::Point<float> pos) const;

    TR909AudioProcessor& processor;
    int dragValue = -1;
    int dragRow = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TR909AudioProcessorEditor)
};
