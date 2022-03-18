#pragma once

#ifndef PSMC_CONTROLLERMANAGER_CPP
#define PSMC_CONTROLLERMANAGER_CPP

#include "PSMC_ControllerManager.h"

namespace PSMC {
	//
	// public:
	//

	ControllerManager::ControllerManager()
	{
		if (PSM_Initialize(PSMOVESERVICE_DEFAULT_ADDRESS, PSMOVESERVICE_DEFAULT_PORT, PSM_DEFAULT_TIMEOUT) != PSMResult_Success)
		{
			throw std::runtime_error("Failed to initialize client");
		}
		RebuildControllerList();
		InitializeControllers();
	}

	ControllerManager::~ControllerManager()
	{
		UninitializeControllers();
	}

	void ControllerManager::Process()
	{
		if (PSM_GetIsConnected())
		{
			PSM_Update();
			
			if (PSM_HasControllerListChanged())
			{
				UninitializeControllers();
				RebuildControllerList();
				InitializeControllers();
			}

			for (int i = 0; i < controllers.size(); i++)
			{
				controllers.at(i)->Process();
			}
		}
	}

	std::string ControllerManager::GetControllerInfoString()
	{
		std::stringstream ss;
		for (const auto& controller : controllers)
		{
			ss << "Controller " << controller->controller->ControllerID << ":\n";

			std::string batteryText;
			if (controller->controller->ControllerState.PSMoveState.BatteryValue == PSMBattery_Charging) batteryText = "Charging...";
			else if (controller->controller->ControllerState.PSMoveState.BatteryValue == PSMBattery_Charged) batteryText = "Charged";
			else {
				batteryText = std::to_string(controller->controller->ControllerState.PSMoveState.BatteryValue * 20);
				batteryText += "%";
			}

			ss << "Battery: " << batteryText << "\n";
			ss << "Refresh rate (times per second): " << controller->controller->DataFrameAverageFPS << "\n\n";
		}
		
		return ss.str();
	}

	bool ControllerManager::ADOFAI_Mode = false;
	PSMControllerID ControllerManager::mainPadID = 0;
	float ControllerManager::force = -4.0f;

	//
	// private:
	//

	void ControllerManager::RebuildControllerList()
	{
		memset(&controllerList, 0, sizeof(PSMControllerList));
		PSM_GetControllerList(&controllerList, PSM_DEFAULT_TIMEOUT);
	}

	void ControllerManager::InitializeControllers()
	{
		controllers.resize(controllerList.count);
		for (int i = 0; i < controllerList.count; i++)
		{
			PSM_AllocateControllerListener(controllerList.controller_id[i]);
			PSM_StartControllerDataStream(
				controllerList.controller_id[i],
				PSMControllerDataStreamFlags::PSMStreamFlags_defaultStreamOptions | PSMControllerDataStreamFlags::PSMStreamFlags_includeCalibratedSensorData,
				PSM_DEFAULT_TIMEOUT
			);

			controllers[i] = new PSMove(PSM_GetController(controllerList.controller_id[i]));
		}
	}

	void ControllerManager::UninitializeControllers()
	{
		controllers.clear();
		for (int i = 0; i < controllerList.count; i++)
		{
			PSM_StopControllerDataStream(controllerList.controller_id[i], PSM_DEFAULT_TIMEOUT);
			PSM_FreeControllerListener(controllerList.controller_id[i]);
		}
	}

	PSMControllerList ControllerManager::controllerList = { 0 };
	std::vector<PSMove*> ControllerManager::controllers;
}

#endif // !PSMC_CONTROLLERMANAGER_CPP
