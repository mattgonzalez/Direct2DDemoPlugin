/*

Copyright(c) 2023 Matthew Gonzalez

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

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

    inputFIFO.setSize(2, fft.getSize() * 4);
    inputFIFO.reset(0);

    outputFIFO.setSize(2 /* numItems */, 2 /* numChannels */, fft.getSize());
    outputFIFO.reset();

    fftOverlapSkipSamples = fft.getSize() - juce::roundToInt(fftOverlapPercent * 0.01f * fft.getSize());
    auto spectraPerSecond = (float)sampleRate_ / (float)fftOverlapSkipSamples;
    float constexpr energyAveragingSeconds = 0.1f;
    energyWeight = 1.0f - 1.0f / (spectraPerSecond * energyAveragingSeconds);

    averagingSpectrum = RealSpectrum<float>{}.withChannels(2).withFFTSize(fft.getSize());
    averagingSpectrum.clear();
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
    etw.log();

    juce::ScopedNoDenormals noDenormals;

    //
    // Store samples in the FIFO and run the FFT if there's enough data
    //
    inputFIFO.write(buffer);
    while (inputFIFO.getNumSamplesStored() >= fft.getSize())
    {
        processFFT();
    }

    //
    // Notify the editor to paint
    //
    readyEvent.signal();
}

void Direct2DDemoProcessor::processFFT()
{
    auto processorOutput = outputFIFO.getWritePointer();
    auto& spectrum = processorOutput->spectrum;
    auto& averageSpectrum = processorOutput->averageSpectrum;

    //
    // Read enough samples from the ring to run the FFT
    // -but-
    // only partially advance the read count for the ring so the next FFT overlaps
    //
    fftWorkBuffer.clear();
    inputFIFO.read(fftWorkBuffer, fft.getSize(), fftOverlapSkipSamples);

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
    for (int channel = 0; channel < averagingSpectrum.getNumChannels(); ++channel)
    {
        for (int bin = 0; bin < averagingSpectrum.getNumBins(); ++bin)
        {
            auto accumulator = averagingSpectrum.getBinMagnitude(channel, bin);
            accumulator = accumulator * energyWeight + spectrum.getBinMagnitude(channel, bin) * newScale;
            averagingSpectrum.setBinValue(channel, bin, accumulator);
        }
    }

    averageSpectrum.copyFrom(averagingSpectrum);

    //
    // Bump the ring buffer
    //
    outputFIFO.advanceWritePosition();
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
