#include "pch.h"
#include "VehicleRequest.h"

void VehicleRequest::RequestVehicle()
{
	DWORD base = FREEMODE_GENERAL_BASE;

	// Set the vehicle slot ID
	DMA::SetGlobalInt(base + REQUESTED_VEHICLE_ID, static_cast<DWORD>(VehicleSlotId));

	// Set distance check to 0 (spawn nearby)
	DMA::SetGlobalInt(base + NODE_DISTANCE_CHECK, 0);

	// Clear impound flag
	DMA::SetGlobalInt(base + EXEC1_IMPOUND, 0);

	// Trigger the request
	DMA::SetGlobalInt(base + PERSONAL_VEHICLE_REQUESTED, 1);

	std::println("[VehicleRequest] Requested vehicle slot {}", VehicleSlotId);
}

void VehicleRequest::Render()
{
	if (ImGui::CollapsingHeader("Personal Vehicle Request"))
	{
		ImGui::SliderInt("Vehicle Slot ID", &VehicleSlotId, 0, 606);

		if (ImGui::Button("Request Last Personal Vehicle"))
		{
			VehicleSlotId = 0;
			RequestVehicle();
		}

		ImGui::SameLine();

		if (ImGui::Button("Request Vehicle"))
		{
			RequestVehicle();
		}
	}
}
