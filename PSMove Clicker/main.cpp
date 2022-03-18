#include "NadWin/NadWin.hpp"
#include <PSMoveClient_CAPI.h>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <chrono>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "PSMoveClient_CAPI.lib")

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

struct RGB {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

struct controllerInfo {
	PSMController* controller;
	RGB color;
};

PSMControllerList controllerList = { 0 };
std::unordered_map<int, PSMVector3f> speeds;
std::unordered_map<int, controllerInfo> controllers;
PSMVector3f VEL = { -4.0f, 0.0f, 0.0f };
int refreshRateVal = 100;
UINT_PTR timerID = 0;
NW::TextBoxMultiline* logTextbox;
int mainPadIndex = 0;
bool adofaiMode = false;

long long getTimestampMilis()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void tryConnect()
{
	if (PSM_Initialize(PSMOVESERVICE_DEFAULT_ADDRESS, PSMOVESERVICE_DEFAULT_PORT, PSM_DEFAULT_TIMEOUT) != PSMResult_Success)
	{
		return;
	}
}

void rebuildControllerList()
{
	memset(&controllerList, 0, sizeof(PSMControllerList));
	PSM_GetControllerList(&controllerList, PSM_DEFAULT_TIMEOUT);
}

void initializeController(int id, unsigned int data_stream_flags = PSMControllerDataStreamFlags::PSMStreamFlags_defaultStreamOptions)
{
	PSM_AllocateControllerListener(controllerList.controller_id[id]);
	PSM_StartControllerDataStream(controllerList.controller_id[id], data_stream_flags, PSM_DEFAULT_TIMEOUT);
}

void uninitializeController(int id)
{
	PSM_StopControllerDataStream(controllerList.controller_id[id], PSM_DEFAULT_TIMEOUT);
	PSM_FreeControllerListener(controllerList.controller_id[id]);
}

void CALLBACK InfoTimer(HWND, UINT, UINT_PTR, DWORD)
{
	std::wstringstream wss;
	wss << L"Connected: " << (PSM_GetIsConnected() ? "Yes" : "No") << "\r\n\r\n";
	if (PSM_GetIsConnected())
	{
		for (int i = 0; i < controllerList.count; i++) {
			PSMController* controller = controllers[i].controller;

			std::wstring batteryText;
			if (controller->ControllerState.PSMoveState.BatteryValue == PSMBattery_Charging) batteryText = L"Charging...";
			else if (controller->ControllerState.PSMoveState.BatteryValue == PSMBattery_Charged) batteryText = L"Charged";
			else {
				batteryText = std::to_wstring(controller->ControllerState.PSMoveState.BatteryValue * 20);
				batteryText += L"%";
			}

			wss << L"Controller " << i << L": \r\n" << L"Battery: " << batteryText << L"\r\n" << L"Refresh rate (times per second) : " << controller->DataFrameAverageFPS << "\r\n\r\n";
		}
	}

	logTextbox->SetText(wss.str());
}

void setControllerColor(int id, unsigned char r, unsigned char g, unsigned char b)
{
	controllers[id].color = { r, g, b };
}

void CALLBACK MainTimer(HWND, UINT, UINT_PTR, DWORD)
{
	if (PSM_GetIsConnected())
	{
		PSM_Update();

		// Poll any events from the service

		if (PSM_HasControllerListChanged())
		{
			for (int i = 0; i < controllerList.count; i++) {
				uninitializeController(i);
				controllers.erase(i);
			}

			rebuildControllerList();
			std::cout << "\n" << controllerList.count << "\n";
			for (int i = 0; i < controllerList.count; i++)
			{
				initializeController(i, PSMControllerDataStreamFlags::PSMStreamFlags_defaultStreamOptions | PSMControllerDataStreamFlags::PSMStreamFlags_includeCalibratedSensorData);
				controllers[i] = { nullptr, {0, 0, 0} };
				controllers[i].controller = PSM_GetController(controllerList.controller_id[i]);
			}
		}

		for (int i = 0; i < controllerList.count; i++)
		{
			controllerInfo& controllerInf = controllers[i];
			PSMController* controller = controllerInf.controller;
			PSMPSMoveCalibratedSensorData& calibsens = controller->ControllerState.PSMoveState.CalibratedSensorData;
			
			PSM_SetControllerLEDOverrideColor(controllerList.controller_id[i], controllerInf.color.r, controllerInf.color.g, controllerInf.color.b);


			if (controller->ControllerState.PSMoveState.CircleButton == PSMButtonState_PRESSED)
			{
				setControllerColor(i, 0, 0, 255);
				INPUT Inputs[2] = { 0 };
				Inputs[0].type = INPUT_MOUSE;
				Inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
				Inputs[1].type = INPUT_MOUSE;
				Inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
				SendInput(2, Inputs, sizeof(INPUT));
			}

			if (controller->ControllerState.PSMoveState.SelectButton == PSMButtonState_PRESSED)
			{
				setControllerColor(i, 255, 0, 0);
				keybd_event(VK_ESCAPE, 0, 0, 0);              // press the Esc key
				keybd_event(VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0); // let up the Esc key
			}

			if (adofaiMode && controller->ControllerState.PSMoveState.PSButton == PSMButtonState_DOWN)
			{
				mainPadIndex = i;
			}

			if (controller->ControllerState.PSMoveState.TriggerValue > 200)
			{
				setControllerColor(i, 0, 255, 0);

				INPUT Input = { 0 };
				Input.type = INPUT_MOUSE;

				Input.mi.dx = calibsens.Gyroscope.z * -10.0f;
				Input.mi.dy = calibsens.Gyroscope.x * -12.5f;

				Input.mi.dwFlags = MOUSEEVENTF_MOVE;
				SendInput(1, &Input, sizeof(INPUT));

				break;
			}

			if (calibsens.Gyroscope.x < VEL.x && speeds[i].x > VEL.x) {
				setControllerColor(i, 255, 255, 255);
				if (mainPadIndex == i || !adofaiMode)
				{
					INPUT Inputs[2] = { 0 };
					Inputs[0].type = INPUT_MOUSE;
					Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					Inputs[1].type = INPUT_MOUSE;
					Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
					SendInput(2, Inputs, sizeof(INPUT));
				}
				else {
					keybd_event(0x41 + i, 0, 0, 0);
					keybd_event(0x41 + i, 0, KEYEVENTF_KEYUP, 0);
				}
			}

			speeds[i] = calibsens.Gyroscope;
			const unsigned char minus = 1500.0f / refreshRateVal;
			const unsigned char brightness = adofaiMode ? (i == mainPadIndex ? 5 : 0) : 0;

			const unsigned char r = static_cast<unsigned char>(controllerInf.color.r - minus) > controllerInf.color.r ? 0 : static_cast<unsigned char>(controllerInf.color.r - minus);
			const unsigned char g = static_cast<unsigned char>(controllerInf.color.g - minus) > controllerInf.color.g ? 0 : static_cast<unsigned char>(controllerInf.color.g - minus);
			const unsigned char b = static_cast<unsigned char>(controllerInf.color.b - minus) > controllerInf.color.b ? 0 : static_cast<unsigned char>(controllerInf.color.b - minus);

			controllerInf.color.r = r < brightness ? brightness : r;
			controllerInf.color.g = g < brightness ? brightness : g;
			controllerInf.color.b = b < brightness ? brightness : b;

		}
	}
	if (!PSM_GetIsConnected()) PostQuitMessage(0);
}

void refreshForceText(NW::TextBoxSingleline* force)
{
	std::wstringstream wss;
	wss << L"Force (" << VEL.x << L")";
	force->SetPlaceholder(wss.str());
}

void refreshRefreshRateText(NW::TextBoxSingleline* refreshRate)
{
	std::wstringstream wss;
	wss << L"Refresh rate (" << refreshRateVal << L")";
	refreshRate->SetPlaceholder(wss.str());
}

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	NW::App app(L"PSMove Clicker");
	NW::Window mainWindow(L"PSMove Clicker", CW_USEDEFAULT, CW_USEDEFAULT, 800, 500);

