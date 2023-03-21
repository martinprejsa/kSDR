#include "Display.h"

void Display::framebufferReSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	if (Display::instance != NULL) {
		Display::instance->width = width;
		Display::instance->height = height;
	}
}

void Display::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
	if (Display::instance != NULL) {
		//cout << xpos << " " << ypos << "\r\n";
		//cout << Display::display->io.WantCaptureMouse;
		auto& io = ImGui::GetIO();
		if (!io.WantCaptureMouse) {
			Display::instance->mouseX = xpos;
			Display::instance->mouseY = ypos;
		}
	}
}

void Display::windowSizeCallback(GLFWwindow* window, int width, int height) {
	framebufferReSizeCallback(window, width, height);
}

void Display::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (Display::instance != NULL) {
		auto& io = ImGui::GetIO();
		if (!io.WantCaptureMouse) {
			Display::instance->whichMouseBtnPressed = button;
			Display::instance->isMouseBtnPressed = action;
		}
	}
}

Display::Display(Environment* environment, FFTSpectreHandler* fftSH) {
	this->environment = environment;
	config = environment->getConfig();
	viewModel = new ViewModel(config);
	fftSH->vM = viewModel;
	this->flowingFFTSpectre = new FlowingFFTSpectre(config, viewModel, fftSH);
	spectre = new Spectre(config, viewModel, flowingFFTSpectre);
	memoryRecordUserInterface = MemoryRecordUserInterface(config, viewModel, spectre);
}

bool errorInitDeviceUserInformed = false;

