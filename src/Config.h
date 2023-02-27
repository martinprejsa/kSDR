#pragma once

#define CONFIG_FILENAME "config.xml"

#include "Device.h"
#include "stdio.h"
#include "tinyxml2/tinyxml2.h"
#include "string"

//using namespace std;

class Config {

public:

	Device* device;

	int circleBufferLen;

	int inputSamplerate;

	int inputSamplerateDivider = 1;
	int delayedInputSamplerateDivider = inputSamplerateDivider;

	int outputSamplerate;

	int outputSamplerateDivider;

	int inputChannelNumber;
	int outputChannelNumber;

	//������ ������� ������ ������ � �������� �����
	int bufferWriteAudioLen;

	//������ ���������� �� ������� ��������� ������
	int readSoundProcessorBufferLen;

	//������ ������ � ������ ��������� �������
	int audioWriteFrameLen;

	int audioReadFrameLen;

	int readSize;

	int hilbertTransformLen;
	int polyphaseFilterLen;

	int fftLen = 64 * 1024;
	int delayedFFTLen = fftLen;

	float fftBandwidth;

	int defaultFilterWidth = 2700;

	double startFrequency = 7100000.0;

	Config();
	~Config();

	int visibleSpectreBinCount = 2048;

	//This data will be stored in config xml
	float waterfallMin = 0;
	float waterfallMax = 0;
	float spectreRatio = 0;
	float spectreMin = 0;
	int spectreSpeed = 0;
	float lastSelectedFreq = (float)startFrequency;
	float volume = 1;

	int filterWidth = defaultFilterWidth;
	
	int startBin = 0;
	int stopBin = fftLen / 2 - 1;

	int removeDCBias = 1;

	struct Receiver {
		int modulation = 1;
		int frequencyShift = 0;
		bool enableFrequencyShift = 0;

		struct AGC {
			double threshold = 0.01;
			double atackSpeedMs = 1.0;
			double holdingTimeMs = 100;
			double releaseSpeed = 0.00005;
		} agc;

	} receiver;

	enum DeviceType {
		RSP,
		HACKRF,
		RTL
	} deviceType, delayedDeviceType;

	struct HackRF {
		int deviceSamplingRate = 4000000;
		int basebandFilter = 1750000;
		char rxAmp = 0;
		char lnaGain = 16;
		char vgaGain = 20;
		char txAmp = 20;
	} hackrf;

	struct RSP {
		int deviceSamplingRate = 4000000;
		int deviceDecimationFactor = 1;
		int gain = 50;
		int lna = 0;
		char api = 2;
	} rsp;

	struct RTL {
		int deviceSamplingRate = 500000;
		int gain = 50;
	} rtl;

	struct App {
		int winWidth = 1920;
		int winHeight = 1080;
	} app;

	struct ColorTheme {
		unsigned int spectreFillColor = 0xFFFFFFFF;
		unsigned int spectreProfileColor = 0xFFFFFFFF;
		unsigned int mainBGColor = 0xFFFFFFFF;
		unsigned int windowsBGColor = 0xFFFFFFFF;
		unsigned int windowsTitleBGColor = 0xFFFFFFFF;
		unsigned int receiveRegionColor = 0xFFFFFFFF;
	} colorTheme;

	//--------------------------------------

	void load();
	void save();

	void setDevice(int deviceID);

private:
	void calcOutputSamplerate();
};