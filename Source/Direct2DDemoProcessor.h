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

#pragma once

#include <JuceHeader.h>
#include "AudioFIFO.h"
#include "Spectrum.h"
#include "ProcessorOutputFIFO.h"

enum RenderMode
{
    software = 0,
    vblankAttachmentDirect2D,
    openGL
}; 

class Direct2DDemoProcessor : public juce::AudioProcessor
{
public:
    Direct2DDemoProcessor();
    ~Direct2DDemoProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int /*index*/) override {}
    const juce::String getProgramName(int /*index*/) override { return {}; }

    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    int getFFTLength() const
    {
        return fft.getSize();
    }

    const juce::String frameRateID = "FrameRate";
    const juce::String rendererID = "Renderer";

    juce::AudioProcessorValueTreeState state;
    double sampleRate = 48000.0;
    int const fftOrder = 10;
    int fftOverlapSkipSamples = 0;
    double fftHertzPerBin = 0.0;
    AudioFIFO inputFIFO;
    ProcessorOutputFIFO outputFIFO;

    struct Parameters
    {
        Parameters(Direct2DDemoProcessor* const processor_, juce::ValueTree& tree_);

        juce::CachedValue<double> frameRate;
        juce::CachedValue<int> renderer;
    } parameters;

private:
    juce::dsp::FFT fft;
    float fftNormalizationScale = 1.0f;
    juce::dsp::WindowingFunction<float> fftWindow;
    juce::AudioBuffer<float> fftWorkBuffer;
    RealSpectrum<float> averagingSpectrum;
    float energyWeight = 1.0f;
    float const fftOverlapPercent = 75.0f;

    void processFFT();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Direct2DDemoProcessor)
};

