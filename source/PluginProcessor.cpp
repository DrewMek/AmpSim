#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
    , apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

namespace ParamIDs
{
    static constexpr const char* inDb     = "inDb";
    static constexpr const char* drive    = "drive";
    static constexpr const char* mix      = "mix";
    static constexpr const char* bassDb   = "bassDb";
    static constexpr const char* midDb    = "midDb";
    static constexpr const char* trebleDb   = "trebleDb";
    static constexpr const char* presenceDb = "presenceDb";
    static constexpr const char* outDb      = "outDb";
    static constexpr const char* mode       = "mode";
    static constexpr const char* cab        = "cab";
    static constexpr const char* namEnabled = "namEnabled";
}

static float dbToGain (float db) { return juce::Decibels::decibelsToGain (db); }

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    using APF   = juce::AudioParameterFloat;
    using APC   = juce::AudioParameterChoice;
    using APB   = juce::AudioParameterBool;
    using Range = juce::NormalisableRange<float>;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Gain staging in dB (human units)
    params.push_back (std::make_unique<APF> (
        ParamIDs::inDb, "Input", Range (-24.0f, 24.0f, 0.1f), 14.0f));

    params.push_back (std::make_unique<APF> (
        ParamIDs::outDb, "Output", Range (-24.0f, 24.0f, 0.1f), -14.0f));

    // Drive amount — logarithmic feel so the usable 0.1–3 range isn't crammed
    // into the bottom quarter of the knob. Centre of knob travel = drive 3.
    {
        Range driveRange (0.1f, 10.0f, 0.1f);
        driveRange.setSkewForCentre (3.0f);
        params.push_back (std::make_unique<APF> (
            ParamIDs::drive, "Drive", driveRange, 2.0f));
    }

    // Blend (0..1)
    params.push_back (std::make_unique<APF> (
        ParamIDs::mix, "Tone", Range (0.0f, 1.0f, 0.001f), 1.0f));

    // Tone controls as dB boosts/cuts (standard amp-ish feel)
    params.push_back (std::make_unique<APF> (
        ParamIDs::bassDb, "Bass", Range (-12.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<APF> (
        ParamIDs::midDb, "Mid", Range (-12.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<APF> (
        ParamIDs::trebleDb, "Treble", Range (-12.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<APF> (
        ParamIDs::presenceDb, "Presence", Range (-12.0f, 12.0f, 0.1f), 0.0f));

    // Drive mode
    params.push_back (std::make_unique<APC> (
        ParamIDs::mode, "Mode",
        juce::StringArray { "Smooth", "Soft Clip", "Asym Tube" },
        0));

    // Cab on/off
    params.push_back (std::make_unique<APB> (
        ParamIDs::cab, "Cab", true));

    // NAM mode on/off
    params.push_back (std::make_unique<APB> (
        ParamIDs::namEnabled, "NAM", false));

    return { params.begin(), params.end() };
}

void PluginProcessor::syncParamsFromAPVTS()
{
    // Read host parameters (atomic floats)
    const float inDb     = apvts.getRawParameterValue (ParamIDs::inDb)->load();
    const float outDb    = apvts.getRawParameterValue (ParamIDs::outDb)->load();
    const float drive    = apvts.getRawParameterValue (ParamIDs::drive)->load();
    const float mix      = apvts.getRawParameterValue (ParamIDs::mix)->load();
    const float bassDb      = apvts.getRawParameterValue (ParamIDs::bassDb)->load();
    const float midDb       = apvts.getRawParameterValue (ParamIDs::midDb)->load();
    const float trebleDb    = apvts.getRawParameterValue (ParamIDs::trebleDb)->load();
    const float presenceDb  = apvts.getRawParameterValue (ParamIDs::presenceDb)->load();
    const int   modeIdx  = (int) apvts.getRawParameterValue (ParamIDs::mode)->load();
    const bool  cabOn    = apvts.getRawParameterValue (ParamIDs::cab)->load() >= 0.5f;

    // Map to DSP-friendly values
    params.inputGain  = dbToGain (inDb);
    params.outputGain = dbToGain (outDb);
    params.drive      = drive;
    params.toneMix    = juce::jlimit (0.0f, 1.0f, mix);

    // Convert EQ dB -> linear
    params.bassGain   = dbToGain (bassDb);
    params.trebleGain = dbToGain (trebleDb);

    // Mid is special because you update IIR coeffs
    if (params.midGainDb != midDb)
    {
        params.midGainDb = midDb;
        if (currentSampleRate > 0.0)
            updateMidCoeffs();
    }

    if (params.presenceGainDb != presenceDb)
    {
        params.presenceGainDb = presenceDb;
        if (currentSampleRate > 0.0)
            updatePresenceCoeffs();
    }

    // Mode mapping
    switch (modeIdx)
    {
        case 0: params.driveMode = DriveMode::SmoothTanh; break;
        case 1: params.driveMode = DriveMode::SoftClip;   break;
        case 2: params.driveMode = DriveMode::AsymTanh;   break;
        default: params.driveMode = DriveMode::SmoothTanh; break;
    }

    cabEnabled = cabOn;
}

PluginProcessor::~PluginProcessor() = default;

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return cabEnabled ? irTailSeconds : 0.0;
}

//==============================================================================
int PluginProcessor::getNumPrograms()
{
    return 1;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void PluginProcessor::updateMidCoeffs() const
{
    constexpr float midFreqHz = 800.0f;
    constexpr float Q         = 0.9f;

    auto gain = juce::Decibels::decibelsToGain (params.midGainDb);

    auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        (double) currentSampleRate, midFreqHz, Q, gain);

    *midPeakL.coefficients = *coeffs;
    *midPeakR.coefficients = *coeffs;
}

void PluginProcessor::updatePresenceCoeffs() const
{
    constexpr float presenceFreqHz = 3500.0f;
    constexpr float Q              = 0.707f;

    auto gain = juce::Decibels::decibelsToGain (params.presenceGainDb);

    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        currentSampleRate, presenceFreqHz, Q, gain);

    *presenceShelfL.coefficients = *coeffs;
    *presenceShelfR.coefficients = *coeffs;
}

void PluginProcessor::setMidGainDb (float db)
{
    params.midGainDb = juce::jlimit (-12.0f, 12.0f, db);
    if (currentSampleRate > 0.0)
        updateMidCoeffs();
}

//==============================================================================
int PluginProcessor::normalizeInputAndGetActiveChannels (juce::AudioBuffer<float>& buffer,
                                                         const BlockContext& ctx)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = ctx.numSamples;
    const int bufCh      = ctx.bufCh;
    const int inCh       = ctx.inCh;

    if (numSamples == 0 || bufCh == 0)
        return 0;

    // --- FIX RIGHT-ONLY INPUT (do this FIRST)
    if (bufCh >= 2)
    {
        const int N = juce::jmin (numSamples, 256);
        const float* L = buffer.getReadPointer (0);
        const float* R = buffer.getReadPointer (1);

        double eL = 0.0, eR = 0.0;
        for (int i = 0; i < N; ++i)
        {
            const double l = (double) L[i];
            const double r = (double) R[i];
            eL += l * l;
            eR += r * r;
        }

        constexpr double ratio = 1.0e-3; // 0.1% energy
        if (eL < eR * ratio)
            buffer.copyFrom (0, 0, buffer, 1, 0, numSamples); // R -> L
        else if (eR < eL * ratio)
            buffer.copyFrom (1, 0, buffer, 0, 0, numSamples); // L -> R
    }

    // Clear any channels beyond stereo
    for (int ch = 2; ch < bufCh; ++ch)
        buffer.clear (ch, 0, numSamples);

    // If bus is mono but buffer is stereo, duplicate L -> R
    if (bufCh >= 2 && inCh == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    return juce::jmin (2, bufCh);
}

//==============================================================================
void PluginProcessor::stageMakeCleanAndHPF (juce::AudioBuffer<float>& buffer, const BlockContext& ctx)
{
    cleanBuffer.makeCopyOf (buffer, true);

    for (int ch = 0; ch < ctx.activeChans; ++ch)
    {
        float* c = cleanBuffer.getWritePointer (ch);
        auto& hpf = (ch == 0 ? leftHPF : rightHPF);

        for (int i = 0; i < ctx.numSamples; ++i)
            c[i] = hpf.process (c[i]);
    }

    // Start main buffer from clean signal
    for (int ch = 0; ch < ctx.activeChans; ++ch)
        buffer.copyFrom (ch, 0, cleanBuffer, ch, 0, ctx.numSamples);
}

void PluginProcessor::stagePreampOversampled (juce::AudioBuffer<float>& buffer, const BlockContext& ctx)
{
    juce::dsp::AudioBlock<float> ioBlock (buffer);
    auto activeBlock = ioBlock.getSubsetChannelBlock (0, (size_t) ctx.activeChans);

    auto osBlock = oversampler.processSamplesUp (activeBlock);
    const int numSamplesOS = (int) osBlock.getNumSamples();

    for (int ch = 0; ch < (int) osBlock.getNumChannels(); ++ch)
    {
        float* s = osBlock.getChannelPointer ((size_t) ch);
        auto& lpfOS = (ch == 0 ? leftFilterOS : rightFilterOS);

        for (int i = 0; i < numSamplesOS; ++i)
        {
            float x = s[i] * params.inputGain;
            float distorted = applyDrive (x, params);
            s[i] = lpfOS.process (distorted);
        }
    }

    oversampler.processSamplesDown (activeBlock);
}

void PluginProcessor::stageNAM (juce::AudioBuffer<float>& buffer, const BlockContext& ctx)
{
    // Swap in a newly loaded model if one is waiting.
    // namModelPending is the race-free signal from the loader thread.
    if (namModelPending.load (std::memory_order_acquire) && namStagingMutex.try_lock())
    {
        if (namStagedModel)
        {
            namModel = std::move (namStagedModel);
            namModel->ResetAndPrewarm ((double) currentSampleRate, ctx.numSamples);
            namModelPending.store (false, std::memory_order_release);
        }
        namStagingMutex.unlock();
    }

    if (! namModel)
        return;

    // Apply input gain before NAM — mirrors the classic path and preserves the
    // +14 dB in / -14 dB out balance already in the plugin. This also lets the
    // Input knob act as a drive control into the model.
    for (int ch = 0; ch < ctx.activeChans; ++ch)
    {
        float* data = buffer.getWritePointer (ch);
        for (int i = 0; i < ctx.numSamples; ++i)
            data[i] *= params.inputGain;
    }

    // NAM process() takes NAM_SAMPLE** (pointer-to-channel-pointer).
    // Models are 1-in/1-out; we call once per channel with a single-element array.
    for (int ch = 0; ch < ctx.activeChans; ++ch)
    {
        NAM_SAMPLE* channelPtr = buffer.getWritePointer (ch);
        namModel->process (&channelPtr, &channelPtr, ctx.numSamples);
    }
}

void PluginProcessor::loadNamModelAsync (const juce::File& file)
{
    if (namLoaderThread && namLoaderThread->joinable())
        namLoaderThread->detach();

    namModelName = file.getFileNameWithoutExtension();
    namModelPath = file.getFullPathName();

    namLoaderThread = std::make_unique<std::thread> (
        [this, path = file.getFullPathName().toStdString()]
        {
            try
            {
                auto loaded = nam::get_dsp (std::filesystem::u8path (path));
                std::lock_guard<std::mutex> lock (namStagingMutex);
                namStagedModel = std::move (loaded);
                namModelPending.store (true, std::memory_order_release);
            }
            catch (...) {}
        });
    namLoaderThread->detach();
}

void PluginProcessor::stageToneEQ (juce::AudioBuffer<float>& buffer, const BlockContext& ctx)
{
    for (int ch = 0; ch < ctx.activeChans; ++ch)
    {
        float* out         = buffer.getWritePointer (ch);
        const float* clean = cleanBuffer.getReadPointer (ch);

        auto& bassLPF    = (ch == 0 ? leftBassLPF     : rightBassLPF);
        auto& mid        = (ch == 0 ? midPeakL        : midPeakR);
        auto& presence   = (ch == 0 ? presenceShelfL  : presenceShelfR);

        for (int i = 0; i < ctx.numSamples; ++i)
        {
            float mixed = (1.0f - params.toneMix) * clean[i]
                        + (        params.toneMix) * out[i];

            mixed = mid.processSample (mixed);

            float lowBand  = bassLPF.process (mixed);
            float highBand = mixed - lowBand;

            float toned = (lowBand  * params.bassGain
                         + highBand * params.trebleGain);

            out[i] = presence.processSample (toned);
        }
    }
}

void PluginProcessor::stageCab (juce::AudioBuffer<float>& buffer, const BlockContext& ctx)
{
    if (cabEnabled)
    {
        juce::dsp::AudioBlock<float> cabBlock (buffer);
        auto subset = cabBlock.getSubsetChannelBlock (0, (size_t) ctx.activeChans);
        juce::dsp::ProcessContextReplacing<float> cabCtx (subset);
        cabConvolution.process (cabCtx);
    }

    // post-cab filter
    for (int ch = 0; ch < ctx.activeChans; ++ch)
    {
        float* out = buffer.getWritePointer (ch);
        auto& airHPF = (ch == 0 ? postCabHPF_L : postCabHPF_R);

        for (int i = 0; i < ctx.numSamples; ++i)
            out[i] = airHPF.process (out[i]);
    }
}

void PluginProcessor::stageOutput (juce::AudioBuffer<float>& buffer, const BlockContext& ctx)
{
    for (int ch = 0; ch < ctx.activeChans; ++ch)
    {
        float* out = buffer.getWritePointer (ch);
        for (int i = 0; i < ctx.numSamples; ++i)
            out[i] *= params.outputGain;
    }
}

// Parses a WAV binary blob (RIFF/WAVE) and returns its duration in seconds.
// Scans chunks rather than assuming a fixed 44-byte header so it handles
// files with extra metadata chunks (JUNK, LIST, fact, etc.) correctly.
static double parseWavDurationSeconds (const void* data, size_t size)
{
    const auto* b = static_cast<const uint8_t*> (data);

    // Validate RIFF/WAVE signature
    if (size < 12
        || b[0] != 'R' || b[1] != 'I' || b[2] != 'F' || b[3] != 'F'
        || b[8] != 'W' || b[9] != 'A' || b[10] != 'V' || b[11] != 'E')
        return 1.0;

    uint16_t numChannels   = 0;
    uint32_t irSampleRate  = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataBytes     = 0;
    bool     gotFmt        = false;
    bool     gotData       = false;

    auto readU16 = [&] (size_t pos) -> uint16_t
    {
        return (uint16_t) (b[pos] | (b[pos + 1] << 8));
    };
    auto readU32 = [&] (size_t pos) -> uint32_t
    {
        return (uint32_t) (b[pos] | (b[pos+1] << 8) | (b[pos+2] << 16) | (b[pos+3] << 24));
    };

    size_t pos = 12;
    while (pos + 8 <= size && ! (gotFmt && gotData))
    {
        const uint32_t chunkSize = readU32 (pos + 4);

        if (b[pos]=='f' && b[pos+1]=='m' && b[pos+2]=='t' && b[pos+3]==' ' && pos + 24 <= size)
        {
            numChannels   = readU16 (pos + 10);
            irSampleRate  = readU32 (pos + 12);
            bitsPerSample = readU16 (pos + 22);
            gotFmt = true;
        }
        else if (b[pos]=='d' && b[pos+1]=='a' && b[pos+2]=='t' && b[pos+3]=='a')
        {
            dataBytes = chunkSize;
            gotData   = true;
        }

        pos += 8 + chunkSize;
        if (chunkSize & 1) ++pos; // WAV chunks are word-aligned
    }

    if (! gotFmt || ! gotData || numChannels == 0 || bitsPerSample == 0 || irSampleRate == 0)
        return 1.0;

    const uint32_t bytesPerFrame = numChannels * (bitsPerSample / 8u);
    if (bytesPerFrame == 0)
        return 1.0;

    return (dataBytes / bytesPerFrame) / (double) irSampleRate;
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Convolution is stereo (we process 2 channels as a block)
    juce::dsp::ProcessSpec cabSpec;
    cabSpec.sampleRate       = sampleRate;
    cabSpec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    cabSpec.numChannels      = 2;

    const float fs   = (float) sampleRate;

    cabConvolution.reset();
    cabConvolution.prepare (cabSpec);

    cabConvolution.loadImpulseResponse(
        static_cast<const void*> (BinaryData::TJ_66_Alnico_Silver_R121SM545_1_wav),
        static_cast<size_t> (BinaryData::TJ_66_Alnico_Silver_R121SM545_1_wavSize),
        juce::dsp::Convolution::Stereo::no,
        juce::dsp::Convolution::Trim::yes,
        0, // size: 0 = use full IR
        juce::dsp::Convolution::Normalise::yes
    );

    irTailSeconds = parseWavDurationSeconds (
        BinaryData::TJ_66_Alnico_Silver_R121SM545_1_wav,
        (size_t) BinaryData::TJ_66_Alnico_Silver_R121SM545_1_wavSize);

    // mid peak filters are mono instances
    juce::dsp::ProcessSpec monoSpec = cabSpec;
    monoSpec.numChannels = 1;

    midPeakL.prepare (monoSpec);
    midPeakR.prepare (monoSpec);
    midPeakL.reset();
    midPeakR.reset();

    presenceShelfL.prepare (monoSpec);
    presenceShelfR.prepare (monoSpec);
    presenceShelfL.reset();
    presenceShelfR.reset();

    updateMidCoeffs();
    updatePresenceCoeffs();

    oversampler.reset();
    oversampler.initProcessing ((size_t) samplesPerBlock);

    const auto osFactor = (float) oversampler.getOversamplingFactor();
    const float fsOS    = fs * osFactor;

    int osLatencyInOS = (int) oversampler.getLatencyInSamples();
    int osLatencyBase = (int) std::round (osLatencyInOS / osFactor);
    int cabLatency = (int) cabConvolution.getLatency();
    int totalLatency = osLatencyBase + cabLatency;

    setLatencySamples (totalLatency);

    DBG ("SR=" << sampleRate
         << " osFactor=" << osFactor
         << " fsOS=" << fsOS
         << " osLatency=" << osLatencyInOS
         << " osLatencyBase=" << osLatencyBase
         << " cabLatency=" << cabLatency
         << " totalLatency=" << totalLatency);

    // Resize and clear the clean buffer
    cleanBuffer.setSize (2, samplesPerBlock);
    cleanBuffer.clear();

    auto updateCoeffs = [] (OnePoleLowpass& lpf, float cutoff, float sRate)
    {
        const float x = std::exp (-2.0f * juce::MathConstants<float>::pi * cutoff / sRate);
        lpf.setCoefficients (1.0f - x, x);
    };

    // Pre-distortion HPF (Base Rate)
    updateCoeffs (leftHPF.lpf,  params.hpfCutoffHz, fs);
    updateCoeffs (rightHPF.lpf, params.hpfCutoffHz, fs);

    // Post-distortion LPF (Oversampled Rate)
    updateCoeffs (leftFilterOS,  params.toneCutoffHz, fsOS);
    updateCoeffs (rightFilterOS, params.toneCutoffHz, fsOS);

    // Bass Split LPF (Base Rate)
    updateCoeffs (leftBassLPF,  params.bassSplitHz, fs);
    updateCoeffs (rightBassLPF, params.bassSplitHz, fs);

    // Post-cab rumble HPF (Base Rate) — removes sub-bass below ~40 Hz after cab IR
    updateCoeffs (postCabHPF_L.lpf, params.hpfCutoffHz, fs);
    updateCoeffs (postCabHPF_R.lpf, params.hpfCutoffHz, fs);

    if (namModel)
        namModel->ResetAndPrewarm (sampleRate, samplesPerBlock);

    DBG ("prepareToPlay SR=" << sampleRate
         << " block=" << samplesPerBlock
         << " inCh=" << getTotalNumInputChannels()
         << " outCh=" << getTotalNumOutputChannels());

    syncParamsFromAPVTS();
}

void PluginProcessor::releaseResources()
{
    // free up anything if needed when audio stops
}

//==============================================================================
bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
   #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
   #else
    const auto out = layouts.getMainOutputChannelSet();
    const auto in  = layouts.getMainInputChannelSet();

    if (out != juce::AudioChannelSet::mono()
     && out != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (in != juce::AudioChannelSet::disabled() && in != out)
    {
        // Also accept mono-in / stereo-out — the standard guitar amp sim layout.
        // This lets Reaper (and other hosts) route a mono DI track at full level
        // into the plugin and receive stereo cab output.
        if (!(in  == juce::AudioChannelSet::mono()
           && out == juce::AudioChannelSet::stereo()))
            return false;
    }
   #endif

    return true;
   #endif
}

float PluginProcessor::applyDrive (float x, const AmpParameters& p)
{
    switch (p.driveMode)
    {
        case DriveMode::SmoothTanh:
        {
            float d = p.drive * x;
            return std::tanh (d);
        }

        case DriveMode::SoftClip:
        {
            float d = p.drive * x;
            return d / (1.0f + std::abs (d));
        }

        case DriveMode::AsymTanh:
        {
            const float bias = 0.3f;
            float biased     = p.drive * (x + bias);
            float y          = std::tanh (biased);

            float biasOnly   = std::tanh (p.drive * bias);
            return y - biasOnly; // remove DC offset
        }

        default:
        {
            float d = p.drive * x;
            return std::tanh (d);
        }
    }
}

//==============================================================================
void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    BlockContext ctx;
    ctx.numSamples = buffer.getNumSamples();
    ctx.bufCh      = buffer.getNumChannels();
    ctx.inCh       = getTotalNumInputChannels();

    if (ctx.numSamples == 0 || ctx.bufCh == 0)
        return;

    syncParamsFromAPVTS();

    ctx.activeChans = normalizeInputAndGetActiveChannels (buffer, ctx);
    if (ctx.activeChans == 0)
        return;

    // Update input peak meters (pre-gain, raw signal)
    for (int ch = 0; ch < ctx.activeChans; ++ch)
    {
        const float* data = buffer.getReadPointer (ch);
        float peak = 0.0f;
        for (int i = 0; i < ctx.numSamples; ++i)
            peak = juce::jmax (peak, std::abs (data[i]));
        (ch == 0 ? inputPeakLeft : inputPeakRight).store (peak);
    }
    if (ctx.activeChans == 1)
        inputPeakRight.store (inputPeakLeft.load());

    stageMakeCleanAndHPF (buffer, ctx);

    if (apvts.getRawParameterValue (ParamIDs::namEnabled)->load() >= 0.5f)
        stageNAM (buffer, ctx);
    else
        stagePreampOversampled (buffer, ctx);

    stageToneEQ (buffer, ctx);
    stageCab (buffer, ctx);
    stageOutput (buffer, ctx);

    // Output peak meters (post everything)
    for (int ch = 0; ch < ctx.activeChans; ++ch)
    {
        const float* data = buffer.getReadPointer (ch);
        float peak = 0.0f;
        for (int i = 0; i < ctx.numSamples; ++i)
            peak = juce::jmax (peak, std::abs (data[i]));
        (ch == 0 ? outputPeakLeft : outputPeakRight).store (peak);
    }
    if (ctx.activeChans == 1)
        outputPeakRight.store (outputPeakLeft.load());

    for (int ch = ctx.activeChans; ch < ctx.bufCh; ++ch)
        buffer.clear (ch, 0, ctx.numSamples);
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    apvts.state.setProperty ("namModelPath", namModelPath, nullptr);
    juce::MemoryOutputStream mos (destData, true);
    apvts.state.writeToStream (mos);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData (data, (size_t) sizeInBytes);
    if (tree.isValid())
        apvts.replaceState (tree);

    // Reload NAM model if a path was saved
    const juce::String savedPath = apvts.state.getProperty ("namModelPath").toString();
    if (savedPath.isNotEmpty())
    {
        const juce::File modelFile (savedPath);
        if (modelFile.existsAsFile())
            loadNamModelAsync (modelFile);
    }

    // ensure DSP picks up restored values
    syncParamsFromAPVTS();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}