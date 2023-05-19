#include "Direct2DDemoProcessor.h"
#include "Direct2DDemoEditor.h"

Direct2DDemoProcessor::Direct2DDemoProcessor() :
    AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    state(*this, nullptr, "Direct2DDemoPlugin", 
        { 
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { frameRateID, 1 }, "Frame rate", 
                juce::NormalisableRange{ 1.0f, 200.0f }, 
                60.0f),
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ rendererID, 1 }, "Render mode",
                juce::StringArray{ "Software renderer", "Direct2D from VBlankAttachment callback", "Direct2D from dedicated thread" },
                RenderMode::software)
        }),
    parameters(this, state.state),
    fft(fftOrder),
    fftWindow(fft.getSize(), juce::dsp::WindowingFunction<float>::blackmanHarris, true)
{
    for (auto& spectrumBuffer : spectrumBuffers)
    {
        spectrumBuffer.setSize(2, fft.getSize() * 2);
    }

    fftOverlap = fft.getSize() / 2;
    fftNormalizationScale = 2.0f / (float)fft.getSize();
    
#if RUN_UNIT_TESTS
    juce::UnitTestRunner runner;
    runner.runAllTests();
#endif
}

void Direct2DDemoProcessor::prepareToPlay(double sampleRate_, int samplesPerBlock)
{
    sampleRate = sampleRate_;
    fftHertzPerBin = sampleRate_ / fft.getSize();

    double bufferLengthSeconds = 4.0;
    ringBuffer.setSize(2, juce::roundToInt(bufferLengthSeconds * sampleRate_) + samplesPerBlock);
    ringBuffer.reset(0);

    auto spectraPerSecond = sampleRate_ / (double)fftOverlap;
    float constexpr energyAveragingSeconds = 0.070f;
    float energyAveragingSpectra = (float)(spectraPerSecond * energyAveragingSeconds);
    energyWeight = 1.0f - 1.0f / energyAveragingSpectra;

    for (auto& energyBuffer : energyBuffers)
    {
        energyBuffer.setSize(2, getNumBins());
        energyBuffer.clear();
    }

    accumulatorBuffer.setSize(2, getNumBins());
    accumulatorBuffer.clear();

    for (auto& spectrumBuffer : spectrumBuffers)
    {
        spectrumBuffer.clear();
    }
    spectrumFillCount = 0;
}

void Direct2DDemoProcessor::releaseResources()
{
}

bool Direct2DDemoProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo() 
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::disabled())
        return false;

    return true;
}

void Direct2DDemoProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    //
    // FFT
    //
    ringBuffer.write(buffer);
    while (ringBuffer.getNumSamplesStored() >= fft.getSize())
    {
        processFFT();
    }

    //
    // Notify the editor to paint
    //
    readyEvent.signal();
}

void printBuffer(juce::AudioBuffer<float>& buffer)
{
    for (int i = 0; i < buffer.getNumSamples(); i++)
    {
        auto text = juce::String::formatted("%04d  % 8f", i, buffer.getSample(0, i));
        DBG(text);
    }
}

void Direct2DDemoProcessor::processFFT()
{
    int spectrumIndex = (spectrumCount.get() & 1) ^ 1; // note the XOR to get the other buffer
    auto& fftBuffer = spectrumBuffers[spectrumIndex];
    auto& averageSpectrumBuffer = energyBuffers[spectrumIndex];

    //
    // Read enough samples from the ring to run the FFT
    // -but-
    // only partially advance the read count for the ring so the next FFT overlaps
    //
    fftBuffer.clear();
    ringBuffer.read(fftBuffer, fft.getSize(), fftOverlap);

    //
    // Run the FFT for each channel
    //
    for (int channel = 0; channel < fftBuffer.getNumChannels(); ++channel)
    {
        //
        // Apply windowing function to the input data
        //
        fftWindow.multiplyWithWindowingTable(fftBuffer.getWritePointer(channel), fft.getSize());

        //
        // Run the FFT; performFrequencyOnlyForwardTransform takes the magnitude of the complex FFT output
        //
        fft.performFrequencyOnlyForwardTransform(fftBuffer.getWritePointer(channel), true);

        //
        // Normalize the FFT output; can use applyGain here since these are real numbers
        //
        fftBuffer.applyGain(channel, 0, fftBuffer.getNumSamples() / 2, fftNormalizationScale);
    }

    //
    // Spectrum averaging
    //
    float newScale = 1.0f - energyWeight;
    for (int channel = 0; channel < accumulatorBuffer.getNumChannels(); ++channel)
    {
        for (int bin = 0; bin < getNumBins(); ++bin)
        {
            auto accumulator = accumulatorBuffer.getSample(channel, bin);
            accumulator = accumulator * energyWeight + fftBuffer.getSample(channel, bin) * newScale;
            accumulatorBuffer.setSample(channel, bin, accumulator);
        }

        jassert(averageSpectrumBuffer.getNumSamples() == accumulatorBuffer.getNumSamples());

        averageSpectrumBuffer.copyFrom(channel, 0,
            accumulatorBuffer,
            channel, 0,
            averageSpectrumBuffer.getNumSamples());
    }

    ++spectrumCount;
}

juce::AudioBuffer<float> const& Direct2DDemoProcessor::getSpectrum() const
{
    int spectrumIndex = spectrumCount.get() & 1;
    return spectrumBuffers[spectrumIndex];
}

juce::AudioBuffer<float> const& Direct2DDemoProcessor::getEnergySpectrum() const
{
    int spectrumIndex = spectrumCount.get() & 1;
    return energyBuffers[spectrumIndex];
}

juce::AudioProcessorEditor* Direct2DDemoProcessor::createEditor()
{
    return new Direct2DDemoEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Direct2DDemoProcessor();
}

template <typename T> static juce::CachedValue<T> makeCachedValue(juce::ValueTree& tree, juce::Identifier id)
{
    for (auto&& child : tree)
    {
        if (child["id"] == id.toString())
        {
            return juce::CachedValue<T>(child, "value", nullptr);
        }
    }

    return {};
}

Direct2DDemoProcessor::Parameters::Parameters(Direct2DDemoProcessor* const processor_, juce::ValueTree& tree_) :
    frameRate(makeCachedValue<double>(tree_, processor_->frameRateID)),
    renderer(makeCachedValue<int>(tree_, processor_->rendererID))
{
}
