#pragma once

#ifndef PSMC_CONTROLLERMANAGER_H
#define PSMC_CONTROLLERMANAGER_H

#include "PSMoveClient_CAPI/PSMoveClient_CAPI.h"
#include "PSMC_PSMove.h"
#include <vector>
#include <sstream>

namespace PSMC {
	class ControllerManager {
	public:
		ControllerManager();
		~ControllerManager();

		void Process();
		static std::string GetControllerInfoString();

		static bool ADOFAI_Mode;
		static PSMControllerID mainPadID;
		static float force;
	private:
		void RebuildControllerList();

		void InitializeControllers();
		void UninitializeControllers();

		static PSMControllerList controllerList;
		static std::vector<PSMove*> controllers;

		friend class PSMove;
	};
}

#endif // !PSMC_CONTROLLERMANAGER_H
