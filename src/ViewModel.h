#pragma once

#include "Env.h"
#include "Config.h"

class ViewModel {
	Config* config;
public:

	ViewModel(Config* config);

	float volume;

	int receiverMode = LSB;

	int windowWidth = 0;

	float absoluteXpos = 0, stepX = 0, receiverPos = 0;

	int selectedBin = 0;
	int selectedFreq = 0;

	double amp = 0;

	int filterWidth;

	double signalMaxdB = 0.0;

	//41965664 - �������� ssb � ������
	//int frequency = 41965664;
	int frequency = 3700000;

	int gain = -30;

	bool gainControl = false;
	bool audioFilter = true;

	double serviceField1 = 0;
	double serviceField2 = 0;

};