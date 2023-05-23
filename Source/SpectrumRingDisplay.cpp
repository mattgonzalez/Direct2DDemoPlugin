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

#include "SpectrumRingDisplay.h"

SpectrumRingDisplay::SpectrumRingDisplay(Direct2DDemoProcessor& processor_, Direct2DAttachment& direct2DAttachment_) :
    audioProcessor(processor_),
    direct2DAttachment(direct2DAttachment_)
{
}

void SpectrumRingDisplay::paint(juce::Graphics& g, juce::Rectangle<float> bounds, ProcessorOutput const* const processorOutput)
{
    auto const& averageSpectrum = processorOutput->averageSpectrum;

    //
    // Limit the bin range; skip DC bin
    //
    int numBins = juce::roundToInt((float)averageSpectrum.getNumBins() * 2000.0f / (float)audioProcessor.getSampleRate()) - 1;

    //
    // Calculate bass energy
    //
    int numEnergyBins = 0;
    int firstEnergyBin = 0;
    float peakBassEnergy = 0.0f;
    for (int channel = 0; channel < averageSpectrum.getNumChannels(); ++channel)
    {
        float frequency = 50.0f;
        int bin = (int)std::floor(frequency / audioProcessor.fftHertzPerBin);
        firstEnergyBin = bin;
        numEnergyBins = 0;
        while (frequency <= 200.0f)
        {
            auto energy = averageSpectrum.getBinMagnitude(channel, bin);
            if (energy > peakBassEnergy)
            {
                peakBassEnergy = energy;
            }
            frequency += (float)audioProcessor.fftHertzPerBin;
            bin++;
            numEnergyBins++;
        }
    }

    //
    // Make ring segments if necessary
    //
    int numRings = numBins;
    makeSegmentPaths(numRings, bounds);

    if (ringBounds.isEmpty())
    {
        return;
    }

    //
    // Scale ring segments by the bass energy and the window aspect ratio
    //
    float xScale = 1.0f, yScale = 1.0f;
    if (peakBassEnergy > 0.3f)
    {
        float bump = 0.5f * peakBassEnergy;
        xScale += bump;
        yScale += bump;
    }

    auto aspectRatioWidthOverHeight = ringBounds.getAspectRatio();
    if (aspectRatioWidthOverHeight > 1.0f)
    {
        xScale *= aspectRatioWidthOverHeight;
    }
    else
    {
        yScale /= aspectRatioWidthOverHeight;
    }

    //
    // Paint the ring segments
    //
    gradient = juce::ColourGradient{ juce::Colour{ 0xff6eecfc }, bounds.getCentre(), juce::Colours::hotpink, bounds.getTopLeft(), true };
    auto const translateAndScale = juce::AffineTransform::scale(xScale, yScale).translated(bounds.getCentre());
    juce::NormalisableRange<float> gainRange{ 0.0f, 12.0f };
    for (int channel = 0; channel < averageSpectrum.getNumChannels(); ++channel)
    {
        for (int ring = 0; ring < numRings; ++ring)
        {
	        float magnitude = averageSpectrum.getBinMagnitude(channel, ring + 1); // ring + 1 to skip DC bin
	        magnitude = juce::jmin(1.0f, magnitude);
	
	        g.setColour(gradient.getColourAtPosition(magnitude));
	
            float gainDecibels = gainRange.convertFrom0to1((float)ring / (float)numRings);
            float gain = juce::Decibels::decibelsToGain(gainDecibels);
	        paintSegments(g, channel, ring, magnitude * gain, translateAndScale);
        }
    }
}

void SpectrumRingDisplay::makeSegmentPaths(int numRings, juce::Rectangle<float> bounds)
{
    if (segmentPaths.size() != numRings || ringBounds != bounds)
    {
        float maxRadius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.35f;
        auto center = bounds.getCentre().toFloat();
        auto logNumRings = std::log((float)numRings);
        auto pixelsPerRing = maxRadius / numRings;
        auto pixelsPerLogRing = maxRadius / logNumRings;

        segmentPaths.clear();
        ringBounds = bounds;

        for (int ring = 0; ring < numRings; ++ring)
        {
            std::unique_ptr<juce::Path> path = std::make_unique<juce::Path>();

            float radius = maxRadius - std::log(ring + 0.5f) * pixelsPerLogRing;
            juce::Point<float> origin{};
            float startAngle = (segmentAngleSpacingRadians - segmentFilledAngleRadians) * 0.5f;
            float endAngle = startAngle + segmentFilledAngleRadians;
            path->startNewSubPath(origin.getPointOnCircumference(radius, startAngle));
            path->addCentredArc(0.0f, 0.0f,
                radius, radius,
                0.0f,
                startAngle, endAngle,
                false);
            float innerRadius = maxRadius - std::log(ring + 1.4f) * pixelsPerLogRing;
            path->lineTo(origin.getPointOnCircumference(innerRadius, endAngle));
            path->addCentredArc(0.0f, 0.0f,
                innerRadius, innerRadius, 0.0f,
                endAngle, startAngle,
                false);
            path->closeSubPath();

            segmentPaths.add(path.release());

            radius -= pixelsPerRing;
        }
    }
}

void SpectrumRingDisplay::paintSegments(juce::Graphics& g, int channel, int ring, float magnitude, auto const& translateAndScale)
{
    int numSegments = (int)std::floor(juce::MathConstants<float>::halfPi * segmentAngleSpacingInverse * magnitude);
    numSegments = juce::jmin(numSegments, segmentsPerQuarterCircle);

    float upperAngle = -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::pi * channel;
    float lowerAngle = upperAngle - segmentAngleSpacingRadians;

    auto const segmentPath = segmentPaths[ring];
    for (int i = 0; i < numSegments; ++i)
    {
        g.fillPath(*segmentPath, juce::AffineTransform::rotation(upperAngle).followedBy(translateAndScale));
        g.fillPath(*segmentPath, juce::AffineTransform::rotation(lowerAngle).followedBy(translateAndScale));

        upperAngle += segmentAngleSpacingRadians;
        lowerAngle -= segmentAngleSpacingRadians;
    }
}
