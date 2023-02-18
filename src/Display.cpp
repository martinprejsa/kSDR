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

			//if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) Display::instance->receiver->storeDeltaXPos(Display::instance->mouseX);
		}
	}
}

Display::Display(Config* config, FFTSpectreHandler* fftSH) {
	this->config = config;
	viewModel = new ViewModel(config);
	this->flowingFFTSpectre = new FlowingFFTSpectre(config, viewModel, fftSH);
	spectre = new Spectre(config, viewModel, flowingFFTSpectre);
	//this->hackrf = hackrf;
	//flowingFFTSpectre->zoomIn(30000);
	//flowingFFTSpectre->printCurrentPos();
	//flowingFFTSpectre->zoomIn(1500);
	//flowingFFTSpectre->printCurrentPos();
}

int Display::init() {
			
	glfwInit();

	glfwSetTime(0);

	// Create a GLFWwindow object
	window = glfwCreateWindow(1920, 1080, "kSDR", nullptr, nullptr);
	if (window == nullptr) {
		printf("Failed to create GLFW window\r\n");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	glfwSwapInterval(1);

	//receiver = new ReceiverLogic(config, width);

	glfwSetFramebufferSizeCallback(window, framebufferReSizeCallback);
	glfwSetWindowSizeCallback(window, windowSizeCallback);

	glfwSetCursorPosCallback(window, cursorPositionCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);

	initImGUI();

	glfwSwapBuffers(window);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//glEnable(GL_LINE_SMOOTH);

	glLineWidth(1.5);

	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1;

	//Инициализируем шрифт
}

void Display::mainLoop() {
	// Game loop
	while (!glfwWindowShouldClose(window)) {
		
		// Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//fftSpectreHandler->getSemaphore()->lock();
		renderImGUIFirst();
		//fftSpectreHandler->getSemaphore()->unlock();
		//drawScene();

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
	viewModel->fontMyRegular = io.Fonts->AddFontFromFileTTF("DroidSansMono.ttf", 18);
	viewModel->fontBigRegular = io.Fonts->AddFontFromFileTTF("DroidSansMono.ttf", 42);

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void Display::renderImGUIFirst() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//Проверка которая исключает настройку приемника при перетягивании стороннего окна в область окна спектра
	if (ImGui::IsMouseDown(0)) {
		if (!spectre->isMouseOnSpectreRegion(
			spectre->windowFrame.UPPER_RIGHT.x,
			spectre->windowFrame.UPPER_RIGHT.y,
			spectre->windowFrame.BOTTOM_LEFT.x,
			spectre->windowFrame.BOTTOM_LEFT.y)) viewModel->mouseBusy = true;
	} else {
		viewModel->mouseBusy = false;
	}

	ImGui::Begin(APP_NAME);

		ImGui::SliderInt("Frequency", &viewModel->centerFrequency, 1000000, 30000000);

		ImGui::SliderInt("Filter width", &viewModel->filterWidth, 0, 12000);

		ImGui::SliderFloat("Waterfall min", &viewModel->waterfallMin, -130, 0); ImGui::SameLine();
		if (ImGui::Button("Waterfall Auto")) {
			spectre->waterfallAutoColorCorrection();
		}
		ImGui::SliderFloat("Waterfall max", &viewModel->waterfallMax, -130, 0);

		ImGui::SliderFloat("Spectre ratio", &viewModel->ratio, -150, 0); ImGui::SameLine();
		if (ImGui::Button("Spectre Auto")) {
			spectre->spectreRatioAutoCorrection();
		}
		ImGui::SliderFloat("Spectre min val", &viewModel->minDb, -150, 0);

		ImGui::SliderInt("Spectre speed", &viewModel->spectreSpeed, 1, 200);
		
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	ImGui::End();

	spectre->draw();

	smeter->draw(viewModel->signalMaxdB);

	if (config->deviceType == Config::HACKRF) {
		ImGui::Begin("HackRF options");
			Hackrf* hackrf = (Hackrf*)config->device;

			hackrf->setFreq((uint64_t)viewModel->centerFrequency);

			ImGui::SliderInt("LNA Gain", &viewModel->hackRFModel.lnaGain, 0, 5);
			hackrf->setLnaGain((uint32_t)(viewModel->hackRFModel.lnaGain * 8));

			ImGui::SliderInt("VGA Gain", &viewModel->hackRFModel.vgaGain, 0, 31);
			hackrf->setVgaGain((uint32_t)(viewModel->hackRFModel.vgaGain * 2));

			ImGui::SliderInt("AMP Gain", &viewModel->hackRFModel.enableAmp, 0, 1);
			//ImGui::Checkbox("AMP", &viewModel->enableAmp);
			hackrf->enableAmp((uint8_t)viewModel->hackRFModel.enableAmp);

			const char* items[] = { "1750000", "2500000", "3500000", "5000000", "5500000", "6000000", "7000000", "8000000", "9000000", "10000000", "20000000" };
			static int item_current_idx = 0; // Here we store our selection data as an index.
			const char* combo_preview_value = items[item_current_idx];  // Pass in the preview value visible before opening the combo (it could be anything)
			if (ImGui::BeginCombo("Filter", combo_preview_value, 0)) {
				for (int n = 0; n < IM_ARRAYSIZE(items); n++)
				{
					const bool is_selected = (item_current_idx == n);
					if (ImGui::Selectable(items[n], is_selected)) {
						item_current_idx = n;
						uint32_t baseband;
						hackrf->parse_u32((char*)items[n], &baseband);
						hackrf->setBaseband(baseband);
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		ImGui::End();
	}

	if (config->deviceType == Config::RSP) {
		ImGui::Begin("RSP options");
			ImGui::SliderInt("Gain", &viewModel->rspModel.gain, 0, 59);
			ImGui::Checkbox("Disable LNA", &viewModel->rspModel.lna);
			//ImGui::Checkbox("Gain Control", &viewModel->gainControl);
		ImGui::End();
	}

	ImGui::Begin("DATA");
		ImGui::Text("Sampling rate: %d", config->inputSamplerate);
		ImGui::Text("FFT length: %d", config->fftLen);
		ImGui::Text("AMP: %.2f", viewModel->amp);
		ImGui::Text("CPU usage: %.1f", cpu.getCurrentValue());
		ImGui::Text("Buffer available: %.2f sec", viewModel->bufferAvailable);
		ImGui::Text("Service field1: %f", viewModel->serviceField1);
		ImGui::Text("Service field2: %f", viewModel->serviceField2);
	ImGui::End();

	ImGui::Begin("MODE");
		if (ImGui::Button("160m")) {
			viewModel->centerFrequency = 1900000;
		}
		ImGui::SameLine();
		if (ImGui::Button("80m")) {
			viewModel->centerFrequency = 3700000;
		}
		ImGui::SameLine();
		if (ImGui::Button("40m")) {
			viewModel->centerFrequency = 7100000;
		}
		ImGui::SameLine();
		if (ImGui::Button("30m")) {
			viewModel->centerFrequency = 10325000;
		}
		ImGui::SameLine();
		if (ImGui::Button("20m")) {
			viewModel->centerFrequency = 14150000;
		}

		if (ImGui::Button("17m")) {
			viewModel->centerFrequency = 18100000;
		}
		ImGui::SameLine();
		if (ImGui::Button("15m")) {
			viewModel->centerFrequency = 21100000;
		}
		ImGui::SameLine();
		if (ImGui::Button("12m")) {
			viewModel->centerFrequency = 24900000;
		}
		ImGui::SameLine();
		if (ImGui::Button("10m")) {
			viewModel->centerFrequency = 28500000;
		}

		ImGui::RadioButton("USB", &viewModel->receiverMode, USB); ImGui::SameLine();
		ImGui::RadioButton("LSB", &viewModel->receiverMode, LSB); ImGui::SameLine();
		ImGui::RadioButton("AM", &viewModel->receiverMode, AM);

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
		ImGui::SliderFloat("Volume", &viewModel->volume, 0, 5);

		if (ImGui::Button("+")) {
			flowingFFTSpectre->zoomIn(100);
			spectre->receiverLogicNew->setFrequencyDelta(spectre->receiverLogicNew->getFrequencyDelta());
		} ImGui::SameLine();
		if (ImGui::Button("-")) {
			flowingFFTSpectre->zoomOut(100);
			spectre->receiverLogicNew->setFrequencyDelta(spectre->receiverLogicNew->getFrequencyDelta());
		} ImGui::SameLine();
		if (ImGui::Button("<-")) {
			flowingFFTSpectre->move(-100);
			spectre->receiverLogicNew->setFrequencyDelta(spectre->receiverLogicNew->getFrequencyDelta());
		} ImGui::SameLine();
		if (ImGui::Button("->")) {
			flowingFFTSpectre->move(100);
			spectre->receiverLogicNew->setFrequencyDelta(spectre->receiverLogicNew->getFrequencyDelta());
		}

	ImGui::End();
}