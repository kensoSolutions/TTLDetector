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

#include <stdio.h>
#include "TTLDetector.h"


TTLDetector::TTLDetector()
    : GenericProcessor      ("TTL Detector")
    , overflowBuffer        (2, 10028)
    , dataBuffer            (nullptr),
      overflowBufferSize    (10028)
    , currentElectrode      (-1)
    , uniqueID              (0)
{
    setProcessorType (PROCESSOR_TYPE_FILTER);

    //// the standard form:
    electrodeTypes.add ("single electrode");
	timeBaseRange.add("20");
	timeBaseRange.add("50");
	timeBaseRange.add("100");
	timeBaseRange.add("200");
	timeBaseRange.add("500");
	timeBaseRange.add("750");


    //electrodeTypes.add ("stereotrode");
    //electrodeTypes.add ("tetrode");

    //// the technically correct form (Greek cardinal prefixes):
    // electrodeTypes.add("hentrode");
    // electrodeTypes.add("duotrode");
    // electrodeTypes.add("triode");
    // electrodeTypes.add("tetrode");
    // electrodeTypes.add("pentrode");
    // electrodeTypes.add("hextrode");
    // electrodeTypes.add("heptrode");
    // electrodeTypes.add("octrode");
    // electrodeTypes.add("enneatrode");
    // electrodeTypes.add("decatrode");
    // electrodeTypes.add("hendecatrode");
    // electrodeTypes.add("dodecatrode");
    // electrodeTypes.add("triskaidecatrode");
    // electrodeTypes.add("tetrakaidecatrode");
    // electrodeTypes.add("pentakaidecatrode");
    // electrodeTypes.add("hexadecatrode");
    // electrodeTypes.add("heptakaidecatrode");
    // electrodeTypes.add("octakaidecatrode");
    // electrodeTypes.add("enneakaidecatrode");
    // electrodeTypes.add("icosatrode");

    for (int i = 0; i < electrodeTypes.size() + 1; ++i)
    {
        electrodeCounter.add (0);
    }
	avgState = false;
	sampleRate = getSampleRate()/1000;
}


TTLDetector::~TTLDetector()
{
}


AudioProcessorEditor* TTLDetector::createEditor()
{
    editor = new TTLDetectorEditor (this, true);
    return editor;
}


void TTLDetector::createSpikeChannels()
{
	for (int i = 0; i < electrodes.size(); ++i)
	{
		SimpleElectrode* elec = electrodes[i];
		unsigned int nChans = elec->numChannels;
		Array<const DataChannel*> chans;
		for (int c = 0; c < nChans; c++)
		{
			chans.add(getDataChannel(elec->channels[c]));
		}
		SpikeChannel* spk = new SpikeChannel(SpikeChannel::typeFromNumChannels(nChans), this, chans);
		spk->setNumSamples(elec->prePeakSamples, elec->postPeakSamples);
		spikeChannelArray.add(spk);
		//SpikeEvent::SpikeBuffer spikeData(spikeChannelArray[i]);
	}
}



void TTLDetector::updateSettings()
{
	if (getNumInputs() > 0)
	{
		overflowBuffer.setSize(getNumInputs(), overflowBufferSize);
		//overflowBuffer.clear();
		sampleRate = getDataChannel(0)->getSampleRate() / 1000;
	}
		
}


