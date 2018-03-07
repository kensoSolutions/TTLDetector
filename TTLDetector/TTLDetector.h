/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2018 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __TTLDETECTOR_H_3F920F95__
#define __TTLDETECTOR_H_3F920F95__

#include <ProcessorHeaders.h>
#include "TTLDetectorEditor.h"


class TTLDetectorEditor;

struct SimpleElectrode
{
    String name;

    int numChannels;
    int prePeakSamples, postPeakSamples;
    int lastBufferIndex;
    int electrodeID;
	double preSec;
	double postSec;
	int contador;
	bool isFound;
	int bufferCont;

    bool isMonitored;

    HeapBlock<int> channels;
    HeapBlock<double> thresholds;
    HeapBlock<bool> isActive;
	Array<float> signal;
};


/**
    Detects spikes in a continuous signal and outputs events containing the spike data.

    @see GenericProcessor, TTLDetectorEditor
*/
class TTLDetector : public GenericProcessor
{
public:
	TTLDetector();
	~TTLDetector();

    /** Processes an incoming continuous buffer and places new spikes into the event buffer. */
    void process (AudioSampleBuffer& buffer) override;

    /** Used to alter parameters of data acquisition. */
    void setParameter (int parameterIndex, float newValue) override;

    /** Called whenever the signal chain is altered. */
    void updateSettings() override;

	void createSpikeChannels() override;

    /** Called prior to start of acquisition. */
    bool enable() override;

    /** Called after acquisition is finished. */
    bool disable() override;

    /** Creates the TTLDetectorEditor. */
    AudioProcessorEditor* createEditor() override;

    void saveCustomParametersToXml (XmlElement* parentElement)  override;
    void loadCustomParametersFromXml()                          override;
	void handleEvent(const EventChannel* eventInfo, const MidiMessage& event, int);

    // INTERNAL BUFFERS
    // =====================================================================
    /** Extra samples are placed in this buffer to allow seamless
        transitions between callbacks. */
    AudioSampleBuffer overflowBuffer;
    // =====================================================================


    // CREATE AND DELETE ELECTRODES
    // =====================================================================
    /** Adds an electrode with n channels to be processed. */
	bool addElectrode(int nChans, double preSeconds, double postSeconds,int electrodeID = 0);

    /** Removes an electrode with a given index. */
    bool removeElectrode (int index);

	bool checkTimebase(double preSeconds, double postSeconds);
    // =====================================================================


    // EDIT AND QUERY ELECTRODE SETTINGS/
    // =====================================================================
    /** Returns the number of channels for a given electrode. */
    int getNumChannels (int index) const;

    /** Returns the continuous channel that maps to a given electrode channel. */
    int getChannel (int index, int chan) const;

    bool isChannelActive (int electrodeIndex, int channelNum);

    /** Edits the mapping between input channels and electrode channels. */
    void setChannel (int electrodeIndex, int channelNum, int newChannel);

    /** Sets the name of a given electrode. */
    void setElectrodeName (int index, String newName);

	/** Sets the timebase of all electrodes. */
	void setTimebase(double preSeconds,double postSeconds);

	void getAvgState(bool state);

    void setChannelActive (int electrodeIndex, int channelNum, bool active);
    // =====================================================================


    // RETURN STRING ARRAYS
    // =====================================================================
    /** Returns a StringArray containing the names of all electrodes */
    StringArray getElectrodeNames() const;

    // TODO: now it returns nothing. Need to double-check it.
    /** Returns array of electrodes. */
    void getElectrodes (Array<SimpleElectrode*>& electrodeArray);

    /** Returns array of electrodes. */
    SimpleElectrode* getActiveElectrode() const;

    /** Sets the current electrode index */
    SimpleElectrode* setCurrentElectrodeIndex (int);

    /** Returns a list of possible electrode types (e.g., stereotrode, tetrode). */
    StringArray electrodeTypes;

	/** Returns a list of possible time base range (e.g., 50, 100). */
	StringArray timeBaseRange;
    // =====================================================================

    void setChannelThreshold (int electrodeNum, int channelNum, float threshold);

    double getChannelThreshold (int electrodeNum, int channelNum) const;
	


private:

	std::atomic<int> triggerEvent;
	std::atomic<int> triggerChannel;

	enum Edges { RISING = 0, FALLING = 1 };
	enum Types { SET = 0, TOGGLE = 1 };

	Edges triggerEdge;
	Types triggerType;
	bool avgState;
    float getDefaultThreshold() const;
	int sampleRate;

    float getNextSample (int& chan);
    float getCurrentSample (int& chan);
    bool samplesAvailable (int nSamples);

      void addWaveformToSpikeObject (SpikeEvent::SpikeBuffer& s,
                                   int& peakIndex,
                                   int& electrodeNumber,
                                   int& currentChannel);

	  void addWaveformToArray(int& peakIndex,
		  int& electrodeNumber,
		  int& currentChannel,
		  int nSamples);

    void resetElectrode (SimpleElectrode*);

    /** Pointer to a continuous buffer. */
    AudioSampleBuffer* dataBuffer;

    int overflowBufferSize;
    int sampleIndex;
	int64 timestamp;
	Array<const SpikeChannel*> spikesChannels;
	int ttlPosition;
	
	
	double preSeconds;
	double postSeconds;

    Array<int> electrodeCounter;
	Array<int> electrodeFault;

    Array<bool> useOverflowBuffer;

    int currentElectrode;
    int currentChannelIndex;

    OwnedArray<SimpleElectrode> electrodes;
    int uniqueID;

    // void createSpikeEvent(int& peakIndex,
    // 					  int& electrodeNumber,
    // 					  int& currentChannel,
    // 					  MidiBuffer& eventBuffer);

    uint16_t sampleRateForElectrode;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TTLDetector);
};



#endif  // __TTLDetector_H_3F920F95__
