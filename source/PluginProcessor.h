#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <NAM/dsp.h>
#include <NAM/get_dsp.h>

#include <mutex>
#include <thread>

#if (MSVC)
#include "ipps.h"
#endif

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    juce::AudioProcessorValueTreeState&       getAPVTS()       { return apvts; }
    const juce::AudioProcessorValueTreeState& getAPVTS() const { return apvts; }

    float getInputPeakLeft()   const { return inputPeakLeft.load();   }
    float getInputPeakRight()  const { return inputPeakRight.load();  }
    float getOutputPeakLeft()  const { return outputPeakLeft.load();  }
    float getOutputPeakRight() const { return outputPeakRight.load(); }

    enum class DriveMode
    {
        SmoothTanh = 0,
        SoftClip,
        AsymTanh
    };

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Internal param sync (APVTS -> DSP params)
    void syncParamsFromAPVTS();

    void  setMidGainDb (float db);
    float getMidGainDb() const { return params.midGainDb; }

    void loadNamModelAsync (const juce::File& file);
    bool isNamModelLoaded() const { return namModel != nullptr; }
    juce::String getNamModelName() const { return namModelName; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    juce::AudioBuffer<float> cleanBuffer;

    struct BlockContext
    {
        int numSamples   = 0;
        int bufCh        = 0;
        int inCh         = 0;
        int activeChans  = 0;
    };

    int  normalizeInputAndGetActiveChannels (juce::AudioBuffer<float>& buffer, const BlockContext& ctx);

    void stageMakeCleanAndHPF   (juce::AudioBuffer<float>& buffer, const BlockContext& ctx);
    void stagePreampOversampled (juce::AudioBuffer<float>& buffer, const BlockContext& ctx);
    void stageNAM               (juce::AudioBuffer<float>& buffer, const BlockContext& ctx);
    void stageToneEQ            (juce::AudioBuffer<float>& buffer, const BlockContext& ctx);
    void stageCab               (juce::AudioBuffer<float>& buffer, const BlockContext& ctx);
    void stageOutput            (juce::AudioBuffer<float>& buffer, const BlockContext& ctx);

    juce::dsp::Oversampling<float> oversampler {
        2,
        2, // 4x
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR
    };

    juce::dsp::Convolution cabConvolution;
    bool cabEnabled = true;

    struct AmpParameters
    {
        float inputGain    = 1.0f;    // linear
        float drive        = 2.0f;
        float toneMix      = 1.0f;    // 0..1
        float outputGain   = 1.0f;    // linear
        float toneCutoffHz = 12000.0f;
        float hpfCutoffHz  = 40.0f;

        float midGainDb      = 0.0f;    // -12..+12
        float presenceGainDb = 0.0f;  // -12..+12

        float bassGain     = 1.0f;    // linear
        float trebleGain   = 1.0f;    // linear
        float bassSplitHz  = 250.0f;

        DriveMode driveMode = DriveMode::SmoothTanh;
    };

    struct OnePoleLowpass
    {
        float a0 = 0.0f, b1 = 0.0f, z1 = 0.0f;

        void setCoefficients (float newA0, float newB1)
        {
            a0 = newA0; b1 = newB1; z1 = 0.0f;
        }

        float process (float x)
        {
            z1 = a0 * x + b1 * z1;
            return z1;
        }
    };

    struct OnePoleHighpass
    {
        OnePoleLowpass lpf;

        void setCoefficients (float newA0, float newB1) { lpf.setCoefficients (newA0, newB1); }

        float process (float x)
        {
            const float low = lpf.process (x);
            return x - low;
        }
    };

    float applyDrive (float x, const AmpParameters& p);
    void  updateMidCoeffs() const;
    void  updatePresenceCoeffs() const;

    AmpParameters params;
    double currentSampleRate = 44100.0;

    OnePoleHighpass leftHPF, rightHPF;
    OnePoleLowpass  leftFilterOS, rightFilterOS;
    OnePoleLowpass  leftBassLPF, rightBassLPF;

    OnePoleHighpass postCabHPF_L, postCabHPF_R;

    juce::dsp::IIR::Filter<float> midPeakL, midPeakR;
    juce::dsp::IIR::Filter<float> presenceShelfL, presenceShelfR;

    // NAM model
    std::unique_ptr<nam::DSP> namModel;
    std::unique_ptr<nam::DSP> namStagedModel;
    std::mutex                namStagingMutex;
    std::atomic<bool>         namModelPending { false }; // set by loader thread, cleared after swap
    juce::String              namModelName;
    juce::String              namModelPath;
    std::unique_ptr<std::thread> namLoaderThread;

    std::atomic<float> inputPeakLeft   { 0.0f };
    std::atomic<float> inputPeakRight  { 0.0f };
    std::atomic<float> outputPeakLeft  { 0.0f };
    std::atomic<float> outputPeakRight { 0.0f };

    double irTailSeconds = 1.0;
};