bool TTLDetector::addElectrode(int nChans, double preSeconds, double postSeconds, int electrodeID)
{
    std::cout << "Adding electrode with " << nChans << " channels." << std::endl;

    int firstChan;
    if (electrodes.size() == 0)
    {
        firstChan = 0;
    }
    else
    {
        SimpleElectrode* e = electrodes.getLast();
        firstChan = *(e->channels + (e->numChannels - 1)) + 1;
    }

    if (firstChan + nChans > getNumInputs())
    {
        firstChan = 0; // make sure we don't overflow available channels
    }

	
	sampleRate = getEventChannel(0)->getSampleRate() /1000;

	//std::cout << preSeconds << std::endl;
	//std::cout << postSeconds << std::endl;
	//std::cout << sampleRate << std::endl;
    String electrodeName;

    // hard-coded for tetrode configuration
    if (nChans < 3)
        electrodeName = electrodeTypes[nChans - 1];
    else
        electrodeName = electrodeTypes[nChans - 2];

    String newName = electrodeName.substring (0,1);
    newName = newName.toUpperCase();
    electrodeName = electrodeName.substring (1, electrodeName.length());
    newName += electrodeName;
    newName += " ";
	if (electrodeFault.size() == 0)
	{
		int currentVal = electrodeCounter[nChans];
		electrodeCounter.set(nChans, ++currentVal);
		newName += electrodeCounter[nChans];
	}
	else
	{
		newName += electrodeFault[0];
		electrodeFault.remove(0);
	}

    SimpleElectrode* newElectrode = new SimpleElectrode;

    newElectrode->name = newName;
    newElectrode->numChannels = nChans;
	newElectrode->preSec = preSeconds;
	newElectrode->postSec = postSeconds;
	if (sampleRate*(newElectrode->preSec + newElectrode->postSec)>16376)
	{
		String responseString = "There are too many ms to capture the signal.\n";
		responseString += "You can capture the signal after TTL if:  \n";
		responseString += "    File sample rate * seconds you want to capture are higher than 16376.";
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Error at adding electrode", responseString);
		return false;
	}
	newElectrode->prePeakSamples = newElectrode->preSec*sampleRate;
	newElectrode->postPeakSamples = newElectrode->postSec*sampleRate;
    newElectrode->thresholds.malloc (nChans);
    newElectrode->isActive.malloc (nChans);
    newElectrode->channels.malloc (nChans);
    newElectrode->isMonitored = false;

    for (int i = 0; i < nChans; ++i)
    {
        *(newElectrode->channels + i) = firstChan+i;
        *(newElectrode->thresholds + i) = getDefaultThreshold();
        *(newElectrode->isActive + i) = true;
    }

    if (electrodeID > 0) 
    {
        newElectrode->electrodeID = electrodeID;
        uniqueID = std::max (uniqueID, electrodeID);
    }
    else
    {
        newElectrode->electrodeID = ++uniqueID;
    }

	newElectrode->bufferCont = 0;
    resetElectrode (newElectrode);

    electrodes.add (newElectrode);

    currentElectrode = electrodes.size() - 1;

    return true;
}


float TTLDetector::getDefaultThreshold() const
{
    return 0.0f;
}


StringArray TTLDetector::getElectrodeNames() const
{
    StringArray names;

    for (int i = 0; i < electrodes.size(); ++i)
    {
        names.add (electrodes[i]->name);
    }

    return names;
}


void TTLDetector::resetElectrode (SimpleElectrode* e)
{
    e->lastBufferIndex = 0;
	e->contador = 0;
	e->isFound = false;
	e->signal.clear();

}

void TTLDetector::getAvgState(bool state)
{
	
	for (int i = 0; i < electrodes.size(); ++i)
	{
		resetElectrode(electrodes[i]);
		electrodes[i]->bufferCont = 0;
	}
	avgState = state;
}


bool TTLDetector::removeElectrode (int index)
{
    // std::cout << "Spike detector removing electrode" << std::endl;

    if (index > electrodes.size() || index < 0)
        return false;
	String name = electrodes[index]->name;
	name = name.substring(name.length() - 2, name.length());
	int number = (int)name.getFloatValue();
    electrodes.remove (index);
	electrodeFault.addUsingDefaultSort(number);
    return true;
}

bool TTLDetector::checkTimebase(double preSeconds, double postSeconds)
{
	// std::cout << "Spike detector removing electrode" << std::endl;

	//int sampleRate = getEventChannel(0)->getSampleRate() / 1000;
	if (sampleRate*(preSeconds + postSeconds)>16376)
	{
		String responseString = "There are too many ms to capture the signal.\n";
		responseString += "You can capture the signal after TTL if:  \n";
		responseString += "    File sample rate * seconds you want to capture are higher than 16376.";
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Error at adding electrode", responseString);
		return false;
	}
	else
		return true;
}


void TTLDetector::setElectrodeName (int index, String newName)
{
    electrodes[index - 1]->name = newName;
}

void TTLDetector::setTimebase(double preSeconds, double postSeconds)
{
	for (int i = 0; i < electrodes.size(); ++i)
	{
		int sampleRate = getEventChannel(0)->getSampleRate() / 1000;
		electrodes[i]->preSec = preSeconds;
		electrodes[i]->postSec = postSeconds;
		electrodes[i]->prePeakSamples = sampleRate*preSeconds;
		electrodes[i]->postPeakSamples = sampleRate*postSeconds;
		resetElectrode(electrodes[i]);
		electrodes[i]->bufferCont = 0;
		
	}
	
}