	NW::Font font(19, L"Segoe UI");
	mainWindow.SetDefaultFont(&font);

	mainWindow.EventHandler = [&](NW::WindowEventTypes eventType, NW::WindowEventInfo* eventInfo) {
		switch (eventType)
		{
		case NW::WindowEventTypes::Destroy:
			PostQuitMessage(0);
			eventInfo->OverrideProcResult(0);
			break;
		}
	};

	NW::TextBoxSingleline force(&mainWindow, NW::Position(5, 5, 200, 25), L"");
	refreshForceText(&force);
	NW::Button applyForce(&mainWindow, NW::Position(210, 5, 150, 25), L"Apply");
	applyForce.EventHandler = [&](NW::ControlEventTypes eventType, NW::ControlEventInfo* eventInfo) {
		switch (eventType)
		{
		case NW::ControlEventTypes::FromParent_Command:
		{
			try {
				VEL.x = std::stof(force.GetText());
			}
			catch (...) {}

			force.SetText(L"");

			refreshForceText(&force);
			break;
		}
		}

	};


	NW::TextBoxSingleline refreshRate(&mainWindow, NW::Position(5, 35, 200, 25), L"");
	refreshRefreshRateText(&refreshRate);
	NW::Button refreshRateApply(&mainWindow, NW::Position(210, 35, 150, 25), L"Apply");
	refreshRateApply.EventHandler = [&](NW::ControlEventTypes eventType, NW::ControlEventInfo* eventInfo) {
		switch (eventType)
		{
		case NW::ControlEventTypes::FromParent_Command:
		{
			try {
				refreshRateVal = std::stoi(refreshRate.GetText());
			}
			catch (...) {}

			refreshRate.SetText(L"");

			refreshRefreshRateText(&refreshRate);

			KillTimer(nullptr, timerID);
			timerID = SetTimer(nullptr, 0, 1000 / refreshRateVal, MainTimer);
			break;
		}
		}
	};

