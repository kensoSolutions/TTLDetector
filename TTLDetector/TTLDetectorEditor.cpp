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

#include "TTLDetectorEditor.h"
#include "TTLDetector.h"
#include <stdio.h>



TTLDetectorEditor::TTLDetectorEditor(GenericProcessor* parentNode, bool useDefaultParameterEditors=true)
    : GenericEditor(parentNode, useDefaultParameterEditors), isPlural(true)

{
	int silksize;
	const char* silk = CoreServices::getApplicationResource("silkscreenserialized", silksize);
    MemoryInputStream mis(silk, silksize, false);
    Typeface::Ptr typeface = new CustomTypeface(mis);
    font = Font(typeface);

    desiredWidth = 320;

    electrodeTypes = new ComboBox("Electrode Types");

    TTLDetector* processor = (TTLDetector*) getProcessor();

    for (int i = 0; i < processor->electrodeTypes.size(); i++)
    {
        String type = processor->electrodeTypes[i];
        electrodeTypes->addItem(type += "s", i+1);
    }

    electrodeTypes->setEditableText(false);
    electrodeTypes->setJustificationType(Justification::centredLeft);
    electrodeTypes->addListener(this);
    electrodeTypes->setBounds(65,45,110,20);
    electrodeTypes->setSelectedId(1);
    addAndMakeVisible(electrodeTypes);

    electrodeList = new ComboBox("Electrode List");
    electrodeList->setEditableText(false);
    electrodeList->setJustificationType(Justification::centredLeft);
    electrodeList->addListener(this);
    electrodeList->setBounds(30,75,110,20);
    addAndMakeVisible(electrodeList);

    numElectrodes = new Label("Number of Electrodes","1");
    numElectrodes->setEditable(true);
    numElectrodes->addListener(this);
    numElectrodes->setBounds(30,45,25,20);
    //labelTextChanged(numElectrodes);
    addAndMakeVisible(numElectrodes);

    upButton = new TriangleButton(1);
    upButton->addListener(this);
    upButton->setBounds(50,45,10,8);
    addAndMakeVisible(upButton);

    downButton = new TriangleButton(2);
    downButton->addListener(this);
    downButton->setBounds(50,55,10,8);
    addAndMakeVisible(downButton);

    plusButton = new UtilityButton("+", titleFont);
    plusButton->addListener(this);
    plusButton->setRadius(3.0f);
    plusButton->setBounds(15,47,14,14);
    addAndMakeVisible(plusButton);

	loadButton = new LoadButton();
	loadButton->addListener(this);
	loadButton->setBounds(295, 5, 15, 15);
	addAndMakeVisible(loadButton);

	saveButton = new SaveButton();
	saveButton->addListener(this);
	saveButton->setBounds(275, 5, 15, 15);
	addAndMakeVisible(saveButton);

    ElectrodeEditorButton* e1 = new ElectrodeEditorButton("EDIT",font);
    e1->addListener(this);
    addAndMakeVisible(e1);
    e1->setBounds(30,100,40,10);
    electrodeEditorButtons.add(e1);

    ElectrodeEditorButton* e2 = new ElectrodeEditorButton("MONITOR",font);
    e2->addListener(this);
    addAndMakeVisible(e2);
    e2->setBounds(100,100,70,10);
    electrodeEditorButtons.add(e2);

    ElectrodeEditorButton* e3 = new ElectrodeEditorButton("DELETE",font);
    e3->addListener(this);
    addAndMakeVisible(e3);
    e3->setBounds(100,115,70,10);
    electrodeEditorButtons.add(e3);

	timeBaseLabel = new Label("Name", "Timebase (ms):");
	font.setHeight(10);
	timeBaseLabel->setFont(font);
	timeBaseLabel->setBounds(202, 30, 95, 15);
	timeBaseLabel->setColour(Label::textColourId, Colours::grey);
	addAndMakeVisible(timeBaseLabel);

	timeBaseList = new ComboBox("Time Base List");
	for (int i = 0; i < processor->timeBaseRange.size(); i++)
	{
		String num = processor->timeBaseRange[i];
		timeBaseList->addItem(num, i + 1);
	}
	timeBaseList->setEditableText(false);
	timeBaseList->setJustificationType(Justification::centredLeft);
	timeBaseList->addListener(this);
	timeBaseList->setBounds(207, 45, 80, 20);
	timeBaseList->setSelectedId(3);
	addAndMakeVisible(timeBaseList);
	lastIdTimebase = 3;

	timeBaseLabel = new Label("Name", "TTL time (ms):");
	font.setHeight(10);
	timeBaseLabel->setFont(font);
	timeBaseLabel->setBounds(202, 70, 95, 15);
	timeBaseLabel->setColour(Label::textColourId, Colours::grey);
	addAndMakeVisible(timeBaseLabel);

	timeBaseSlider = new Slider(Slider::LinearHorizontal, Slider::TextBoxBelow);
	timeBaseSlider->setBounds(200, 80, 100, 40);
	timeBaseSlider->setName("TTL time (ms)");
	timeBaseSlider->setColour(Slider::textBoxBackgroundColourId, Colours::lightgrey);
	addAndMakeVisible(timeBaseSlider);
	timeBaseSlider->addListener(this);
	int ID = timeBaseList->getSelectedId();
	String value = timeBaseList->getItemText(ID - 1);
	timeBaseSlider->setRange(0, value.getFloatValue(), 1);
	timeBaseSlider->setTextBoxIsEditable(true);

	averageButton = new UtilityButton("Avg", Font("Small Text", 13, Font::plain));
	averageButton->setRadius(3.0f);
	averageButton->addListener(this);
	averageButton->setClickingTogglesState(true);
	averageButton->setBounds(30, 115, 45, 14);
	averageButton->setToggleState(false, sendNotification);
	addAndMakeVisible(averageButton);
		
	preSeconds = (double)timeBaseSlider->getValue() / 1.0f;
	postSeconds = (double)(timeBaseSlider->getMaximum() - timeBaseSlider->getValue()) / 1.0f;
   /* thresholdLabel = new Label("Name","Threshold");
    font.setHeight(10);
    thresholdLabel->setFont(font);
    thresholdLabel->setBounds(202, 105, 95, 15);
    thresholdLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(thresholdLabel);*/

    // create a custom channel selector
    //deleteAndZero(channelSelector);

    // channelSelector = new ChannelSelector(false, font);
    //  addChildComponent(channelSelector);
    // channelSelector->setVisible(false);
    //
    //  Array<int> a;

    channelSelector->inactivateButtons();
    channelSelector->paramButtonsToggledByDefault(false);
    //  channelSelector->paramButtonsActiveByDefault(false);

}