void TTLDetector::setChannel (int electrodeIndex, int channelNum, int newChannel)
{
    std::cout << "Setting electrode " << electrodeIndex << " channel " << channelNum
                << " to " << newChannel << std::endl;

    *(electrodes[electrodeIndex]->channels + channelNum) = newChannel;
}


int TTLDetector::getNumChannels (int index) const
{
    if (index < electrodes.size())
        return electrodes[index]->numChannels;
    else
        return 0;
}


int TTLDetector::getChannel (int index, int i) const
{
    return *(electrodes[index]->channels + i);
}


void TTLDetector::getElectrodes (Array<SimpleElectrode*>& electrodeArray)
{
    electrodeArray.addArray (electrodes);
}


SimpleElectrode* TTLDetector::setCurrentElectrodeIndex (int i)
{
    jassert (i >= 0 & i < electrodes.size());
    currentElectrode = i;

    return electrodes[i];
}


SimpleElectrode* TTLDetector::getActiveElectrode() const
{
    if (electrodes.size() == 0)
        return nullptr;

    return electrodes[currentElectrode];
}


void TTLDetector::setChannelActive (int electrodeIndex, int subChannel, bool active)
{
    currentElectrode = electrodeIndex;
    currentChannelIndex = subChannel;

    std::cout << "Setting channel active to " << active << std::endl;

    if (active)
        setParameter (98, 1);
    else
        setParameter (98, 0);
}


bool TTLDetector::isChannelActive (int electrodeIndex, int i)
{
    return *(electrodes[electrodeIndex]->isActive + i);
}


void TTLDetector::setChannelThreshold (int electrodeNum, int channelNum, float thresh)
{
    currentElectrode = electrodeNum;
    currentChannelIndex = channelNum;

    std::cout << "Setting electrode " << electrodeNum << " channel threshold " << channelNum << " to " << thresh << std::endl;

    setParameter (99, thresh);
}


double TTLDetector::getChannelThreshold(int electrodeNum, int channelNum) const
{
    return *(electrodes[electrodeNum]->thresholds + channelNum);
}


void TTLDetector::setParameter (int parameterIndex, float newValue)
{
    //editor->updateParameterButtons(parameterIndex);

    if (parameterIndex == 99 && currentElectrode > -1)
    {
        *(electrodes[currentElectrode]->thresholds + currentChannelIndex) = newValue;
    }
    else if (parameterIndex == 98 && currentElectrode > -1)
    {
        if (newValue == 0.0f)
            *(electrodes[currentElectrode]->isActive + currentChannelIndex) = false;
        else
            *(electrodes[currentElectrode]->isActive + currentChannelIndex) = true;
    }
	if (parameterIndex == 0)
	{
		triggerEvent = static_cast<int>(newValue);
	}
	else if (parameterIndex == 1)
	{
		triggerChannel = static_cast<int>(newValue);
	}
	else if (parameterIndex == 2)
	{
		triggerType = (Types)((int)newValue - 1);
	}
	else if (parameterIndex == 3)
	{
		triggerEdge = (Edges)((int)newValue - 1);
	}

}


bool TTLDetector::enable()
{
    sampleRateForElectrode = (uint16_t) getSampleRate();

    useOverflowBuffer.clear();

    for (int i = 0; i < electrodes.size(); ++i)
        useOverflowBuffer.add (false);

    return true;
}


bool TTLDetector::disable()
{
    for (int n = 0; n < electrodes.size(); ++n)
    {
        resetElectrode (electrodes[n]);
    }

    return true;
}

void TTLDetector::addWaveformToSpikeObject(SpikeEvent::SpikeBuffer& s,
	int& peakIndex,
	int& electrodeNumber,
	int& currentChannel)
{
	int spikeLength = electrodes[electrodeNumber]->prePeakSamples
		+ electrodes[electrodeNumber]->postPeakSamples;

	const int chan = *(electrodes[electrodeNumber]->channels + currentChannel);
	if (isChannelActive(electrodeNumber, currentChannel))
	{
		//std::cout << "ONDA: " << std::endl;
		for (int sample = 0; sample < spikeLength; ++sample)
		{
			float aaa = electrodes[electrodeNumber]->signal[sample];// getNextSample(*(electrodes[electrodeNumber]->channels + currentChannel));
			//if (sample == 0 || sample == spikeLength - 1 || sample == spikeLength/2)
			//	std::cout << "Value: " << aaa << std::endl;
			//std::cout << currentChannel << std::endl;
			s.set(currentChannel, sample, aaa);
			//std::cout << currentIndex << std::endl;
		}
	}
	else
	{
		for (int sample = 0; sample < spikeLength; ++sample)
		{
			// insert a blank spike if the
			s.set(currentChannel, sample, 0);
			//std::cout << currentIndex << std::endl;
		}
	}
}

