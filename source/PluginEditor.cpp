#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    addAndMakeVisible (inspectButton);

    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };

    // ==========================
    // Input Gain slider
    // ==========================
    addAndMakeVisible (inputGainSlider);
    inputGainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    inputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    inputGainSlider.setTextValueSuffix (" dB");
    inputGainSlider.setNumDecimalPlacesToDisplay (1);

    addAndMakeVisible (inputGainLabel);
    inputGainLabel.setJustificationType (juce::Justification::centred);
    inputGainLabel.setText ("Input", juce::dontSendNotification);

    // ==========================
    // Drive slider
    // ==========================
    addAndMakeVisible (driveSlider);
    driveSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);

    addAndMakeVisible (driveLabel);
    driveLabel.setJustificationType (juce::Justification::centred);
    driveLabel.setText ("Drive", juce::dontSendNotification);

    // ==========================
    // Output Gain slider
    // ==========================
    addAndMakeVisible (outputGainSlider);
    outputGainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    outputGainSlider.setTextValueSuffix (" dB");
    outputGainSlider.setNumDecimalPlacesToDisplay (1);

    addAndMakeVisible (outputGainLabel);
    outputGainLabel.setJustificationType (juce::Justification::centred);
    outputGainLabel.setText ("Output", juce::dontSendNotification);

    // ==========================
    // Cab toggle
    // ==========================
    addAndMakeVisible (cabButton);
    cabButton.setButtonText ("Cab");
    cabButton.setClickingTogglesState (true);

    auto& apvts = processorRef.getAPVTS();
    cabAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, "cab", cabButton);

    addAndMakeVisible (cabLabel);
    cabLabel.setJustificationType (juce::Justification::centred);
    cabLabel.setText ("Cab", juce::dontSendNotification);

    // ==========================
    // Tone mix slider
    // ==========================
    addAndMakeVisible (toneSlider);
    toneSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    toneSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    toneSlider.setNumDecimalPlacesToDisplay (0);
    toneSlider.textFromValueFunction = [] (double v)
    {
        return juce::String ((int) std::round (v * 100.0)) + " %";
    };
    toneSlider.valueFromTextFunction = [] (const juce::String& t)
    {
        const auto cleaned = t.retainCharacters ("0123456789.");
        const double pct = cleaned.getDoubleValue();
        return juce::jlimit (0.0, 1.0, pct / 100.0);
    };

    addAndMakeVisible (toneLabel);
    toneLabel.setJustificationType (juce::Justification::centred);
    toneLabel.setText ("Mix", juce::dontSendNotification);

    // ==========================
    // Bass slider
    // ==========================
    addAndMakeVisible (bassSlider);
    bassSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    bassSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    bassSlider.setTextValueSuffix (" dB");
    bassSlider.setNumDecimalPlacesToDisplay (1);

    addAndMakeVisible (bassLabel);
    bassLabel.setJustificationType (juce::Justification::centred);
    bassLabel.setText ("Bass", juce::dontSendNotification);

    // ==========================
    // Mid Peak slider
    // ==========================
    addAndMakeVisible (midPeakLSlider);
    midPeakLSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    midPeakLSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    midPeakLSlider.setTextValueSuffix (" dB");
    midPeakLSlider.setNumDecimalPlacesToDisplay (1);

    addAndMakeVisible (midPeakLabel);
    midPeakLabel.setJustificationType (juce::Justification::centred);
    midPeakLabel.setText ("Mid", juce::dontSendNotification);

    // ==========================
    // Treble slider
    // ==========================
    addAndMakeVisible (trebleSlider);
    trebleSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    trebleSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    trebleSlider.setTextValueSuffix (" dB");
    trebleSlider.setNumDecimalPlacesToDisplay (1);

    addAndMakeVisible (trebleLabel);
    trebleLabel.setJustificationType (juce::Justification::centred);
    trebleLabel.setText ("Treble", juce::dontSendNotification);

    // ==========================
    // Presence slider
    // ==========================
    addAndMakeVisible (presenceSlider);
    presenceSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    presenceSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    presenceSlider.setTextValueSuffix (" dB");
    presenceSlider.setNumDecimalPlacesToDisplay (1);

    addAndMakeVisible (presenceLabel);
    presenceLabel.setJustificationType (juce::Justification::centred);
    presenceLabel.setText ("Presence", juce::dontSendNotification);

    // ==========================
    // Drive Mode selector
    // ==========================
    addAndMakeVisible (driveModeBox);
    driveModeBox.addItem ("Smooth",    1);  // SmoothTanh
    driveModeBox.addItem ("Soft Clip", 2);  // SoftClip
    driveModeBox.addItem ("Asym Tube", 3);  // AsymTanh

    addAndMakeVisible (driveModeLabel);
    driveModeLabel.setJustificationType (juce::Justification::centred);
    driveModeLabel.setText ("Drive Mode", juce::dontSendNotification);

    // ==========================
    // NAM section
    // ==========================
    addAndMakeVisible (namEnabledButton);
    namEnabledButton.setButtonText ("NAM");
    namEnabledButton.setClickingTogglesState (true);
    namEnabledAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, "namEnabled", namEnabledButton);

    addAndMakeVisible (namLoadButton);
    namLoadButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Load NAM Model",
            juce::File::getSpecialLocation (juce::File::userHomeDirectory),
            "*.nam");

        fileChooser->launchAsync (
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                if (fc.getResults().size() > 0)
                {
                    auto f = fc.getResult();
                    processorRef.loadNamModelAsync (f);
                    namModelLabel.setText (f.getFileNameWithoutExtension(),
                                          juce::dontSendNotification);
                }
            });
    };

    addAndMakeVisible (namModelLabel);
    namModelLabel.setJustificationType (juce::Justification::centredLeft);
    const auto savedName = processorRef.getNamModelName();
    namModelLabel.setText (savedName.isEmpty() ? "No model loaded" : savedName,
                           juce::dontSendNotification);

    // APVTS attachments (must come after all items are added)
    inAttach     = std::make_unique<SliderAttachment> (apvts, "inDb",     inputGainSlider);
    driveAttach  = std::make_unique<SliderAttachment> (apvts, "drive",    driveSlider);
    mixAttach    = std::make_unique<SliderAttachment> (apvts, "mix",      toneSlider);
    bassAttach   = std::make_unique<SliderAttachment> (apvts, "bassDb",   bassSlider);
    midAttach    = std::make_unique<SliderAttachment> (apvts, "midDb",    midPeakLSlider);
    trebleAttach   = std::make_unique<SliderAttachment> (apvts, "trebleDb",   trebleSlider);
    presenceAttach = std::make_unique<SliderAttachment> (apvts, "presenceDb", presenceSlider);
    outAttach      = std::make_unique<SliderAttachment> (apvts, "outDb",      outputGainSlider);
    modeAttach   = std::make_unique<ComboAttachment>  (apvts, "mode",     driveModeBox);

    // ==========================
    // A/B Comparison buttons
    // ==========================
    addAndMakeVisible (abStoreA);
    addAndMakeVisible (abLoadA);
    addAndMakeVisible (abStoreB);
    addAndMakeVisible (abLoadB);
    addAndMakeVisible (abLabel);

    abLabel.setJustificationType (juce::Justification::centredLeft);
    abLabel.setText ("A/B:", juce::dontSendNotification);

    abLoadA.setEnabled (false);
    abLoadB.setEnabled (false);

    abStoreA.onClick = [this]
    {
        abSlotA = processorRef.getAPVTS().state.createCopy();
        abLoadA.setEnabled (true);
    };

    abLoadA.onClick = [this]
    {
        if (abSlotA.isValid())
            processorRef.getAPVTS().replaceState (abSlotA.createCopy());
    };

    abStoreB.onClick = [this]
    {
        abSlotB = processorRef.getAPVTS().state.createCopy();
        abLoadB.setEnabled (true);
    };

    abLoadB.onClick = [this]
    {
        if (abSlotB.isValid())
            processorRef.getAPVTS().replaceState (abSlotB.createCopy());
    };

    startTimerHz (30);

    setSize (600, 500);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::timerCallback()
{
    // Smooth decay: ~85% per frame at 30 Hz gives roughly 0.5 s half-life
    constexpr float kDecay = 0.85f;
    meterPeakL    = juce::jmax (processorRef.getInputPeakLeft(),    meterPeakL    * kDecay);
    meterPeakR    = juce::jmax (processorRef.getInputPeakRight(),   meterPeakR    * kDecay);
    outMeterPeakL = juce::jmax (processorRef.getOutputPeakLeft(),   outMeterPeakL * kDecay);
    outMeterPeakR = juce::jmax (processorRef.getOutputPeakRight(),  outMeterPeakR * kDecay);
    repaint (meterBounds);
    repaint (outMeterBounds);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);

    auto area = getLocalBounds();
    g.drawFittedText ("generic amp sim v0.2",
                      area.removeFromTop (40),
                      juce::Justification::centred,
                      1);

    auto drawMeter = [&] (juce::Rectangle<int> bounds, const char* label,
                          float peakL, float peakR)
    {
        if (bounds.isEmpty())
            return;

        auto mb = bounds;

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (mb.toFloat(), 3.0f);

        g.setColour (juce::Colours::grey);
        g.setFont (10.0f);
        g.drawText (label, mb.removeFromTop (14), juce::Justification::centred, false);

        // Reserve bottom for dBFS readout
        auto dbArea = mb.removeFromBottom (16);

        mb.reduce (3, 2);
        const int barW = (mb.getWidth() - 2) / 2;
        auto lBar = mb.removeFromLeft (barW);
        mb.removeFromLeft (2);
        auto rBar = mb.removeFromLeft (barW);

        auto drawBar = [&] (juce::Rectangle<int> bar, float level)
        {
            const float dB    = juce::Decibels::gainToDecibels (level, -60.0f);
            const float norm  = juce::jlimit (0.0f, 1.0f, (dB + 60.0f) / 60.0f);
            const int   fillH = (int) (bar.getHeight() * norm);

            const auto colour = (dB < -12.0f) ? juce::Colour (0xff4caf50)
                              : (dB <  -3.0f)  ? juce::Colour (0xffffeb3b)
                                               : juce::Colour (0xfff44336);
            g.setColour (colour);
            g.fillRect (bar.withTop (bar.getBottom() - fillH));
            g.setColour (juce::Colours::darkgrey);
            g.drawRect (bar);
        };

        drawBar (lBar, peakL);
        drawBar (rBar, peakR);

        // dBFS numeric readout (left channel)
        const float dBL = juce::Decibels::gainToDecibels (peakL, -60.0f);
        g.setColour (juce::Colours::lightgrey);
        g.setFont (9.0f);
        const auto dbStr = (dBL <= -60.0f)
                         ? juce::String ("-inf")
                         : juce::String ((int) std::round (dBL)) + "dB";
        g.drawText (dbStr, dbArea, juce::Justification::centred, false);
    };

    drawMeter (meterBounds,    "IN",  meterPeakL,    meterPeakR);
    drawMeter (outMeterBounds, "OUT", outMeterPeakL, outMeterPeakR);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (20);

    // ====== Vertical input meter — full-height strip on the far left ======
    meterBounds = area.removeFromLeft (28);
    area.removeFromLeft (6); // gap between meter and controls

    // ====== Vertical output meter — full-height strip on the far right ======
    outMeterBounds = area.removeFromRight (28);
    area.removeFromRight (6); // gap between meter and controls

    // ====== Top row: Input / Drive / Output / Cab ======
    auto topRow = area.removeFromTop (130);
    const int quarter = topRow.getWidth() / 4;

    {
        auto col       = topRow.removeFromLeft (quarter).reduced (10);
        auto labelArea = col.removeFromTop (20);
        inputGainLabel .setBounds (labelArea);
        inputGainSlider.setBounds (col);
    }
    {
        auto col       = topRow.removeFromLeft (quarter).reduced (10);
        auto labelArea = col.removeFromTop (20);
        driveLabel .setBounds (labelArea);
        driveSlider.setBounds (col);
    }
    {
        auto col       = topRow.removeFromLeft (quarter).reduced (10);
        auto labelArea = col.removeFromTop (20);
        outputGainLabel .setBounds (labelArea);
        outputGainSlider.setBounds (col);
    }
    {
        auto col       = topRow.removeFromLeft (quarter).reduced (10);
        auto labelArea = col.removeFromTop (20);
        cabLabel  .setBounds (labelArea);
        cabButton .setBounds (col.withSizeKeepingCentre (80, 30));
    }

    // ====== NAM row: toggle / load button / model name ======
    auto namRow = area.removeFromTop (40);
    namRow.reduce (0, 6);
    namEnabledButton.setBounds (namRow.removeFromLeft (60).withSizeKeepingCentre (54, 28));
    namRow.removeFromLeft (6);
    namLoadButton.setBounds (namRow.removeFromLeft (90).withSizeKeepingCentre (86, 28));
    namRow.removeFromLeft (8);
    namModelLabel.setBounds (namRow);

    // ====== Second row: Mix / Bass / Mid / Treble / Presence ======
    auto secondRow = area.removeFromTop (130);
    const int q2   = secondRow.getWidth() / 5;

    {
        auto col       = secondRow.removeFromLeft (q2).reduced (10);
        auto labelArea = col.removeFromTop (20);
        toneLabel .setBounds (labelArea);
        toneSlider.setBounds (col);
    }
    {
        auto col       = secondRow.removeFromLeft (q2).reduced (10);
        auto labelArea = col.removeFromTop (20);
        bassLabel .setBounds (labelArea);
        bassSlider.setBounds (col);
    }
    {
        auto col       = secondRow.removeFromLeft (q2).reduced (10);
        auto labelArea = col.removeFromTop (20);
        midPeakLabel  .setBounds (labelArea);
        midPeakLSlider.setBounds (col);
    }
    {
        auto col       = secondRow.removeFromLeft (q2).reduced (10);
        auto labelArea = col.removeFromTop (20);
        trebleLabel .setBounds (labelArea);
        trebleSlider.setBounds (col);
    }
    {
        auto col       = secondRow.removeFromLeft (q2).reduced (10);
        auto labelArea = col.removeFromTop (20);
        presenceLabel  .setBounds (labelArea);
        presenceSlider .setBounds (col);
    }

    // ====== Drive mode selector ======
    auto modeRow = area.removeFromTop (40);
    driveModeBox  .setBounds (modeRow.removeFromLeft (200).reduced (10));
    driveModeLabel.setBounds (driveModeBox.getBounds().translated (0, -20));

    // ====== A/B Comparison row ======
    auto abRow     = area.removeFromTop (44);
    const int bw   = 72;
    const int bh   = 28;
    auto abRowPad  = abRow.reduced (4, 8);

    abLabel .setBounds (abRowPad.removeFromLeft (32));
    abStoreA.setBounds (abRowPad.removeFromLeft (bw).withSizeKeepingCentre (bw - 4, bh));
    abLoadA .setBounds (abRowPad.removeFromLeft (bw).withSizeKeepingCentre (bw - 4, bh));
    abRowPad.removeFromLeft (12); // gap between A and B
    abStoreB.setBounds (abRowPad.removeFromLeft (bw).withSizeKeepingCentre (bw - 4, bh));
    abLoadB .setBounds (abRowPad.removeFromLeft (bw).withSizeKeepingCentre (bw - 4, bh));

    // ====== Inspector button ======
    auto buttonArea = area.removeFromBottom (44);
    inspectButton.setBounds (buttonArea.withSizeKeepingCentre (150, 30));
}