int Display::init() {

	glfwInit();

	glfwSetTime(0);

	/*int i;
	GLFWmonitor** monitors = glfwGetMonitors(&i);

	const GLFWvidmode* mode = glfwGetVideoMode(monitors[0]);

	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);*/

	// Create a GLFWwindow object
	window = glfwCreateWindow(config->app.winWidth, config->app.winHeight, "kSDR", nullptr, nullptr);
	if (window == nullptr) {
		printf("Failed to create GLFW window\r\n");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	glfwSwapInterval(1);

	glfwSetFramebufferSizeCallback(window, framebufferReSizeCallback);
	glfwSetWindowSizeCallback(window, windowSizeCallback);

	glfwSetCursorPosCallback(window, cursorPositionCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);

	initImGUI();

	glfwSwapBuffers(window);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//glEnable(GL_LINE_SMOOTH);

	//glLineWidth(1.5);

	int channels, width, height;
	unsigned char* pixels = stbi_load("icon.png", &width, &height, &channels, 4); //rgba channels 
	
	GLFWimage images[1];
	images[0].width = width;
	images[0].height = height;
	images[0].pixels = pixels;

	glfwSetWindowIcon(window, 1, images);
	
	stbi_image_free(images[0].pixels);

	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1;

	ImGuiStyle& style = ImGui::GetStyle();
	style.AntiAliasedLines = true;
	//style.AntiAliasedFill = true;

	initSettings();
}

void Display::mainLoop() {
	// Game loop
	while (!glfwWindowShouldClose(window)) {
		// Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		ImVec4 bgColor = ImGui::ColorConvertU32ToFloat4(config->colorTheme.mainBGColor);

		glClearColor(bgColor.x, bgColor.y, bgColor.z, bgColor.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderImGUIFirst();

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	glfwTerminate();
}

Display::~Display() {
	config->lastSelectedFreq = spectre->receiverLogicNew->getSelectedFreqNew(); //saving last selected freq to config class
	delete viewModel;
	delete flowingFFTSpectre;
}


void Display::initImGUI() {
	const char* glsl_version = "#version 130";

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	auto& io = ImGui::GetIO(); (void)io;

	//ImFont* fontStandard = io.Fonts->AddFontDefault();
	//viewModel->fontMyRegular = io.Fonts->AddFontFromFileTTF("DroidSansMono.ttf", 18);
	viewModel->fontMyRegular = io.Fonts->AddFontFromFileTTF("m-regular.ttf", 18);
	viewModel->fontBigRegular = io.Fonts->AddFontFromFileTTF("m-regular.ttf", 48);

	//ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	ImGui::StyleColorsClassic();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void Display::renderImGUIFirst() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	DeviceN* device = environment->getDeviceController()->getDevice();
	DeviceController* deviceController = environment->getDeviceController();

	//Store window size
	config->app.winWidth = width;
	config->app.winHeight = height;

	//Проверка которая исключает настройку приемника при перетягивании стороннего окна в область окна спектра
	if (ImGui::IsMouseDown(0)) {
		if (!spectre->isMouseOnRegion(
			Spectre::Region{ 
				ImVec2(spectre->windowFrame.UPPER_RIGHT.x, spectre->windowFrame.UPPER_RIGHT.y),
				ImVec2(spectre->windowFrame.BOTTOM_LEFT.x, spectre->windowFrame.BOTTOM_LEFT.y) }
			)
		) viewModel->mouseBusy = true;
	} else {
		viewModel->mouseBusy = false;
	}

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(config->colorTheme.windowsBGColor));
	//ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImGui::ColorConvertU32ToFloat4(config->colorTheme.windowsTitleBGColor));

	initDynamicSettings();

	//ImGui::PushID(9);
	ImGui::Begin(APP_NAME);

		smeter->draw(viewModel->signalMaxdB);

		ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

		if (ImGui::Button("GO")) {
			spectre->receiverLogicNew->setFreq((float)viewModel->goToFreq);
		}
		ImGui::SameLine();
		ImGui::InputInt("To freq", &viewModel->goToFreq, 500, 1000);

		if (ImGui::Button("160m")) { spectre->receiverLogicNew->setFreq(1900000); viewModel->receiverMode = LSB; } ImGui::SameLine();
		if (ImGui::Button("80m")) { spectre->receiverLogicNew->setFreq(3700000); viewModel->receiverMode = LSB; } ImGui::SameLine();
		if (ImGui::Button("40m")) { spectre->receiverLogicNew->setFreq(7100000); viewModel->receiverMode = LSB; } ImGui::SameLine();
		if (ImGui::Button("30m")) { spectre->receiverLogicNew->setFreq(10300000); viewModel->receiverMode = USB; } ImGui::SameLine();
		if (ImGui::Button("20m")) { spectre->receiverLogicNew->setFreq(14150000); viewModel->receiverMode = USB; }
		if (ImGui::Button("17m")) { spectre->receiverLogicNew->setFreq(18100000); viewModel->receiverMode = USB; } ImGui::SameLine();
		if (ImGui::Button("15m")) { spectre->receiverLogicNew->setFreq(21100000); viewModel->receiverMode = USB; } ImGui::SameLine();
		if (ImGui::Button("12m")) { spectre->receiverLogicNew->setFreq(24900000); viewModel->receiverMode = USB; } ImGui::SameLine();
		if (ImGui::Button("10m")) { spectre->receiverLogicNew->setFreq(28500000); viewModel->receiverMode = USB; }

		ImGui::RadioButton("USB", &viewModel->receiverMode, USB); ImGui::SameLine();
		ImGui::RadioButton("LSB", &viewModel->receiverMode, LSB); ImGui::SameLine();
		ImGui::RadioButton("AM", &viewModel->receiverMode, AM); ImGui::SameLine();
		ImGui::RadioButton("nFM", &viewModel->receiverMode, nFM);

		ImGui::SliderFloat("Volume", &viewModel->volume, 0, 5);

		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {
			if (ImGui::BeginTabItem("Controls")) {
				ImGui::Spacing();

				ImGui::SeparatorText("Receiver");

				ImGui::SliderInt("Center freq", &viewModel->centerFrequency, 1000000, 30000000);

				if (ImGui::Button("+")) {
					flowingFFTSpectre->zoomIn();
					spectre->receiverLogicNew->setFrequencyDelta(spectre->receiverLogicNew->getFrequencyDelta());
				} ImGui::SameLine();
				if (ImGui::Button("-")) {
					flowingFFTSpectre->zoomOut();
					spectre->receiverLogicNew->setFrequencyDelta(spectre->receiverLogicNew->getFrequencyDelta());
				} ImGui::SameLine();
				if (ImGui::Button("<-")) {
					flowingFFTSpectre->move(-250);
					spectre->receiverLogicNew->setFrequencyDelta(spectre->receiverLogicNew->getFrequencyDelta());
				} ImGui::SameLine();
				if (ImGui::Button("->")) {
					flowingFFTSpectre->move(250);
					spectre->receiverLogicNew->setFrequencyDelta(spectre->receiverLogicNew->getFrequencyDelta());
				}

				ImGui::Spacing();

				ImGui::SeparatorText("Filter");

				ImGui::SliderInt("Width", &viewModel->filterWidth, 0, 12000); 
				
				if (ImGui::Button("100")) {
					viewModel->filterWidth = 100;
				}
				ImGui::SameLine();
				if (ImGui::Button("500")) {
					viewModel->filterWidth = 500;
				}
				ImGui::SameLine();
				if (ImGui::Button("1.2k")) {
					viewModel->filterWidth = 1200;
				}
				ImGui::SameLine();
				if (ImGui::Button("2.7k")) {
					viewModel->filterWidth = 2700;
				}
				ImGui::SameLine();
				if (ImGui::Button("3.2k")) {
					viewModel->filterWidth = 3200;
				}
				ImGui::SameLine();
				if (ImGui::Button("6.0k")) {
					viewModel->filterWidth = 6000;
				}
				ImGui::SameLine();
				if (ImGui::Button("8.0k")) {
					viewModel->filterWidth = 8000;
				}
				ImGui::SameLine();
				if (ImGui::Button("12.0k")) {
					viewModel->filterWidth = 12000;
				}
				
				ImGui::Spacing();

				ImGui::SeparatorText("Waterfall");

				ImGui::SliderFloat("Min", &viewModel->waterfallMin, -150, 0);

				ImGui::SliderFloat("Max", &viewModel->waterfallMax, -150, 100); ImGui::Spacing();

				ImGui::SeparatorText("Spectre");

				ImGui::SliderFloat("Ratio", &viewModel->ratio, -200, 100);

				ImGui::SliderFloat("Min val", &viewModel->minDb, -200, 0);

				ImGui::SliderInt("Speed", &config->spectre.spectreSpeed, 1, 50);
				ImGui::SliderInt("Speed 2", &config->spectre.spectreSpeed2, 1, 50); ImGui::Spacing();

				//ImGui::SliderInt("Test", &viewModel->test, 2300000, 2700000);

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Memory")) {
				ImGui::Spacing();
				memoryRecordUserInterface.drawMemoryBlock();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Device Options")) {
				ImGui::Spacing();

				showSelectDeviceSetting();

				if (config->deviceType == DeviceType::HACKRF) {
					hackRFsampRateLS->drawSetting();

					HackRFDevice* hackRFDevice = (HackRFDevice*)(environment->getDeviceController()->getDevice());
					HackRfInterface* hackRfInterface = environment->getDeviceController()->getHackRfInterface();

					ImGui::SliderInt("LNA Gain", &viewModel->hackRFModel.lnaGain, 0, 5);
					hackRfInterface->setLnaGain((uint32_t)viewModel->hackRFModel.lnaGain);

					ImGui::SliderInt("VGA Gain", &viewModel->hackRFModel.vgaGain, 0, 31);
					hackRfInterface->setVgaGain(viewModel->hackRFModel.vgaGain);

					ImGui::SliderInt("AMP Gain", &viewModel->hackRFModel.enableAmp, 0, 1);
					hackRfInterface->enableAmp(viewModel->hackRFModel.enableAmp);

					hackRFbasebandFilterLS->drawSetting();
					hackRfInterface->setBaseband(config->hackrf.basebandFilter);
					
					if (environment->getDeviceController()->isReadyToReceiveCmd()) hackRfInterface->sendParamsToDevice();
				}

				if (config->deviceType == DeviceType::RSP) {
					ImGui::Text("\nRSP Settings:");

					rspSampRateLS->drawSetting();

					rspDecimationFactorLS->drawSetting();

					bool useRspApiv3 = (config->rsp.api == 3) ? true : false;
					ImGui::Checkbox("Use APIv3 (instead of v2) (*)", &useRspApiv3);
					config->rsp.api = (useRspApiv3 == true) ? 3 : 2;

					ImGui::SliderInt("Gain", &viewModel->rspModel.gain, 20, 59);
					ImGui::Checkbox("Disable LNA", &viewModel->rspModel.lna);

					rspbasebandFilterLS->drawSetting();
				}

				if (config->deviceType == DeviceType::RTL) {
					rtlSampRateLS->drawSetting();
					rtlDeviceGainLS->drawSetting();
				}

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Settings")) {
				ImGui::Spacing();
				ImGui::Text("After changing the settings marked with an asterisk, you need to restart the application.\n");

				decimationLS->drawSetting();

				ImGui::Spacing();
				ImGui::SeparatorText("Frequency shift");

				ImGui::InputInt("Shift in Hz", &config->receiver.frequencyShift);
				ImGui::Checkbox("Enable shift", &config->receiver.enableFrequencyShift);

				ImGui::Spacing();
				ImGui::SeparatorText("AGC settings");

				ImGui::InputDouble("Sound threshold", &config->receiver.agc.threshold, 0.001f, 0.1f, "%.3f");
				ImGui::InputDouble("Atack time (ms)", &config->receiver.agc.atackSpeedMs, 0.1f, 0.1f, "%.1f");
				ImGui::InputDouble("Hold time (ms)", &config->receiver.agc.holdingTimeMs, 0.1f, 0.1f, "%.1f");
				ImGui::InputDouble("Release time", &config->receiver.agc.releaseSpeed, 0.00001f, 0.1f, "%.7f");

				ImGui::Spacing();
				ImGui::SeparatorText("Color theme");

				showColorPicker(string("Windows Background"), &config->colorTheme.windowsBGColor, false);
				showColorPicker(string("Main Background"), &config->colorTheme.mainBGColor, false);
				//showColorPicker(string("Window Title Background"), &config->colorTheme.windowsTitleBGColor);
				showColorPicker(string("Spectre Fill"), &config->colorTheme.spectreFillColor, false);
				showColorPicker(string("Spectre Profile"), &config->colorTheme.spectreProfileColor, false);
				showColorPicker(string("Receive Region"), &config->colorTheme.receiveRegionColor, true);

				ImGui::Spacing();
				ImGui::SeparatorText("Spectre settings");

				spectreStyleLS->drawSetting();
				ImGui::Checkbox("Contour shows signal power", &config->spectre.contourShowsPower);
				ImGui::SliderFloat("Top coeff", &config->spectre.topCoeff, 0.5f, 1.5f);
				ImGui::SliderFloat("Bottom coeff", &config->spectre.bottomCoeff, 0.5f, 1.5f);
				smoothingDepthLS->drawSetting();
				ImGui::Checkbox("Hang&Decay", &config->spectre.hangAndDecay);
				ImGui::BeginDisabled(!config->spectre.hangAndDecay);
					ImGui::SliderFloat("Decay speed", &config->spectre.decaySpeed, 0, 1);
					ImGui::SliderFloat("Decay speed delta", &config->spectre.decaySpeedDelta, 0, 2);
				ImGui::EndDisabled();

				ImGui::SliderInt("Spectre correction Db", &config->spectre.spectreCorrectionDb, -200, 200);

				ImGui::Spacing();
				ImGui::SeparatorText("Other");

				fftLenLS->drawSetting();

				ImGui::Checkbox("Remove DC", &viewModel->removeDCBias);

				waterfallSpeedLS->drawSetting();

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Info")) {
				ImGui::Spacing();
				ImGui::Text("Sampling rate: %d", config->inputSamplerate);
				ImGui::Text("FFT length: %d", config->fftLen);
				ImGui::Text("AMP: %.2f", viewModel->amp);
				ImGui::Text("CPU usage: %.1f%%", cpu.getCurrentValue());
				ImGui::Text("Buffer available: %.2f sec", viewModel->bufferAvailable);
				ImGui::Text("Service field1: %f", viewModel->serviceField1);
				ImGui::Text("Service field2: %f", viewModel->serviceField2);
				ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("About")) {
				ImGui::Spacing();
				string msg;
				msg.append(APP_NAME).append("\n\n");
				msg.append("For all questions related to our software,\nplease, contact our email box: dkcocto@gmail.com.\n\n");
				msg.append("We will be glad to hear your wishes and suggestions.");

				ImGui::Text(msg.c_str());

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
		
		//Если вкладка опций устройства не выбрана, то все равно устанавливаем конфигурацию на текущее устройство
		if (deviceController->isReadyToReceiveCmd()) {
			deviceController->getHackRfInterface()->setFreq(viewModel->centerFrequency);
			deviceController->getHackRfInterface()->sendParamsToDevice();
		}
		//if (device != nullptr && config->deviceType == DeviceType::RTL) ((RTLDevice*)config->device)->setConfiguration();

	ImGui::End();

	spectre->draw();
	if (device != nullptr) {
		showAlertOKDialog(std::string("Warning"), std::string("Application couldn't init a selected device.\nPlease, go to settings and select the correct device or plug your device to USB port.\nMake sure you have selected the correct api version in the settings for RSP devices.\n\nReturned answer:\n\n").append(deviceController->getResult()->err));
		if (deviceController->getResult()->status != DeviceN::INIT_OK && !errorInitDeviceUserInformed) {
			spectre->disableControl(DISABLE_CONTROL_DIALOG);
			ImGui::OpenPopup(std::string("Warning").c_str());
			errorInitDeviceUserInformed = true;
		}
	}
	ImGui::PopStyleColor();
}

void Display::showSelectDeviceSetting() {
	const char* items[] = { "RSP1/RSP1A", "HackRF", "RTLSDR" };
	static int item_current_idx = config->deviceType; // Here we store our selection data as an index.
	const char* combo_preview_value = items[item_current_idx];  // Pass in the preview value visible before opening the combo (it could be anything)
	if (ImGui::BeginCombo("Device (*)", combo_preview_value, 0)) {
		for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
			const bool is_selected = (item_current_idx == n);
			if (ImGui::Selectable(items[n], is_selected)) {
				item_current_idx = n;
				config->setDevice(item_current_idx);
			}
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
}

void Display::showHackrfSamplingRateSetting() {
	std::map<int, std::string> samplingRateMap = { 
		{0 , "2000000"},
		{1 , "4000000"},
		{2 , "5000000"},
		{3 , "8000000"},
		{4 , "10000000"},
		{5 , "12500000"},
		{6 , "16000000"},
		{7 , "20000000"}
	};

	const char* items[] = { "2000000", "4000000", "5000000", "8000000", "10000000", "12500000", "16000000", "20000000" };
	static int item_current_idx = config->deviceType; // Here we store our selection data as an index.
	const char* combo_preview_value = items[item_current_idx];  // Pass in the preview value visible before opening the combo (it could be anything)
	if (ImGui::BeginCombo("Sampling rate (*)", combo_preview_value, 0)) {
		for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
			const bool is_selected = (item_current_idx == n);
			if (ImGui::Selectable(items[n], is_selected)) {
				item_current_idx = n;
				uint32_t samplingRate;
				Utils::parse_u32((char*)items[n], &samplingRate);
				config->hackrf.deviceSamplingRate = samplingRate;
			}
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
}

void Display::showAlertOKDialog(std::string title, std::string msg) {
	if (ImGui::BeginPopupModal(title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text(msg.c_str());
		ImGui::Separator();

		if (ImGui::Button("OK", ImVec2(120, 0))) { spectre->enableControl(DISABLE_CONTROL_DIALOG); ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void Display::showColorPicker(string title, unsigned int *configVal, bool withTransparency) {
	static bool hdr = false;
	static bool drag_and_drop = false;
	static bool alpha_half_preview = false;
	static bool options_menu = false;
	static bool alpha_preview = false;

	ImGuiColorEditFlags misc_flags = (hdr ? ImGuiColorEditFlags_HDR : 0) | (drag_and_drop ? 0 : ImGuiColorEditFlags_NoDragDrop) | (alpha_half_preview ? ImGuiColorEditFlags_AlphaPreviewHalf : (alpha_preview ? ImGuiColorEditFlags_AlphaPreview : 0)) | (options_menu ? 0 : ImGuiColorEditFlags_NoOptions);
	//static ImVec4 color = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 1.0f);

	ImVec4 color = ImGui::ColorConvertU32ToFloat4(*configVal);

	if (!withTransparency) {
		ImGui::ColorEdit3(title.c_str(), (float*)&color, misc_flags);
	} else {
		ImGui::ColorEdit4(title.c_str(), (float*)&color, misc_flags);
	}

	//unsigned long rgb = (r<<16)|(g<<8)|b; 

	*configVal = ImGui::ColorConvertFloat4ToU32(color);

	//std::string s = std::format("{:x}", ccc);
	//printf("%s\r\n", s.c_str());
}


void Display::initDynamicSettings() {
	std::map<int, std::string> decimationMap;
	for (int i = 1, j = 0; i <= 64; i++) {
		if (config->deviceType == DeviceType::HACKRF) {
			if (config->hackrf.deviceSamplingRate % i == 0) {
				decimationMap.insert(pair<int, std::string>{j, to_string(i)});
				j++;
			}

		}
		if (config->deviceType == DeviceType::RSP) {
			if ((config->rsp.deviceSamplingRate / config->rsp.deviceDecimationFactor) % i == 0) {
				decimationMap.insert(pair<int, std::string>{j, to_string(i)});
				j++;
			}
		}
		if (config->deviceType == DeviceType::RTL) {
			if (config->rtl.deviceSamplingRate % i == 0) {
				decimationMap.insert(pair<int, std::string>{j, to_string(i)});
				j++;
			}
		}
	}
	decimationLS = std::make_unique<ListSetting>(config, decimationMap, "Decimation", true);
	decimationLS->bindVariable(&config->delayedInputSamplerateDivider);

	if (config->deviceType == DeviceType::RSP) {
		std::map<int, std::string> rspDecimationFactorMap;
		for (int i = 1, j = 0; i <= 32; i++) {
			if (config->rsp.deviceSamplingRate % i == 0 && (i & (i - 1)) == 0) {
				rspDecimationFactorMap.insert(pair<int, std::string>{j, to_string(i)});
				j++;
			}
		}

		rspDecimationFactorLS = std::make_unique<ListSetting>(config, rspDecimationFactorMap, "Decimation factor", true);
		rspDecimationFactorLS->bindVariable(&config->rsp.deviceDecimationFactor);
	}
}

void Display::initSettings() {
	//auto begin = std::chrono::steady_clock::now();

	if (config->deviceType == DeviceType::HACKRF) {
		//-------HackRF settings
		std::map<int, std::string> hackRFsamplingRateMap = {
			{0 , "2000000"},
			{1 , "4000000"},
			{2 , "5000000"},
			{3 , "8000000"},
			{4 , "10000000"},
			{5 , "12500000"},
			{6 , "16000000"},
			{7 , "20000000"}
		};

		hackRFsampRateLS = std::make_unique<ListSetting>(config, hackRFsamplingRateMap, "Sampling rate", true);
		hackRFsampRateLS->bindVariable(&config->hackrf.deviceSamplingRate);

		//const char* items[] = { "1750000", "2500000", "3500000", "5000000", "5500000", "6000000", "7000000", "8000000", "9000000", "10000000", "20000000" };
		std::map<int, std::string> hackRFBasebandFilterMap = {
			{0 , "1750000"},
			{1 , "2500000"},
			{2 , "3500000"},
			{3 , "5000000"},
			{4 , "5500000"},
			{5 , "6000000"},
			{6 , "7000000"},
			{7 , "8000000"},
			{8 , "9000000"},
			{9 , "10000000"},
			{10 , "20000000"}
		};

		hackRFbasebandFilterLS = std::make_unique<ListSetting>(config, hackRFBasebandFilterMap, "Baseband filter", false);
		hackRFbasebandFilterLS->bindVariable(&config->hackrf.basebandFilter);
		//--------------------
	}

	if (config->deviceType == DeviceType::RSP) {
		//-------RSP settings-
		std::map<int, std::string> rspSamplingRateMap = {
			{0 , "2000000"},
			{1 , "3000000"},
			{2 , "4000000"},
			{3 , "6000000"},
			{4 , "7000000"},
			{5 , "8000000"},
			{6 , "10000000"}
		};

		rspSampRateLS = std::make_unique<ListSetting>(config, rspSamplingRateMap, "Sampling rate", true);
		rspSampRateLS->bindVariable(&config->rsp.deviceSamplingRate);


		std::map<int, std::string> rspbasebandFilterMap = {
			{0 , "0"},
			{1 , "200"},
			{2 , "300"},
			{3 , "600"},
			{4 , "1536"},
			{5 , "5000"},
			{6 , "6000"},
			{7 , "7000"},
			{8 , "8000"}
		};
		rspbasebandFilterLS = std::make_unique<ListSetting>(config, rspbasebandFilterMap, "Baseband Filter", false);
		rspbasebandFilterLS->bindVariable(&config->rsp.basebandFilter);
		//--------------------
	}

	if (config->deviceType == DeviceType::RTL) {
		//-10, 15, 40, 65, 90, 115, 140, 165, 190,
		//215, 240, 290, 340, 420, 430, 450, 470, 490
		std::map<int, std::string> rtlGainMap = {
			{0 , "-10"},
			{1 , "15"},
			{2 , "40"},
			{3 , "65"},
			{4 , "90"},
			{5 , "115"},
			{6 , "140"},
			{7 , "165"},
			{8 , "190"},
			{9 , "215"},
			{10 , "240"},
			{11 , "290"},
			{12 , "340"},
			{13 , "420"},
			{14 , "430"},
			{15 , "450"},
			{16 , "470"},
			{17 , "490"}
		};
		rtlDeviceGainLS = std::make_unique<ListSetting>(config, rtlGainMap, "Gain", false);
		rtlDeviceGainLS->bindVariable(&config->rtl.gain);

		//225001 - 300000 Hz
		//900001 - 3200000 Hz
		std::map<int, std::string> rtlSampRateMap = {
			{0 , "250000"},
			{1 , "300000"},
			{2 , "1000000"},
			{3 , "1500000"},
			{4 , "2000000"},
			{5 , "2500000"},
			{6 , "3000000"},
			{7 , "3200000"}
		};

		rtlSampRateLS = std::make_unique<ListSetting>(config, rtlSampRateMap, "Sampling rate", false);
		rtlSampRateLS->bindVariable(&config->rtl.deviceSamplingRate);
	}

	//-------------Spectre
	std::map<int, std::string> spectreStyleMap;
	spectreStyleMap.insert(pair<int, string> {0, "0"});
	spectreStyleMap.insert(pair<int, string> {1, "1"});
	spectreStyleMap.insert(pair<int, string> {2, "2"});
	spectreStyleLS = std::make_unique<ListSetting>(config, spectreStyleMap, "Spectre style", true);
	spectreStyleLS->bindVariable(&config->spectre.style);

	std::map<int, std::string> smoothingDepthMap;
	smoothingDepthMap.insert(pair<int, string> {0, "0"});
	smoothingDepthMap.insert(pair<int, string> {1, "1"});
	smoothingDepthMap.insert(pair<int, string> {2, "2"});
	smoothingDepthMap.insert(pair<int, string> {3, "3"});
	smoothingDepthMap.insert(pair<int, string> {4, "4"});
	smoothingDepthMap.insert(pair<int, string> {5, "5"});
	smoothingDepthMap.insert(pair<int, string> {6, "6"});
	smoothingDepthMap.insert(pair<int, string> {7, "7"});
	smoothingDepthMap.insert(pair<int, string> {8, "8"});
	smoothingDepthLS = std::make_unique<ListSetting>(config, smoothingDepthMap, "Smoothing depth", false);
	smoothingDepthLS->bindVariable(&config->spectre.smoothingDepth);
	//--------------------

	//--------------Other-
	std::map<int, std::string> fftLenMap;
	fftLenMap.insert(pair<int, string> {0, "8192"});
	fftLenMap.insert(pair<int, string> {1, "16384"});
	fftLenMap.insert(pair<int, string> {2, "32768"});
	fftLenMap.insert(pair<int, string> {3, "65536"});
	fftLenMap.insert(pair<int, string> {4, "131072"});
	fftLenMap.insert(pair<int, string> {5, "262144"});
	fftLenMap.insert(pair<int, string> {6, "524288"});
	fftLenLS = std::make_unique<ListSetting>(config, fftLenMap, "FFT length", true);
	fftLenLS->bindVariable(&config->delayedFFTLen);

	std::map<int, std::string> waterfallSpeedMap;
	waterfallSpeedMap.insert(pair<int, string> {0, "1"});
	waterfallSpeedMap.insert(pair<int, string> {1, "2"});
	waterfallSpeedMap.insert(pair<int, string> {2, "3"});
	waterfallSpeedLS = std::make_unique<ListSetting>(config, waterfallSpeedMap, "Waterfall speed", false);
	waterfallSpeedLS->bindVariable(&config->waterfall.speed);
	//--------------------

	/*auto end = std::chrono::steady_clock::now();
	auto elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
	std::cout << "The time: " << elapsed_ms.count() << " micros\n";*/
}