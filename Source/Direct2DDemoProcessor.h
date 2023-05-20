#pragma once

#include <JuceHeader.h>
#include "RingBuffer.h"
#include "Spectrum.h"

enum RenderMode
{
    software = 0,
    vblankAttachmentDirect2D,
    dedicatedThreadDirect2D
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

    RealSpectrum<float> const& getSpectrum() const;
    RealSpectrum<float> const& getEnergySpectrum() const;
    int getFFTLength() const
    {
        return fft.getSize();
    }

    const juce::String frameRateID = "FrameRate";
    const juce::String rendererID = "Renderer";

    juce::AudioProcessorValueTreeState state;
    double sampleRate = 48000.0;
    int const fftOrder = 11;
    int fftOverlap = 0;
    double fftHertzPerBin = 0.0;
    juce::WaitableEvent readyEvent;
    RingBuffer ringBuffer;

    struct Parameters
    {
        Parameters(Direct2DDemoProcessor* const processor_, juce::ValueTree& tree_);

        juce::CachedValue<double> frameRate;
        juce::CachedValue<int> renderer;
    } parameters;

private:
    RealSpectrum<float> spectra[2];
    RealSpectrum<float> energySpectra[2];
    juce::Atomic<int> spectrumCount;
    int spectrumFillCount = 0;
    juce::dsp::FFT fft;
    float fftNormalizationScale = 1.0f;
    juce::dsp::WindowingFunction<float> fftWindow;
    juce::AudioBuffer<float> fftWorkBuffer;
    RealSpectrum<float> accumulatorBuffer;
    float energyWeight = 1.0f;

    void processFFT();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Direct2DDemoProcessor)
};