void TTLDetector::addWaveformToArray(int& peakIndex,
	int& electrodeNumber,
	int& currentChannel,
	int nSamples)
{
	int spikeLength = electrodes[electrodeNumber]->prePeakSamples
		+ electrodes[electrodeNumber]->postPeakSamples;

	int posIni = electrodes[electrodeNumber]->contador;
	int posFin = 0;
	if (posIni == 0)
		posFin = std::min(posIni + nSamples + electrodes[electrodeNumber]->prePeakSamples, spikeLength);
	else
		posFin = std::min(posIni + nSamples, spikeLength);
	int samplesLeft = std::min(spikeLength - posIni, nSamples);
	//std::cout << "**************sampleLeft: " << samplesLeft << std::endl;
	//std::cout << "**************posFin: " << posFin << std::endl;
	//std::cout << "**************contador: " << posIni << std::endl;
	float peso1 = (float)electrodes[electrodeNumber]->bufferCont / (float)(electrodes[electrodeNumber]->bufferCont + 1);
	float peso2 = 1.0f / (float)(electrodes[electrodeNumber]->bufferCont + 1);
	//std::cout << "**************peso1: " << peso1 << std::endl;
	//std::cout << "**************peso2: " << peso2 << std::endl;

	const int chan = *(electrodes[electrodeNumber]->channels + currentChannel);

	if (isChannelActive(electrodeNumber, currentChannel))
	{
		//std::cout << "ONDA: " << std::endl;
		for (int sample = posIni; sample < posFin; ++sample)
		{
			float aaa = getNextSample(*(electrodes[electrodeNumber]->channels + currentChannel));
			//if (sample == 0 || sample == spikeLength - 1 || sample == spikeLength/2)
			if (avgState)
			{
				//if (sample == 0 || sample == spikeLength - 1 || sample == spikeLength/2)
				aaa = electrodes[electrodeNumber]->signal[sample] * peso1 + aaa*peso2;
			}
			//	std::cout << "Value: " << aaa << std::endl;
			electrodes[electrodeNumber]->signal.set(sample, aaa);
			++sampleIndex;
			++electrodes[electrodeNumber]->contador;
			//std::cout << currentIndex << std::endl;
		}
	}
	else
	{
		for (int sample = posIni; sample < posFin + 1; ++sample)
		{
			float aaa = 0;
			if (avgState)
			{
				aaa = electrodes[electrodeNumber]->signal[sample] * peso1 + aaa*peso2;
			}
			// insert a blank spike if the
			electrodes[electrodeNumber]->signal.set(sample, aaa);
			++sampleIndex;
			++electrodes[electrodeNumber]->contador;
			//std::cout << currentIndex << std::endl;
		}
	}
	sampleIndex -= samplesLeft; // reset sample index
}




void TTLDetector::handleEvent(const EventChannel* eventInfo, const MidiMessage& event, int samplePosition)
{

	
	if (triggerEvent < 0) return;
	if (eventInfo->getChannelType() == EventChannel::TTL && eventInfo == eventChannelArray[triggerEvent])
	{
		
		TTLEventPtr ttl = TTLEvent::deserializeFromMessage(event, eventInfo);
		//std::cout << "HandleEvent3 - triggerChan_> " << triggerChannel<< std::endl;
		
		if (ttl->getChannel() == triggerChannel)
		{
			int eventId = ttl->getState() ? 1 : 0;
			
		
			//int edge = triggerEdge == RISING ? 1 : 0;
			//std::cout << "HandleEvent4 - triggerType " << triggerType << " edge: " << edge << " eventId:" << eventId << std::endl;

			//************************ MICODIGO ******************************//
		
			for (int i = 0; i < electrodes.size(); ++i)
			{
				//  std::cout << "ELECTRODE " << i << std::endl;
				if (eventId)
				{
					//std::cout << "***************Event position: " << samplePosition << std::endl;
					electrodes[i]->isFound = true;
					ttlPosition = samplePosition;
				}
				

			}
		}
	}
}



