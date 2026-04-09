#pragma once

#include "PluginProcessor.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include <juce_gui_extra/juce_gui_extra.h>

class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    PluginProcessor& processorRef;

    juce::TextButton inspectButton { "Inspect" };
    std::unique_ptr<melatonin::Inspector> inspector;

    juce::Slider inputGainSlider, driveSlider, outputGainSlider;
    juce::Slider toneSlider, bassSlider, midPeakLSlider, trebleSlider, presenceSlider;

    juce::Label inputGainLabel, driveLabel, outputGainLabel;
    juce::Label toneLabel, bassLabel, midPeakLabel, trebleLabel, presenceLabel;
    juce::Label driveModeLabel;

    juce::ToggleButton cabButton;
    juce::Label cabLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> cabAttach;

    juce::ComboBox driveModeBox;

    // Input level meter
    float meterPeakL = 0.0f, meterPeakR = 0.0f;
    juce::Rectangle<int> meterBounds;

    // Output level meter
    float outMeterPeakL = 0.0f, outMeterPeakR = 0.0f;
    juce::Rectangle<int> outMeterBounds;

    // A/B comparison
    juce::ValueTree abSlotA, abSlotB;
    juce::TextButton abStoreA { "Store A" }, abLoadA { "A" };
    juce::TextButton abStoreB { "Store B" }, abLoadB { "B" };
    juce::Label abLabel;

    // NAM section
    juce::ToggleButton namEnabledButton;
    juce::TextButton   namLoadButton { "Load .nam" };
    juce::Label        namModelLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> namEnabledAttach;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> inAttach, driveAttach, mixAttach, bassAttach, midAttach, trebleAttach, presenceAttach, outAttach;
    std::unique_ptr<ComboAttachment>  modeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
