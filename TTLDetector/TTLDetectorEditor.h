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

#ifndef __TTLDetectorEDITOR_H_F0BD2DD9__
#define __TTLDetectorEDITOR_H_F0BD2DD9__


#include <EditorHeaders.h>

class TriangleButton;
class UtilityButton;



/**

  User interface for the TTLDetector processor.

  Allows the user to add single electrodes, stereotrodes, or tetrodes.

  Parameters of individual channels, such as channel mapping, threshold,
  and enabled state, can be edited.

  @see TTLDetector

*/

class TTLDetectorEditor : public GenericEditor,
    public Label::Listener,
    public ComboBox::Listener

{
public:
    TTLDetectorEditor(GenericProcessor* parentNode, bool useDefaultParameterEditors);
    virtual ~TTLDetectorEditor();
    void buttonEvent(Button* button);
    void labelTextChanged(Label* label);
    void comboBoxChanged(ComboBox* comboBox);
    void sliderEvent(Slider* slider);

    void channelChanged (int channel, bool newState) override;

    bool addElectrode(int nChans, double preSec, double postSec, int electrodeID = 0);
    void removeElectrode(int index);

	double getPreSeconds();
	double getPostSeconds();

    void checkSettings();
    void refreshElectrodeList();

	String writePrbFile(File filename);
	String loadPrbFile(File filename);

private:

    void drawElectrodeButtons(int);

    ComboBox* electrodeTypes;
    ComboBox* electrodeList;
    Label* numElectrodes;
	ComboBox* timeBaseList;
	Label* timeBaseLabel;
    TriangleButton* upButton;
    TriangleButton* downButton;
    UtilityButton* plusButton;
	UtilityButton* averageButton;
    Slider* timeBaseSlider;
	LoadButton* loadButton;
	SaveButton* saveButton;

    OwnedArray<ElectrodeButton> electrodeButtons;
    Array<ElectrodeEditorButton*> electrodeEditorButtons;

    void editElectrode(int index, int chan, int newChan);

    int lastId;
	int lastIdTimebase;
    bool isPlural;
	double preSeconds;
	double postSeconds;

	Array<int> channelArray;
	Array<bool> enabledChannelArray;

    Font font;
	ScopedPointer<DynamicObject> info;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TTLDetectorEditor);

};




#endif  // __TTLDetectorEDITOR_H_F0BD2DD9__
