#include "Direct2DDemoProcessor.h"
#include "Direct2DDemoEditor.h"
#include "UnitTests.h"

Direct2DDemoProcessor::Direct2DDemoProcessor() :
    AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    state(*this, nullptr, "Direct2DDemoPlugin", 
        { 
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { frameRateID, 1 }, "Frame rate", 
                juce::NormalisableRange{ 10.0f, 200.0f }, 
                60.0f),
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ rendererID, 1 }, "Render mode",
                juce::StringArray{ "Software renderer", "Direct2D from VBlankAttachment callback", "Direct2D from dedicated thread" },
                RenderMode::software)
        }),
    parameters(this, state.state),
    fft(fftOrder),
    fftWindow(fft.getSize(), juce::dsp::WindowingFunction<float>::blackmanHarris, true),
    fftWorkBuffer(2, fft.getSize() * 2)
{
    fftOverlap = fft.getSize() / 2;
    fftNormalizationScale = 2.0f / (float)fft.getSize();

#if RUN_UNIT_TESTS
    UnitTests unitTests;
    juce::UnitTestRunner runner;
    runner.runAllTests();
#endif
}

void Direct2DDemoProcessor::prepareToPlay(double sampleRate_, int /*samplesPerBlock*/)
{
    sampleRate = sampleRate_;
    fftHertzPerBin = sampleRate_ / fft.getSize();

    ringBuffer.setSize(2, fft.getSize() * 4);
    ringBuffer.reset(0);

    auto spectraPerSecond = sampleRate_ / (double)fftOverlap;
    float constexpr energyAveragingSeconds = 0.070f;
    float energyAveragingSpectra = (float)(spectraPerSecond * energyAveragingSeconds);
    energyWeight = 1.0f - 1.0f / energyAveragingSpectra;

    for (auto& energyBuffer : energySpectra)
    {
        energyBuffer = RealSpectrum<float>{}.withChannels(2).withFFTSize(fft.getSize());
        energyBuffer.clear();
    }

    accumulatorBuffer = RealSpectrum<float>{}.withChannels(2).withFFTSize(fft.getSize());
    accumulatorBuffer.clear();

    for (auto& spectrumBuffer : spectra)
    {
        spectrumBuffer = RealSpectrum<float>{}.withChannels(2).withFFTSize(fft.getSize());
        spectrumBuffer.clear();
    }
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
    auto& spectrum = spectra[spectrumIndex];
    auto& averageSpectrum = energySpectra[spectrumIndex];

    //
    // Read enough samples from the ring to run the FFT
    // -but-
    // only partially advance the read count for the ring so the next FFT overlaps
    //
    fftWorkBuffer.clear();
    ringBuffer.read(fftWorkBuffer, fft.getSize(), fftOverlap);

    //
    // Run the FFT for each channel
    //
    for (int channel = 0; channel < fftWorkBuffer.getNumChannels(); ++channel)
    {
        //
        // Apply windowing function to the input data
        //
        fftWindow.multiplyWithWindowingTable(fftWorkBuffer.getWritePointer(channel), fft.getSize());

        //
        // Run the FFT; performFrequencyOnlyForwardTransform takes the magnitude of the complex FFT output
        //
        fft.performFrequencyOnlyForwardTransform(fftWorkBuffer.getWritePointer(channel), true);
    }

    //
    // Store the normalized FFT results
    // 
    spectrum.copyFrom(fftWorkBuffer, fft.getSize() / 2 + 1, fftNormalizationScale);

    //
    // Spectrum averaging
    //
    float newScale = 1.0f - energyWeight;
    for (int channel = 0; channel < accumulatorBuffer.getNumChannels(); ++channel)
    {
        for (int bin = 0; bin < accumulatorBuffer.getNumBins(); ++bin)
        {
            auto accumulator = accumulatorBuffer.getBinMagnitude(channel, bin);
            accumulator = accumulator * energyWeight + spectrum.getBinMagnitude(channel, bin) * newScale;
            accumulatorBuffer.setBinValue(channel, bin, accumulator);
        }
    }

    averageSpectrum.copyFrom(accumulatorBuffer);

    ++spectrumCount;
}

RealSpectrum<float> const& Direct2DDemoProcessor::getSpectrum() const
{
    int spectrumIndex = spectrumCount.get() & 1;
    return spectra[spectrumIndex];
}

RealSpectrum<float> const& Direct2DDemoProcessor::getEnergySpectrum() const
{
    int spectrumIndex = spectrumCount.get() & 1;
    return energySpectra[spectrumIndex];
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
