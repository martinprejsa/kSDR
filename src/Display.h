#pragma once

#include "Environment.h"

#include "FlowingFFTSpectre.h"
#include "../include/GLFW/glfw3.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_stdlib.h"
#include "KalmanFilter.h"
#include "SMeter.h"
#include "Spectre.h"
#include "CPU.h"
#include "RSPv2.h"
#include "ListSetting.h"
#include "functional"
#include "map"
#include "RTLDevice.h"
#include "MemoryRecordUserInterface.h"

#include <stb/stb_image.h>

class Display {

private:
	
	GLFWwindow* window = nullptr;

	static void framebufferReSizeCallback(GLFWwindow* window, int width, int height);

	static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);

	static void windowSizeCallback(GLFWwindow* window, int width, int height);

	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

	SMeter* smeter = new SMeter(50, 50, 400, 0.2);

	CPU cpu;

	void showSelectDeviceSetting();
	void showHackrfSamplingRateSetting();

	std::unique_ptr<ListSetting> decimationLS;
	std::unique_ptr<ListSetting> hackRFsampRateLS;
	std::unique_ptr<ListSetting> hackRFbasebandFilterLS;
	std::unique_ptr<ListSetting> rspSampRateLS;
	std::unique_ptr<ListSetting> rspDecimationFactorLS;
	std::unique_ptr<ListSetting> rspbasebandFilterLS;
	std::unique_ptr<ListSetting> fftLenLS;
	std::unique_ptr<ListSetting> waterfallSpeedLS;
	std::unique_ptr<ListSetting> smoothingDepthLS;
	std::unique_ptr<ListSetting> spectreStyleLS;
	std::unique_ptr<ListSetting> rtlDeviceGainLS;
	std::unique_ptr<ListSetting> rtlSampRateLS;

	void initSettings();
	void initDynamicSettings();

	void showColorPicker(string title, unsigned int* configVal, bool withTransparency);

	MemoryRecordUserInterface memoryRecordUserInterface;

	Environment* env;

	ViewModel* viewModel;

public:

	static Display* instance;

	Spectre* spectre;

	int width = 0;
	int height = 0;

	int whichMouseBtnPressed = -1;
	int isMouseBtnPressed = 0;

	double mouseX = 0;
	double mouseY = 0;
	
	Display(Environment* env);
	~Display();

	int init();

	void mainLoop();

	void initImGUI();

	void renderImGUIFirst();

	void showAlertOKDialog(std::string title, std::string msg);

	ViewModel* getViewModel();

};