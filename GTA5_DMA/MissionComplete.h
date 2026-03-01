#pragma once
#include "core/DMAScriptHelper.h"

// Heist instant finish via script local manipulation.
// Writes mission state variables to force "mission passed" for each heist type.
// Requires player to be mission host (always true in solo play).

class MissionComplete
{
public:
	enum HeistType
	{
		Auto,
		Cayo,
		Casino,
		Apartment,
		ApartmentPacific,
		Doomsday,
		DoomsdayAct3,
		COUNT
	};

	static void InstantFinish(HeistType type);

	static inline std::string LastResult;
	static inline int SelectedHeist = 0; // index into HeistType

	static constexpr const char* HeistNames[] = {
		"Auto-Detect",
		"Cayo Perico",
		"Diamond Casino",
		"Apartment Heist",
		"Apartment (Pacific Standard)",
		"Doomsday Heist",
		"Doomsday Act III"
	};

private:
	static bool FinishCayo();
	static bool FinishCasino();
	static bool FinishApartment();
	static bool FinishApartmentPacific();
	static bool FinishDoomsday();
	static bool FinishDoomsdayAct3();
};