TTLDetectorEditor::~TTLDetectorEditor()
{

    for (int i = 0; i < electrodeButtons.size(); i++)
    {
        removeChildComponent(electrodeButtons[i]);
    }

    deleteAllChildren();

}

void TTLDetectorEditor::sliderEvent(Slider* slider)
{
	TTLDetector* processor = (TTLDetector*)getProcessor();
	int prSec = (int)slider->getValue();
	preSeconds = (double)prSec/1.0f;
	//std::cout << preSeconds << std::endl;
	int poSec = (int)slider->getMaximum() - prSec;
	postSeconds = (double)poSec/1.0f;
	processor->setTimebase(preSeconds, postSeconds);
	//std::cout << postSeconds << std::endl;
	
}

double TTLDetectorEditor::getPreSeconds()
{
	return preSeconds;
}
double TTLDetectorEditor::getPostSeconds()
{
	return postSeconds;
}

void TTLDetectorEditor::buttonEvent(Button* button)
{


    if (electrodeButtons.contains((ElectrodeButton*) button))
    {

        if (electrodeEditorButtons[0]->getToggleState()) // EDIT is active
        {
            ElectrodeButton* eb = (ElectrodeButton*) button;
            int electrodeNum = eb->getChannelNum()-1;

            std::cout << "Channel number: " << electrodeNum << std::endl;
            Array<int> a;
            a.add(electrodeNum);
            channelSelector->setActiveChannels(a);

            TTLDetector* processor = (TTLDetector*) getProcessor();

           // thresholdSlider->setActive(true);
           // thresholdSlider->setValue(processor->getChannelThreshold(electrodeList->getSelectedItemIndex(),
            //                                                         electrodeButtons.indexOf((ElectrodeButton*) button)));
        }
        else
        {

            TTLDetector* processor = (TTLDetector*) getProcessor();

            ElectrodeButton* eb = (ElectrodeButton*) button;
            int electrodeNum = electrodeList->getSelectedItemIndex();
            int channelNum = electrodeButtons.indexOf(eb);

            processor->setChannelActive(electrodeNum,
                                        channelNum,
                                        button->getToggleState());

            std::cout << "Disabling channel " << channelNum <<
                      " of electrode " << electrodeNum << std::endl;

        }


    }


    int num = numElectrodes->getText().getIntValue();

    if (button == upButton)
    {
        numElectrodes->setText(String(++num), sendNotification);

        return;

    }
    else if (button == downButton)
    {

        if (num > 1)
            numElectrodes->setText(String(--num), sendNotification);

        return;

    }
	else if (button == averageButton)
	{
		if (acquisitionIsActive)
		{
			bool x = button->getToggleState();
			CoreServices::sendStatusMessage("Stop acquisition before.");
			averageButton->setToggleState(!x, dontSendNotification);
			
			return;
		}
		if (button->getToggleState())
			AlertWindow::showMessageBox(AlertWindow::InfoIcon, "TTL Averager", " ");
		else
			AlertWindow::showMessageBox(AlertWindow::InfoIcon, "TTL Detector", " ");
		TTLDetector* processor = (TTLDetector*)getProcessor();
		processor->getAvgState(button->getToggleState());
		refreshElectrodeList();
		CoreServices::updateSignalChain(this);
		CoreServices::highlightEditor(this);
		return;
	}
    else if (button == plusButton)
    {
        // std::cout << "Plus button pressed!" << std::endl;
        if (acquisitionIsActive)
        {
            CoreServices::sendStatusMessage("Stop acquisition before adding electrodes.");
            return;
        }

        int type = electrodeTypes->getSelectedId();
        // std::cout << type << std::endl;
        int nChans;

        switch (type)
        {
            case 1:
                nChans = 1;
                break;
            case 2:
                nChans = 2;
                break;
            case 3:
                nChans = 4;
                break;
            default:
                nChans = 1;
        }
		
        for (int n = 0; n < num; n++)
        {
            if (!addElectrode(nChans,getPreSeconds(),getPostSeconds()))
            {
                CoreServices::sendStatusMessage("Not enough channels to add electrode.");
            }
        }

        electrodeEditorButtons[1]->setToggleState(false, dontSendNotification);

		CoreServices::updateSignalChain(this);
		CoreServices::highlightEditor(this);
        return;

    }
    else if (button == electrodeEditorButtons[0])   // EDIT
    {

        Array<int> activeChannels;

        for (int i = 0; i < electrodeButtons.size(); i++)
        {
            if (button->getToggleState())
            {
                electrodeButtons[i]->setToggleState(false, dontSendNotification);
                electrodeButtons[i]->setRadioGroupId(299);
                channelSelector->activateButtons();
                channelSelector->setRadioStatus(true);
            }
            else
            {
                electrodeButtons[i]->setToggleState(true, dontSendNotification);
                electrodeButtons[i]->setRadioGroupId(0);
                channelSelector->inactivateButtons();
                channelSelector->setRadioStatus(false);
                activeChannels.add(electrodeButtons[i]->getChannelNum()-1);
            }
        }


        if (!button->getToggleState())
        {
            //thresholdSlider->setActive(false);

            // This will be -1 with nothing selected
            int selectedItemIndex = electrodeList->getSelectedItemIndex();
            if (selectedItemIndex != -1)
            {
                drawElectrodeButtons(selectedItemIndex);
            }
            else
            {
                electrodeButtons.clear();
            }
        }

        //   channelSelector->setActiveChannels(activeChannels);

        return;

    }
    else if (button == electrodeEditorButtons[1])   // MONITOR
    {

        Button* audioMonitorButton = electrodeEditorButtons[1];

        channelSelector->clearAudio();

        TTLDetector* processor = (TTLDetector*) getProcessor();

		Array<SimpleElectrode*> electrodes;
		processor->getElectrodes(electrodes);

        for (int i = 0; i < electrodes.size(); i++)
        {
            SimpleElectrode* e = electrodes[i];
            e->isMonitored = false;
        }

        SimpleElectrode* e = processor->getActiveElectrode();

        if (e != nullptr)
        {

            e->isMonitored = audioMonitorButton->getToggleState();

            for (int i = 0; i < e->numChannels; i++)
            {
                std::cout << "Channel " << e->channels[i] << std::endl;
                int channelNum = e->channels[i];
                channelSelector->setAudioStatus(channelNum, audioMonitorButton->getToggleState());

            }
        }
        else
        {
            audioMonitorButton->setToggleState(false, dontSendNotification);
        }

        return;
    }
    else if (button == electrodeEditorButtons[2])   // DELETE
    {
        if (acquisitionIsActive)
        {
            CoreServices::sendStatusMessage("Stop acquisition before deleting electrodes.");
            return;
        }

        removeElectrode(electrodeList->getSelectedItemIndex());

		CoreServices::updateSignalChain(this);
		CoreServices::highlightEditor(this);

        return;
    }
	else if (button == saveButton)
	{
		//std::cout << "Save button clicked." << std::endl;

		if (!acquisitionIsActive)
		{
			
			FileChooser fc("Choose the file name...",
				CoreServices::getDefaultUserSaveDirectory(),
				"*",
				true);

			if (fc.browseForFileToSave(true))
			{
				File fileToSave = fc.getResult();
				std::cout << fileToSave.getFileName() << std::endl;
				CoreServices::sendStatusMessage(writePrbFile(fileToSave));
			}
		}
		else {
			CoreServices::sendStatusMessage("Stop acquisition before saving the configuration.");
		}



	}
	else if (button == loadButton)
	{
		//std::cout << "Load button clicked." << std::endl;

		if (!acquisitionIsActive)
		{
			
			FileChooser fc("Choose a file to load...",
				CoreServices::getDefaultUserSaveDirectory(),
				"*",
				true);

			if (fc.browseForFileToOpen())
			{
				/*if (reorderActive)
					modifyButton->setToggleState(false, sendNotificationSync);*/
				
				File fileToOpen = fc.getResult();
				std::cout << fileToOpen.getFileName() << std::endl;
				
				CoreServices::sendStatusMessage(loadPrbFile(fileToOpen));
			}
		}
		else {
			CoreServices::sendStatusMessage("Stop acquisition before saving the configuration.");
		}
	}



}

