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
NW::UI::TextBoxMultiline* logTextbox;

long long getTimestampMilis()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void tryConnect()
{
	bool success = false;
	while (!success)
	{
		if (PSM_Initialize(PSMOVESERVICE_DEFAULT_ADDRESS, PSMOVESERVICE_DEFAULT_PORT, PSM_DEFAULT_TIMEOUT) != PSMResult_Success)
		{
			MessageBoxW(nullptr, L"Nie uda³o siê po³¹czyæ z serwerem,\r\nPonowna próba za 5 sekund", L"B³¹d", MB_OK | MB_ICONERROR);
			time_t start = getTimestampMilis();
			while (getTimestampMilis() - start < 5000)
			{
				NW::UI::App::DoEvents();
				Sleep(1);
			}
			continue;
		}
		success = true;
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

void InfoTimer(HWND, UINT, UINT_PTR, DWORD)
{
	std::wstringstream wss;
	wss << L"Po³¹czono: " << (PSM_GetIsConnected() ? "Tak" : "Nie") << "\r\n\r\n";
	if (PSM_GetIsConnected())
	{
		for (int i = 0; i < controllerList.count; i++) {
			PSMController* controller = controllers[i].controller;

			std::wstring batteryText;
			if (controller->ControllerState.PSMoveState.BatteryValue == PSMBattery_Charging) batteryText = L"£adowanie...";
			else if (controller->ControllerState.PSMoveState.BatteryValue == PSMBattery_Charged) batteryText = L"Na³adowana";
			else {
				batteryText = std::to_wstring(controller->ControllerState.PSMoveState.BatteryValue * 20);
				batteryText += L"%";
			}

			wss << L"Kontroler " << i << L": \r\n" << L"Bateria: " << batteryText << L"\r\n" << L"Czêstotliwoœæ odœwie¿ania (razy na sekunde) : " << controller->DataFrameAverageFPS << "\r\n\r\n";
		}
	}

	logTextbox->SetText(wss.str());
}

void setControllerColor(int id, unsigned char r, unsigned char g, unsigned char b)
{
	controllers[id].color = { r, g, b };
}

void MainTimer(HWND, UINT, UINT_PTR, DWORD)
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
				INPUT Inputs[2] = { 0 };
				Inputs[0].type = INPUT_MOUSE;
				Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
				Inputs[1].type = INPUT_MOUSE;
				Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
				SendInput(2, Inputs, sizeof(INPUT));
			}

			speeds[i] = calibsens.Gyroscope;
			const unsigned char minus = 2047.0f / refreshRateVal;
			controllerInf.color.r = static_cast<unsigned char>(controllerInf.color.r - minus) > controllerInf.color.r ? 0 : static_cast<unsigned char>(controllerInf.color.r - minus);
			controllerInf.color.g = static_cast<unsigned char>(controllerInf.color.g - minus) > controllerInf.color.g ? 0 : static_cast<unsigned char>(controllerInf.color.g - minus);
			controllerInf.color.b = static_cast<unsigned char>(controllerInf.color.b - minus) > controllerInf.color.b ? 0 : static_cast<unsigned char>(controllerInf.color.b - minus);
		}
	}
	if (!PSM_GetIsConnected()) PostQuitMessage(0);
}

void refreshForceText(NW::UI::TextBoxSingleline* force)
{
	std::wstringstream wss;
	wss << L"Si³a (" << VEL.x << L")";
	force->SetPlaceholder(wss.str());
}

void refreshRefreshRateText(NW::UI::TextBoxSingleline* refreshRate)
{
	std::wstringstream wss;
	wss << L"Odœwie¿anie (" << refreshRateVal << L")";
	refreshRate->SetPlaceholder(wss.str());
}

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	NW::UI::App app(L"PSMove Clicker");
	NW::UI::Window mainWindow(L"PSMove Clicker", CW_USEDEFAULT, CW_USEDEFAULT, 800, 500);

	mainWindow.EventHandler = [&](NW::UI::WindowEventTypes eventType, NW::UI::WindowEventInfo* eventInfo) {
		switch (eventType)
		{
		case NW::UI::WindowEventTypes::Destroy:
			PostQuitMessage(0);
			eventInfo->OverrideProcResult(0);
			break;
		}
	};

	NW::UI::TextBoxSingleline force(&mainWindow, NW::UI::Position(5, 5, 200, 25), L"");
	refreshForceText(&force);
	NW::UI::Button applyForce(&mainWindow, NW::UI::Position(210, 5, 150, 25), L"PotwierdŸ");
	applyForce.EventHandler = [&](NW::UI::ControlEventTypes eventType, NW::UI::ControlEventInfo* eventInfo) {
		switch (eventType)
		{
		case NW::UI::ControlEventTypes::FromParent_Command:
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


	NW::UI::TextBoxSingleline refreshRate(&mainWindow, NW::UI::Position(5, 35, 200, 25), L"");
	refreshRefreshRateText(&refreshRate);
	NW::UI::Button refreshRateApply(&mainWindow, NW::UI::Position(210, 35, 150, 25), L"PotwierdŸ");
	refreshRateApply.EventHandler = [&](NW::UI::ControlEventTypes eventType, NW::UI::ControlEventInfo* eventInfo) {
		switch (eventType)
		{
		case NW::UI::ControlEventTypes::FromParent_Command:
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





	NW::UI::TextBoxMultiline logLoc(&mainWindow, NW::UI::Position(365, 5, 400, 400), L"");
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