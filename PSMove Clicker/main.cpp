#include "NadWin/NadWin.hpp"
#include <PSMoveClient_CAPI.h>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <future>
#include <chrono>
#include <thread>

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

PSMControllerList controllerList = { 0 };
std::unordered_map<int, PSMVector3f> speeds;
std::unordered_map<int, PSMController*> controllers;
PSMVector3f VEL = { -4.0f, 0.0f, 0.0f };
int refreshRateVal = 100;
UINT_PTR timerID = 0;
NW::UI::TextBoxMultiline* logTextbox;
int lightTime = 100;

template <typename... ParamTypes>
void setTimeOut(int milliseconds, std::function<void(ParamTypes...)> func, ParamTypes... parames)
{
	std::thread{
	[=]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
		func(parames...);
	}
	}.detach();
};

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

void OnEvent(PSMEventMessage* msg) {
	switch (msg->event_type)
	{
	case PSMEventMessage::PSMEvent_connectedToService: {
		
		break;
	}
	}
}

void InfoTimer(HWND, UINT, UINT_PTR, DWORD)
{
	
	if (PSM_GetIsInitialized())
	{
		std::wstringstream wss;
		for (int i = 0; i < controllerList.count; i++) {
			PSMController* controller = controllers[i];

			std::wstring batteryText;
			if (controller->ControllerState.PSMoveState.BatteryValue == PSMBattery_Charging) batteryText = L"£adowanie...";
			else if (controller->ControllerState.PSMoveState.BatteryValue == PSMBattery_Charged) batteryText = L"Na≥adowana";
			else {
				batteryText = std::to_wstring(controller->ControllerState.PSMoveState.BatteryValue * 20);
				batteryText += L"%";
			}

			wss << L"Kontroler " << i << L": \r\n" << L"Bateria: " << batteryText << L"\r\n" << L"CzÍstotliwoúÊ odúwieøania (razy na sekunde) : " << controller->DataFrameAverageFPS << "\r\n\r\n";

			logTextbox->SetText(wss.str());
		}
	}
}

void MainTimer(HWND, UINT, UINT_PTR, DWORD)
{
	if (PSM_GetIsInitialized())
	{
		PSM_Update();

		// Poll any events from the service
		

		// Poll events queued up by the call to ClientPSMoveAPI::update()
		//PSMMessage message;
		//while (PSM_PollNextMessage(&message, sizeof(message)) == PSMResult_Success)
		//{
		//	switch (message.payload_type)
		//	{
		//	case PSMMessage::_messagePayloadType_Response:
		//		//onClientPSMoveResponse(&message.response_data);
		//		break;
		//	case PSMMessage::_messagePayloadType_Event:
		//		//OnEvent(&message.event_data);
		//		break;
		//	}
		//}

		if (PSM_HasControllerListChanged())
		{
			for (int i = 0; i < controllerList.count; i++) {
				uninitializeController(i);
				controllers[i] = nullptr;
			}

			rebuildControllerList();
			std::cout << "\n" << controllerList.count << "\n";
			for (int i = 0; i < controllerList.count; i++)
			{
				initializeController(i, PSMControllerDataStreamFlags::PSMStreamFlags_defaultStreamOptions | PSMControllerDataStreamFlags::PSMStreamFlags_includeCalibratedSensorData);
				controllers[i] = PSM_GetController(controllerList.controller_id[i]);
			}
		}

		for (int i = 0; i < controllerList.count; i++)
		{
			PSMController* controller = controllers[i];
			PSMPSMoveCalibratedSensorData& calibsens = controller->ControllerState.PSMoveState.CalibratedSensorData;
			
			std::function<void(int)> turnOffCallback = [](int i) {
				PSM_SetControllerLEDOverrideColor(controllerList.controller_id[i], 0, 0, 0);
			};

			auto turnOffLight = [i, turnOffCallback]() {
				if (lightTime == 0) return;
				else setTimeOut<int>(lightTime, turnOffCallback, i);
			};

			if (controller->ControllerState.PSMoveState.TriggerValue > 200)
			{
				INPUT Input = { 0 };
				Input.type = INPUT_MOUSE;

				Input.mi.dx = calibsens.Gyroscope.z * -10.0f;
				Input.mi.dy = calibsens.Gyroscope.x * -12.5f;

				Input.mi.dwFlags = MOUSEEVENTF_MOVE;
				SendInput(1, &Input, sizeof(INPUT));


				break;
			}


			if (calibsens.Gyroscope.x < VEL.x && speeds[i].x > VEL.x) {
				PSM_SetControllerLEDOverrideColor(controllerList.controller_id[i], 255, 30, 255);
				INPUT Inputs[2] = { 0 };
				Inputs[0].type = INPUT_MOUSE;
				Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
				Inputs[1].type = INPUT_MOUSE;
				Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
				SendInput(2, Inputs, sizeof(INPUT));

				turnOffLight();
			}
			if (controller->ControllerState.PSMoveState.CircleButton == PSMButtonState_PRESSED)
			{
				PSM_SetControllerLEDOverrideColor(controllerList.controller_id[i], 30, 255, 255);
				INPUT Inputs[2] = { 0 };
				Inputs[0].type = INPUT_MOUSE;
				Inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
				Inputs[1].type = INPUT_MOUSE;
				Inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
				SendInput(2, Inputs, sizeof(INPUT));

				turnOffLight();
			}
			if (controller->ControllerState.PSMoveState.SelectButton == PSMButtonState_PRESSED)
			{
				PSM_SetControllerLEDOverrideColor(controllerList.controller_id[i], 255, 0, 0);
				keybd_event(VK_ESCAPE, 0, 0, 0);              // press the Esc key
				keybd_event(VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0); // let up the Esc key

				turnOffLight();
			}
			speeds[i] = calibsens.Gyroscope;
		}

		
	}
}