void TTLDetectorEditor::channelChanged (int channel, bool /*newState*/)
{

    if (electrodeEditorButtons[0]->getToggleState()) // editing is active
    {
        //std::cout << "New channel: " << chan << std::endl;

        for (int i = 0; i < electrodeButtons.size(); i++)
        {
            if (electrodeButtons[i]->getToggleState())
            {
                electrodeButtons[i]->setChannelNum (channel);
                electrodeButtons[i]->repaint();

                TTLDetector* processor = (TTLDetector*) getProcessor();
                processor->setChannel(electrodeList->getSelectedItemIndex(),
                                      i,
                                      channel - 1);
            }
        }
    }

}

void TTLDetectorEditor::refreshElectrodeList()
{

    electrodeList->clear();

    TTLDetector* processor = (TTLDetector*) getProcessor();

    StringArray electrodeNames = processor->getElectrodeNames();

    for (int i = 0; i < electrodeNames.size(); i++)
    {
        electrodeList->addItem(electrodeNames[i], electrodeList->getNumItems()+1);
    }

    if (electrodeList->getNumItems() > 0)
    {
        electrodeList->setSelectedId(electrodeList->getNumItems(), sendNotification);
        electrodeList->setText(electrodeList->getItemText(electrodeList->getNumItems()-1));
        lastId = electrodeList->getNumItems();
        electrodeList->setEditableText(true);

        drawElectrodeButtons(electrodeList->getNumItems()-1);
    }
}