void TTLDetector::process (AudioSampleBuffer& buffer)
{
	SimpleElectrode* electrode;
	dataBuffer = &buffer;
	//std::cout << "Buffer x " << std::endl;
	checkForEvents();
	// cycle through electrodes
	

	for (int i = 0; i < electrodes.size(); ++i)
	{
		//  std::cout << "ELECTRODE " << i << std::endl;
			electrode = electrodes[i];
			//std::cout << "is Found = " << electrodes[i]->isFound << std::endl;
			spikesChannels.add(getSpikeChannel(i));
			//SpikeEvent::SpikeBuffer spikeData(spikesChannels[i]);


			// refresh buffer index for this electrode
			sampleIndex = electrode->lastBufferIndex - 1;
			// subtract 1 to account for
			// increment at start of getNextSample()

			const int nSamples = getNumSamples(*electrode->channels);
			//std::cout << "///////////////nSamples:  " << nSamples << std::endl;
			// cycle through samples
			while (samplesAvailable(nSamples))
			{
				++sampleIndex;
				

					//std::cout << "?????????samplesLeftNow: " << samplesLeftNow << std::endl;
					// cycle through channels
					for (int chan = 0; chan < electrode->numChannels; ++chan)
					{
						// std::cout << "  channel " << chan << std::endl;
						if (*(electrode->isActive + chan))
						{
							int currentChannel = *(electrode->channels + chan);
							if (electrodes[i]->isFound)
							{
								//std::cout << "Found2" << std::endl;
								int peakIndex = ttlPosition;
								int samplesLeft = electrode->prePeakSamples + electrode->postPeakSamples - electrodes[i]->contador;
								int samplesLeftNow = std::min(samplesLeft, nSamples - sampleIndex);
								if (electrodes[i]->contador == 0)
								{
									sampleIndex = ttlPosition;
									//std::cout << sampleIndex << std::endl;
									samplesLeftNow = std::min(samplesLeft, nSamples - sampleIndex);
									sampleIndex -= (electrode->prePeakSamples + 1);
									//std::cout << sampleIndex << std::endl;
								}
							
								const SpikeChannel* spikeChan = getSpikeChannel(i);
								SpikeEvent::SpikeBuffer spikeData(spikeChan);

								Array<float> thresholds;

								for (int channel = 0; channel < electrode->numChannels; ++channel)
								{
									addWaveformToArray(peakIndex,
										i,
										channel,
										samplesLeftNow);
								}

								if (electrodes[i]->contador >= electrode->prePeakSamples + electrode->postPeakSamples)
								{
									for (int channel = 0; channel < electrode->numChannels; ++channel)
									{
										addWaveformToSpikeObject(spikeData,
											peakIndex,
											i,
											channel);
										thresholds.add((int)*(electrode->thresholds + channel));
									}
									timestamp = getTimestamp(electrode->channels[0]) + peakIndex;
									SpikeEventPtr newSpike = SpikeEvent::createSpikeEvent(spikeChan, timestamp, thresholds, spikeData, 0);
									// package spikes;
									addSpike(spikeChan, newSpike, peakIndex);
									electrodes[i]->contador = 0;
									electrodes[i]->isFound = false;
									electrodes[i]->bufferCont++;
									
									//std::cout << "Spike realizado " << std::endl;
								}

								// advance the sample index
								sampleIndex = sampleIndex + samplesLeftNow + 1;// electrode->postPeakSamples;

								// quit spike "for" loop
								break;

							

							// end if channel is active
							}

						// end cycle through channels on electrode
						}
					}

				// end cycle through samples
				}

			electrode->lastBufferIndex = sampleIndex - nSamples; // should be negative
			for (int j = 0; j < electrode->numChannels; ++j)
			{
				AudioSampleBuffer auxBuffer(overflowBuffer);
				overflowBuffer.copyFrom(*electrode->channels + j,
					0,
					auxBuffer,
					*electrode->channels + j,
					nSamples,
					overflowBufferSize-nSamples);

				//overflowBuffer.clear(overflowBufferSize - nSamples, nSamples);

				overflowBuffer.copyFrom(*electrode->channels + j,
					overflowBufferSize-nSamples,
					buffer,
					*electrode->channels + j,
					0,
					nSamples);

					//overflowBufferSize += nSamples;
					/*float bloque1 = overflowBuffer.getSample(*electrode->channels + j, 4425);
					std::cout << "********bloque1: " << bloque1 << std::endl;
					float bloque2 = overflowBuffer.getSample(*electrode->channels + j, 6425);
					std::cout << "********bloque2: " << bloque2 << std::endl;
					float bloque3 = overflowBuffer.getSample(*electrode->channels + j, 8351);
					std::cout << "********bloque3: " << bloque3 << std::endl;
					std::cout << " //////////////////// " << std::endl;*/

			}
			useOverflowBuffer.set(i, true);
			

			// end cycle through electrodes
		}
			
}