	NW::CheckBox adofaiModeCheckbox(&mainWindow, NW::Position(5, 65, 360, 25), L"ADOFAI mode");
	adofaiModeCheckbox.EventHandler = [&](NW::ControlEventTypes eventType, NW::ControlEventInfo* eventInfo) {
		switch (eventType)
		{
		case NW::ControlEventTypes::FromParent_Command:
			adofaiMode = adofaiModeCheckbox.GetChecked();
			break;
		}
	};


	NW::Font logFont(14, L"Segoe UI");
	NW::TextBoxMultiline logLoc(&mainWindow, NW::Position(365, 5, 400, 200), L"");
	logLoc.SetFont(&logFont);
	logLoc.SetReadOnly(true);
	logTextbox = &logLoc;
	mainWindow.Show();
	SetTimer(nullptr, 0, 1000 / 5, InfoTimer);



	tryConnect();



	timerID = SetTimer(nullptr, 0, 1000 / refreshRateVal, MainTimer);

	rebuildControllerList();
	for (int i = 0; i < controllerList.count; i++)
	{
		initializeController(i, PSMControllerDataStreamFlags::PSMStreamFlags_defaultStreamOptions | PSMControllerDataStreamFlags::PSMStreamFlags_includeCalibratedSensorData);
		controllers[i] = { nullptr, {0, 0, 0} };
		controllers[i].controller = PSM_GetController(controllerList.controller_id[i]);
	}

	WPARAM result = app.MessageLoop();
	for (int i = 0; i < controllerList.count; i++)
	{
		PSM_SetControllerRumble(controllerList.controller_id[i], PSMControllerRumbleChannel_All, 0.0f);
		uninitializeController(i);
	}
	PSM_Shutdown();
	return result;
}