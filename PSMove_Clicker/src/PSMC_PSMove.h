#pragma once

#ifndef PSMC_PSMOVE_H
#define PSMC_PSMOVE_H

#include "PSMoveClient_CAPI/PSMoveClient_CAPI.h"

namespace PSMC {
	// True == down, False == up
	struct PSMoveButtons {
		bool CircleButton;
		bool CrossButton;
		bool MoveButton;
		bool PSButton;
		bool SelectButton;
		bool SquareButton;
		bool StartButton;
		bool TriangleButton;
	};

	struct RGB {
		unsigned char r;
		unsigned char g;
		unsigned char b;
	};

	class PSMove {
	public:
		PSMove(PSMController* controller);
		~PSMove();

		void Process();
	private:
		void UpdateButtonStates();

		void SetColor(unsigned char r, unsigned char g, unsigned char b);
		void UpdateColor();

		PSMoveButtons buttonPrevStates = { 0 };
		PSMVector3f gyroPrevState;
		PSMController* controller = nullptr;
		RGB color = { 0, 0, 0 };

		friend class ControllerManager;
	};
}

#endif // !PSMC_PSMOVE_H