float TTLDetector::getNextSample (int& chan)
{
	
	if (sampleIndex < 0)
	{
		const int ind = overflowBufferSize + sampleIndex;
		if (ind < overflowBuffer.getNumSamples())
			return *overflowBuffer.getWritePointer(chan, ind);
		
		else
			return 0;
		
    }
    else
    {
		if (sampleIndex < dataBuffer->getNumSamples())
			return *dataBuffer->getWritePointer(chan, sampleIndex);
        else
            return 0;
		
    }
}


float TTLDetector::getCurrentSample (int& chan)
{
    if (sampleIndex < 1)
    {
        return *overflowBuffer.getWritePointer (chan, overflowBufferSize + sampleIndex - 1);
    }
    else
    {
        return *dataBuffer->getWritePointer (chan, sampleIndex - 1);
    }
}


bool TTLDetector::samplesAvailable (int nSamples)
{
    if (sampleIndex > nSamples)
    {
        return false;
    }
    else
    {
        return true;
    }
}


void TTLDetector::saveCustomParametersToXml (XmlElement* parentElement)
{
    for (int i = 0; i < electrodes.size(); ++i)
    {
        XmlElement* electrodeNode = parentElement->createNewChildElement ("ELECTRODE");
        electrodeNode->setAttribute ("name",             electrodes[i]->name);
        electrodeNode->setAttribute ("numChannels",      electrodes[i]->numChannels);
        electrodeNode->setAttribute ("prePeakSamples",   electrodes[i]->prePeakSamples);
        electrodeNode->setAttribute ("postPeakSamples",  electrodes[i]->postPeakSamples);
        electrodeNode->setAttribute ("electrodeID",      electrodes[i]->electrodeID);
		electrodeNode->setAttribute ("preSeconds",       electrodes[i]->preSec);
		electrodeNode->setAttribute ("postSeconds",      electrodes[i]->postSec);

        for (int j = 0; j < electrodes[i]->numChannels; ++j)
        {
            XmlElement* channelNode = electrodeNode->createNewChildElement ("SUBCHANNEL");
            channelNode->setAttribute ("ch",        *(electrodes[i]->channels + j));
            channelNode->setAttribute ("thresh",    *(electrodes[i]->thresholds + j));
            channelNode->setAttribute ("isActive",  *(electrodes[i]->isActive + j));
        }
    }
}


void TTLDetector::loadCustomParametersFromXml()
{
    if (parametersAsXml != nullptr) // prevent double-loading
    {
        // use parametersAsXml to restore state

        TTLDetectorEditor* sde = (TTLDetectorEditor*) getEditor();

        int electrodeIndex = -1;

        forEachXmlChildElement (*parametersAsXml, xmlNode)
        {
            if (xmlNode->hasTagName ("ELECTRODE"))
            {
                ++electrodeIndex;

                std::cout << "ELECTRODE>>>" << std::endl;

                const int channelsPerElectrode = xmlNode->getIntAttribute ("numChannels");
                const int electrodeID          = xmlNode->getIntAttribute ("electrodeID");
				const double preSeconds           = xmlNode->getIntAttribute("preSeconds");
				const double postSeconds           = xmlNode->getIntAttribute("postSeconds");

                sde->addElectrode (channelsPerElectrode, preSeconds, postSeconds, electrodeID);

                setElectrodeName (electrodeIndex + 1, xmlNode->getStringAttribute ("name"));
                sde->refreshElectrodeList();
				

                int channelIndex = -1;

                forEachXmlChildElement (*xmlNode, channelNode)
                {
                    if (channelNode->hasTagName ("SUBCHANNEL"))
                    {
                        ++channelIndex;

                        std::cout << "Subchannel " << channelIndex << std::endl;

                        setChannel          (electrodeIndex, channelIndex, channelNode->getIntAttribute ("ch"));
                        setChannelThreshold (electrodeIndex, channelIndex, channelNode->getDoubleAttribute ("thresh"));
                        setChannelActive    (electrodeIndex, channelIndex, channelNode->getBoolAttribute ("isActive"));
                    }
                }
            }
        }
        sde->checkSettings();
    }
}