bool TTLDetectorEditor::addElectrode(int nChans, double preSec, double postSec, int electrodeID)
{
    TTLDetector* processor = (TTLDetector*) getProcessor();

	if (processor->addElectrode(nChans, preSec, postSec, electrodeID))
    {
        refreshElectrodeList();
        return true;
    }
    else
    {
        return false;
    }

}



void TTLDetectorEditor::removeElectrode(int index)
{
    std::cout << "Deleting electrode number " << index << std::endl;
    TTLDetector* processor = (TTLDetector*) getProcessor();
    processor->removeElectrode(index);
    refreshElectrodeList();

    int newIndex = jmin(index, electrodeList->getNumItems()-1);
    newIndex = jmax(newIndex, 0);

    electrodeList->setSelectedId(newIndex, sendNotification);
    electrodeList->setText(electrodeList->getItemText(newIndex));

    if (electrodeList->getNumItems() == 0)
    {
        electrodeButtons.clear();
        electrodeList->setEditableText(false);
    }
}

void TTLDetectorEditor::labelTextChanged(Label* label)
{
    if (label->getText().equalsIgnoreCase("1") && isPlural)
    {
        for (int n = 1; n < electrodeTypes->getNumItems()+1; n++)
        {
            electrodeTypes->changeItemText(n,
                                           electrodeTypes->getItemText(n-1).trimCharactersAtEnd("s"));
        }

        isPlural = false;

        String currentText = electrodeTypes->getText();
        electrodeTypes->setText(currentText.trimCharactersAtEnd("s"));

    }
    else if (!label->getText().equalsIgnoreCase("1") && !isPlural)
    {
        for (int n = 1; n < electrodeTypes->getNumItems()+1; n++)
        {
            String currentString = electrodeTypes->getItemText(n-1);
            currentString += "s";

            electrodeTypes->changeItemText(n,currentString);
        }
        isPlural = true;

        String currentText = electrodeTypes->getText();
        electrodeTypes->setText(currentText += "s");
    }

	CoreServices::updateSignalChain(this);

}