void refreshForceText(NW::UI::TextBoxSingleline* force)
{
	std::wstringstream wss;
	wss << L"Si≥a (" << VEL.x << L")";
	force->SetPlaceholder(wss.str());
}

void refreshRefreshRateText(NW::UI::TextBoxSingleline* refreshRate)
{
	std::wstringstream wss;
	wss << L"Odúwieøanie (" << refreshRateVal << L")";
	refreshRate->SetPlaceholder(wss.str());
}

void refreshLightTimeText(NW::UI::TextBoxSingleline* lightTimeTextbox)
{
	std::wstringstream wss;
	wss << L"Czas migniÍcia (" << lightTime << L")";
	lightTimeTextbox->SetPlaceholder(wss.str());
}

int main()
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
	NW::UI::Button applyForce(&mainWindow, NW::UI::Position(210, 5, 150, 25), L"Potwierdü");
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
	NW::UI::Button refreshRateApply(&mainWindow, NW::UI::Position(210, 35, 150, 25), L"Potwierdü");
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

	NW::UI::TextBoxSingleline lightTimeTextbox(&mainWindow, NW::UI::Position(5, 65, 200, 25), L"");
	refreshLightTimeText(&lightTimeTextbox);
	NW::UI::Button lightTimeApply(&mainWindow, NW::UI::Position(210, 65, 150, 25), L"Potwierdü");
	lightTimeApply.EventHandler = [&](NW::UI::ControlEventTypes eventType, NW::UI::ControlEventInfo* eventInfo) {
		switch (eventType)
		{
		case NW::UI::ControlEventTypes::FromParent_Command:
		{
			try {
				lightTime = std::stoi(lightTimeTextbox.GetText());
			}
			catch (...) {}

			lightTimeTextbox.SetText(L"");

			refreshLightTimeText(&lightTimeTextbox);
			break;
		}
		}
	};

	NW::UI::TextBoxMultiline logLoc(&mainWindow, NW::UI::Position(365, 5, 400, 400), L"");
	logLoc.SetReadOnly(true);
	logTextbox = &logLoc;


	mainWindow.Show();




	PSM_Initialize(PSMOVESERVICE_DEFAULT_ADDRESS, PSMOVESERVICE_DEFAULT_PORT, PSM_DEFAULT_TIMEOUT);
	timerID = SetTimer(nullptr, 0, 1000 / refreshRateVal, MainTimer);
	SetTimer(nullptr, 0, 1000 / 5, InfoTimer);

	rebuildControllerList();
	for (int i = 0; i < controllerList.count; i++)
	{
		initializeController(i, PSMControllerDataStreamFlags::PSMStreamFlags_defaultStreamOptions | PSMControllerDataStreamFlags::PSMStreamFlags_includeCalibratedSensorData);
		controllers[i] = PSM_GetController(controllerList.controller_id[i]);
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