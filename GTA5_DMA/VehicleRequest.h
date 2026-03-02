#pragma once

class VehicleRequest
{
public:
	static inline int VehicleSlotId = 0; // 0-606
	static inline bool bRequestPending = false;

	static void RequestVehicle();
	static void Render();

private:
	// FreemodeGeneral base global (from YimMenu Globals.cpp)
	static constexpr DWORD FREEMODE_GENERAL_BASE = 2733138;

	// Offsets within FreemodeGeneral struct
	static constexpr DWORD PERSONAL_VEHICLE_REQUESTED = 575;
	static constexpr DWORD NODE_DISTANCE_CHECK = 590;
	static constexpr DWORD REQUESTED_VEHICLE_ID = 639;
	static constexpr DWORD EXEC1_IMPOUND = 642;
};