void TTLDetectorEditor::comboBoxChanged(ComboBox* comboBox)
{

    if (comboBox == electrodeList)
    {
        int ID = comboBox->getSelectedId();

        std::cout << "ID: " << ID << std::endl;

        if (ID == 0)
        {
            //TTLDetector* processor = (TTLDetector*) getProcessor();

            //processor->setElectrodeName(lastId, comboBox->getText());
            //comboBox->changeItemText(lastId, comboBox->getText());
            //electrodeList->setText(comboBox->getText());
            refreshElectrodeList();

        }
        else
        {

            lastId = ID;

            TTLDetector* processor = (TTLDetector*) getProcessor();
            SimpleElectrode* e = processor->setCurrentElectrodeIndex(ID-1);

            electrodeEditorButtons[1]->setToggleState(e->isMonitored, dontSendNotification);

            drawElectrodeButtons(ID-1);

        }
    }
	else if (comboBox == timeBaseList)
	{
		TTLDetector* processor = (TTLDetector*)getProcessor();
		int ID = comboBox->getSelectedId();
		//std::cout << ID << std::endl;
		String value = comboBox->getItemText(ID-1);
		timeBaseSlider->setRange(0,value.getFloatValue(),1);
		preSeconds = (double)timeBaseSlider->getValue() / 1.0f;
		postSeconds = (double)(timeBaseSlider->getMaximum() - timeBaseSlider->getValue()) / 1.0f;
		if (processor->checkTimebase(preSeconds, postSeconds))
		{
			lastIdTimebase = ID;
			processor->setTimebase(preSeconds, postSeconds);
			CoreServices::updateSignalChain(this);
			CoreServices::highlightEditor(this);
		}
		else
		{
			comboBox->setSelectedId(lastIdTimebase);
		}

		//std::cout << value.getFloatValue() << std::endl;
	}

    //thresholdSlider->setActive(false);
}

void TTLDetectorEditor::checkSettings()
{
    electrodeList->setSelectedId(0);
    drawElectrodeButtons(0);

	CoreServices::updateSignalChain(this);
	CoreServices::highlightEditor(this);

}

