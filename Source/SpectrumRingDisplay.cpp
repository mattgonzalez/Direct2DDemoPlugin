#include "SpectrumRingDisplay.h"

SpectrumRingDisplay::SpectrumRingDisplay(Direct2DDemoProcessor& processor_, Direct2DAttachment& direct2DAttachment_, Spectrum<float> const& energyPaintSpectrum_) :
    audioProcessor(processor_),
    direct2DAttachment(direct2DAttachment_),
    energyPaintSpectrum(energyPaintSpectrum_)
{
}

void SpectrumRingDisplay::paint(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    int numBins = juce::roundToInt((float)energyPaintSpectrum.getNumBins() * 2000.0f / (float)audioProcessor.getSampleRate());

    //
    // Calculate bass energy
    //
    int numEnergyBins = 0;
    int firstEnergyBin = 0;
    float peakBassEnergy = 0.0f;
    for (int channel = 0; channel < energyPaintSpectrum.getNumChannels(); ++channel)
    {
        float frequency = 50.0f;
        int bin = (int)std::floor(frequency / audioProcessor.fftHertzPerBin);
        firstEnergyBin = bin;
        numEnergyBins = 0;
        while (frequency <= 200.0f)
        {
            auto energy = energyPaintSpectrum.getBinMagnitude(channel, bin);
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
    float maxRadius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.35f;
    auto center = bounds.getCentre().toFloat();
    auto logNumRings = std::log((float)numRings);
    auto pixelsPerRing = maxRadius / numRings;
    auto pixelsPerLogRing = maxRadius / logNumRings;
    float segmentAngleSpacingRadians = 0.75f * juce::MathConstants<float>::pi / 32.0f;
    float segmentFilledAngleRadians = segmentAngleSpacingRadians * 0.8f;

    if (segmentPaths.size() != numRings || ringBounds != bounds)
    {
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

        gradient = juce::ColourGradient{ juce::Colour{ 0xff6eecfc }, center, juce::Colours::hotpink, { 0.0f, 0.0f }, true };
    }

    if (ringBounds.isEmpty())
    {
        return;
    }

    //
    // Paint ring segments
    //
    float segmentAngleSpacingInverse = 1.0f / segmentAngleSpacingRadians;
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

    for (int channel = 0; channel < energyPaintSpectrum.getNumChannels(); ++channel)
    {
        for (int ring = 0; ring < numRings; ++ring)
        {
            float upperAngle = -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::pi * channel;
            float lowerAngle = upperAngle - segmentAngleSpacingRadians;

            float energyLinear = energyPaintSpectrum.getBinMagnitude(channel, ring);
            energyLinear = juce::jmin(1.0f, energyLinear);
            int numSegments = (int)std::floor(juce::MathConstants<float>::halfPi * segmentAngleSpacingInverse * energyLinear);

            g.setColour(gradient.getColourAtPosition(energyLinear).withAlpha(1.0f));
            auto const segmentPath = segmentPaths[ring];
            for (int i = 0; i < numSegments; ++i)
            {
                {
                    auto transform = juce::AffineTransform::rotation(upperAngle).
                        scaled(xScale, yScale).
                        translated(center);
                    g.fillPath(*segmentPath, transform);
                }
                {
                    auto transform = juce::AffineTransform::rotation(lowerAngle).
                        scaled(xScale, yScale).
                        translated(center);
                    g.fillPath(*segmentPath, transform);
                }

                upperAngle += segmentAngleSpacingRadians;
                lowerAngle -= segmentAngleSpacingRadians;
            }
        }
    }
}
