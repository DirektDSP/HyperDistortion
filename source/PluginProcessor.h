#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "ParameterManager.h"

#if (MSVC)
#include "ipps.h"
#endif

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    // FIFO for audio visualization
    class FifoQueue
    {
    public:
        void push(const juce::AudioBuffer<float>& buffer);
        bool pull(juce::AudioBuffer<float>& buffer);

    private:
        static constexpr int bufferSize = 48000; // 1 second buffer at 48kHz
        juce::AbstractFifo fifo { bufferSize };
        juce::AudioBuffer<float> circularBuffer { 2, bufferSize };
    };

    FifoQueue fifoQueue;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        /*
        Parameters List

        Pre-LP & HP
            - Filter Type (Low Pass & High Pass for multiband processing)
            - Cutoff Freq

        Multiband Pre-Gain
            - Low Gain
            - Mid Gain
            - High Gain

        Haas Delay
            - Time
            - Mix

        Chorus
            - Rate
            - Depth
            - Centre Delay
            - Feedback
            - Mix

        Waveshaper
            - n/a

        Convolution
            - Impulse Response
            - Mix

        Compressor
            - Threshold
            - Ratio
            - Attack
            - Release
            - Mix

        */

        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"IN", 1}, "In Gain", -60.0f, 10.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"OUT", 1}, "Out Gain", -60.0f, 10.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"BYPASS", 1}, "Bypass", false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"BASS_MONO", 1}, "Bass Mono", false));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"BASS_MONO_FREQ", 1}, "Bass Mono Frequency", 10.0f, 300.0f, 0.0f));

        // Visualizer settings
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"VIS_SMOOTH", 1}, "Visualizer Smoothing Value", 0.0f, 1.0f, 0.69f));




        // Pre Filters
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"LOW_MID_FREQ", 1}, "Low Mid Crossover Frequency", 0.0f, 1000.0f, 1000.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"MID_HIGH_FREQ", 1}, "Mid High Crossover Frequency", 1000.0f, 15000.0f, 15000.0f));

        params.push_back (std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"LOW_SOLO", 1}, "Low band Solo", false));
        params.push_back (std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"MID_SOLO", 1}, "Mid band Solo", false));
        params.push_back (std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"HIGH_SOLO", 1}, "High band Solo", false));

        // Band Gains
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"LOW_GAIN", 1}, "Low Gain", -60.0f, 10.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"MID_GAIN", 1}, "Mid Gain", -60.0f, 10.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"HIGH_GAIN", 1}, "High Gain", -60.0f, 10.0f, 0.0f));

        // Haas Delay
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"HAAS_TIME", 1}, "Haas Time (ms)", 5.0f, 35.0f, 20.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"HAAS_MIX", 1}, "Haas Mix", 0.0f, 100.0f, 50.0f));

        // Chorus
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"CHORUS_RATE", 1}, "Chorus Rate", 0.1f, 5.0f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"CHORUS_DEPTH", 1}, "Chorus Depth", 0.0f, 1.0f, 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"CHORUS_CENTRE_DELAY", 1}, "Chorus Centre Delay", 1.0f, 30.0f, 10.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"CHORUS_FEEDBACK", 1}, "Chorus Feedback", -95.0f, 95.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"CHORUS_MIX", 1}, "Chorus Mix", 0.0f, 100.0f, 50.0f));

        // Convolution
        // Impulse Response?
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"CONV_MIX", 1}, "Convolution Mix", 0.0f, 100.0f, 50.0f));

        // Compressor

        // Compressor Parameters
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"COMPRESSOR_THRESHOLD", 1}, "Compressor Threshold", -60.0f, 0.0f, -24.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"COMPRESSOR_RATIO", 1}, "Compressor Ratio", 1.0f, 20.0f, 4.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"COMPRESSOR_ATTACK", 1}, "Compressor Attack", 1.0f, 100.0f, 10.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"COMPRESSOR_RELEASE", 1}, "Compressor Release", 10.0f, 500.0f, 100.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"COMPRESSOR_MIX", 1}, "Compressor Mix", 0.0f, 100.0f, 50.0f));

        return { params.begin(), params.end() };
    }

private:

    ParameterManager paramManager;

    juce::dsp::Compressor<float> compressor;

    juce::dsp::IIR::Filter<float> bassMonoFilter; // IIR has less latency (generally), may change to FIR because of linear phase...

    // Add additional filters for splitting
    juce::dsp::LinkwitzRileyFilter<float> lowMidCrossover;
    juce::dsp::LinkwitzRileyFilter<float> midHighCrossover;
    juce::dsp::Gain<float> lowGain, midGain, highGain;

    juce::dsp::DelayLine<float> haasDelay; // 5-35  ms
    juce::dsp::Chorus<float> chorus;
    // juce::dsp::WaveShaper<float> waveshaper;
    juce::dsp::Convolution convolution; // ir for cabinets or reverb?


    // visualiser

    juce::AudioBuffer<float> midBuffer;
    juce::AudioBuffer<float> sideBuffer;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