void TTLDetectorEditor::drawElectrodeButtons(int ID)
{

    TTLDetector* processor = (TTLDetector*) getProcessor();

    electrodeButtons.clear();

    int width = 20;
    int height = 15;

    int numChannels = processor->getNumChannels(ID);
    int row = 0;
    int column = 0;

    Array<int> activeChannels;
    Array<double> thresholds;

    for (int i = 0; i < numChannels; i++)
    {
        ElectrodeButton* button = new ElectrodeButton(processor->getChannel(ID,i)+1);
        electrodeButtons.add(button);

        thresholds.add(processor->getChannelThreshold(ID,i));

        if (electrodeEditorButtons[0]->getToggleState())
        {
            button->setToggleState(false, dontSendNotification);
            button->setRadioGroupId(299);
        }
        else
        {
            activeChannels.add(processor->getChannel(ID,i));

            button->setToggleState(processor->isChannelActive(ID,i), dontSendNotification);
        }

        if (numChannels < 3)
            button->setBounds(150+(column++)*width, 78+row*height, width, 15);
        else
            button->setBounds(150+(column++)*width, 70+row*height, width, 15);

        addAndMakeVisible(button);
        button->addListener(this);

        if (column%2 == 0)
        {
            column = 0;
            row++;
        }

    }

    channelSelector->setActiveChannels(activeChannels);
    // thresholdSlider->setValues(thresholds);
}

String TTLDetectorEditor::writePrbFile(File filename)
{

	FileOutputStream outputStream(filename);
	outputStream.setPosition(0);
	outputStream.truncate();
	//outputStream.writeString("channel_groups = ");

	TTLDetector* processor = (TTLDetector*)getProcessor();
	info = new DynamicObject();

	DynamicObject* nestedObj = new DynamicObject();
	Array<var> arr;
	Array<var> arr2;
	Array<var> arr3;
	int pos = electrodeList->getSelectedId();
	for (int i = 0; i < electrodeList->getNumItems(); i++)
	{
		arr.add(var(i));
		arr2.add(var(processor->getChannel(i, 0) + 1));
		arr3.add(var(processor->isChannelActive(i, 0)));
		
			
	}
	nestedObj->setProperty("electrodes", var(arr));
	nestedObj->setProperty("channel", var(arr2));
	nestedObj->setProperty("isActive", var(arr3));

	info->setProperty("0", nestedObj);

	info->writeAsJSON(outputStream, 2, false);

	return "Saved " + filename.getFileName();

}

String TTLDetectorEditor::loadPrbFile(File filename)
{
	FileInputStream inputStream(filename);

	var json = JSON::parse(inputStream);

	var returnVal = -255;

	var channelGroup = json.getProperty(Identifier("0"), returnVal);

	if (channelGroup.equalsWithSameType(returnVal))
	{
		return "Not a valid .prb file.";
	}

	var electrode = channelGroup[Identifier("electrodes")];
	Array<var>* elec = electrode.getArray();

	var channel = channelGroup[Identifier("channel")];
	Array<var>* chan = channel.getArray();

	var isActive = channelGroup[Identifier("isActive")];
	Array<var>* isAct = isActive.getArray();

	std::cout << "We found this many: " << elec->size() << std::endl;
	
	/*if (map->size() > previousChannelCount)
		createElectrodeButtons(map->size(), false);*/

	int numElec = electrodeList->getNumItems();
	for (int j = numElec-1; j >= 0; j--)
	{
		removeElectrode(j);
	}

	for (int i = 0; i < elec->size(); i++)
	{
		int aa = elec->getUnchecked(i);
		addElectrode(1, getPreSeconds(), getPostSeconds(), i);

		int bb = chan->getUnchecked(i);
		bool cc = isAct->getUnchecked(i);
		for (int j = 0; j < electrodeButtons.size(); j++)
		{
			electrodeButtons[j]->setChannelNum(bb);
			electrodeButtons[j]->repaint();
			electrodeButtons[j]->setToggleState(cc, dontSendNotification);
			

			TTLDetector* processor = (TTLDetector*)getProcessor();
			processor->setChannel(electrodeList->getSelectedItemIndex(),
				j,
				bb-1);
			processor->setChannelActive(electrodeList->getSelectedItemIndex(),
				j,
				electrodeButtons[j]->getToggleState());
			if (!electrodeButtons[j]->getToggleState())
				std::cout << "Disabling channel " << j << " of electrode " << electrodeList->getSelectedItemIndex() << std::endl;
			
		}
	}
	CoreServices::updateSignalChain(this);
	CoreServices::highlightEditor(this);
		

	return "Loaded " + filename.getFileName();

}