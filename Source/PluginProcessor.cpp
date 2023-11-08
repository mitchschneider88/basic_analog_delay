/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalogDelayAudioProcessor::AnalogDelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), treeState(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    treeState.addParameterListener("delayTimeID", this);
    treeState.addParameterListener("delayFeedbackID", this);
    treeState.addParameterListener("wetLevelID", this);
    treeState.addParameterListener("dryLevelID", this);
}

AnalogDelayAudioProcessor::~AnalogDelayAudioProcessor()
{
    treeState.removeParameterListener("delayTimeID", this);
    treeState.removeParameterListener("delayFeedbackID", this);
    treeState.removeParameterListener("wetLevelID", this);
    treeState.removeParameterListener("dryLevelID", this);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AnalogDelayAudioProcessor::createParameterLayout()
{
    std::vector <std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.reserve(4);
    
    juce::NormalisableRange<float> delayTimeRange (0.f, 2000.f, 1.f, 1.f, false);
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayTimeID", "time", delayTimeRange, 250.f));
    
    juce::NormalisableRange<float> delayFeedbackRange (0.f, 100.f, 1.f, 1.f, false);
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayFeedbackID", "feedback", delayFeedbackRange, 30.f));
    
    juce::NormalisableRange<float> wetLevelRange (-60.f, 12.f, 1.f, 1.f, false);
    params.push_back(std::make_unique<juce::AudioParameterFloat>("wetLevelID", "wetLevel", wetLevelRange, -3.f));
    
    juce::NormalisableRange<float> dryLevelRange (-60.f, 12.f, 1.f, 1.f, false);
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dryLevelID", "dryLevel", dryLevelRange, -3.f));
        
    return { params.begin(), params.end() };
}

void AnalogDelayAudioProcessor::parameterChanged(const juce::String &parameterID, float newValue)
{
    if (parameterID == "delayTimeID")
    {
        params.leftDelay_mSec = newValue;
        params.rightDelay_mSec = newValue;
        analogDelay.setParameters(params);
    }
    
    if (parameterID == "delayFeedbackID")
    {
        params.feedback_Pct = newValue;
        analogDelay.setParameters(params);
    }
    
    if (parameterID == "wetLevelID")
    {
        params.wetLevel_dB = newValue;
        analogDelay.setParameters(params);
    }
    
    if (parameterID == "dryLevelID")
    {
        params.dryLevel_dB = newValue;
        analogDelay.setParameters(params);
    }
}


const juce::String AnalogDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AnalogDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AnalogDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AnalogDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AnalogDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AnalogDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AnalogDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AnalogDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AnalogDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void AnalogDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AnalogDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    analogDelay.reset(sampleRate);
    
    analogDelay.createDelayBuffers(sampleRate, 2000);

    params.leftDelay_mSec = treeState.getRawParameterValue("delayTimeID")->load();
    params.rightDelay_mSec = treeState.getRawParameterValue("delayTimeID")->load();
    
    params.feedback_Pct = treeState.getRawParameterValue("delayFeedbackID")->load();
    
    params.dryLevel_dB = treeState.getRawParameterValue("dryLevelID")->load();
    params.wetLevel_dB = treeState.getRawParameterValue("wetLevelID")->load();
    
    analogDelay.setParameters(params);
}

void AnalogDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AnalogDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AnalogDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    for (int sample = 0; sample < numSamples; ++sample)
    {
        auto* xnL = buffer.getReadPointer(0);
        auto* ynL = buffer.getWritePointer(0);
        
        auto* xnR = buffer.getReadPointer(1);
        auto* ynR = buffer.getWritePointer(1);
        
        float inputs[2] = {xnL[sample], xnR[sample]};
        float outputs[2] = {0.0, 0.0};
        analogDelay.processAudioFrame(inputs, outputs, 2, 2);
        
        ynL[sample] = outputs[0];
        ynR[sample] = outputs[1];
    }
}

//==============================================================================
bool AnalogDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AnalogDelayAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void AnalogDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AnalogDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnalogDelayAudioProcessor();
}
