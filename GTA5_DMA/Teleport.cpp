#include "pch.h"

#include "Teleport.h"
#include "Heightmap.h"

#include "Locations.h"

#include "offsets/Offsets.h"

bool Teleport::UpdatePlayerStartingLocation()
{
	puts(__FUNCTION__);

	uintptr_t LocationAddress = DMA::NavigationAddress + offsetof(CNavigation, Position);

	DWORD BytesRead = 0;

	VMMDLL_MemReadEx(DMA::vmh, DMA::PID, LocationAddress, (BYTE*)&StartingLocation, sizeof(Vec3), &BytesRead, VMMDLL_FLAG_NOCACHE);

	if (BytesRead != sizeof(Vec3))
	{
		ZeroMemory(&StartingLocation, sizeof(Vec3));
		return 0;
	}

	return 1;
}

void Teleport::OverwriteLocation(Vec3 Location)
{
	/* save starting location for later comparison */
	UpdatePlayerStartingLocation();

	BYTE InVehicleBits = 0x0;
	uintptr_t InVehicleAddress = DMA::LocalPlayerAddress + offsetof(PED, InVehicleBits);
	VMMDLL_MemRead(DMA::vmh, DMA::PID, InVehicleAddress, (BYTE*)&InVehicleBits, sizeof(BYTE));

	VMMDLL_SCATTER_HANDLE vmsh = VMMDLL_Scatter_Initialize(DMA::vmh, DMA::PID, VMMDLL_FLAG_NOCACHE);

	PreparePlayerWrites(vmsh);

	if (InVehicleBits & 0x1)
		PrepareVehicleWrites(vmsh);

	bool bSuccessfullyTeleported = false;
	for (int i = 0; i < 200; i++)
	{
		VMMDLL_Scatter_Execute(vmsh);

		Sleep(10);

		if (!DMA::UpdatePlayerCurrentLocation())
			continue;

		// Use 2D distance (XY only) -- Z is unreliable for waypoint teleports
		// where we estimate altitude. Player may be falling from high Z.
		float dist2D = DMA::LocalPlayerLocation.Distance2D(Location);
		float fromStart2D = DMA::LocalPlayerLocation.Distance2D(StartingLocation);

		if (dist2D > 1.0f
			&& dist2D < 100.0f
			&& fromStart2D > 50.0f)
		{
			std::println("[Teleport] Success (2D dist from target: {:.1f}, from start: {:.1f})", dist2D, fromStart2D);
			bSuccessfullyTeleported = true;
			break;
		}
	}

	VMMDLL_Scatter_CloseHandle(vmsh);

	if (!bSuccessfullyTeleported)
		puts("Teleport failed.");
}

bool Teleport::OnDMAFrame()
{
	if (bRequestedTeleport)
	{
		OverwriteLocation(DesiredLocation);
		bRequestedTeleport = false;
	}

	return 1;
}

static const std::vector<std::string>CayoSecondaryLocationStrings = { "Coke Field","NorthDock1","NorthDock2","NorthDock3","Middle","Main1","Main2","Main3","Hangar1","Hangar2","Spawn" };
static const std::vector<std::string>GeneralLocationStrings = { "Military Base","Sandy Shores","Casino","Paleto Bay","Airport"};


bool Teleport::Render()
{

	ImGui::Begin("Teleport", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("Current Location: %.2f, %.2f, %.2f", DMA::LocalPlayerLocation.x, DMA::LocalPlayerLocation.y, DMA::LocalPlayerLocation.z);

	ImGui::InputFloat("X", &Teleport::DesiredLocation.x);
	ImGui::InputFloat("Y", &Teleport::DesiredLocation.y);
	ImGui::InputFloat("Z", &Teleport::DesiredLocation.z);

	if (ImGui::Button("Teleport Player"))
	{
		bRequestedTeleport = true;
	}

	if (ImGui::Button("Teleport To Waypoint"))
	{
		Vec3 WaypointCoords = GetWaypointCoords();
		if (WaypointCoords.x != 0.0f)
		{
			DesiredLocation = WaypointCoords;
			bRequestedTeleport = true;
		}
	}

	if (ImGui::CollapsingHeader("General TPs"))
	{
		for (auto Name : GeneralLocationStrings)
		{
			if (ImGui::Button(Name.c_str()))
			{
				DesiredLocation = LocationMap[Name];
				bRequestedTeleport = true;
			}
		}
	}

	if (ImGui::CollapsingHeader("Business Locations"))
	{
		if (ImGui::Button("Clubhouse"))
		{
			DesiredLocation = LocationMap["Clubhouse"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Coke"))
		{
			DesiredLocation = LocationMap["Coke"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Fake Cash"))
		{
			DesiredLocation = LocationMap["Fake Cash"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Meth"))
		{
			DesiredLocation = LocationMap["Meth"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Weed"))
		{
			DesiredLocation = LocationMap["Weed"];
			bRequestedTeleport = true;
		}

		//Next line
		if (ImGui::Button("Bunker"))
		{
			DesiredLocation = LocationMap["Bunker"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Nightclub"))
		{
			DesiredLocation = LocationMap["Nightclub"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Arcade"))
		{
			DesiredLocation = LocationMap["Arcade"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Auto Shop"))
		{
			DesiredLocation = LocationMap["Auto Shop"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Agency"))
		{
			DesiredLocation = LocationMap["Agency"];
			bRequestedTeleport = true;
		}

		//Next line
		if (ImGui::Button("Office"))
		{
			DesiredLocation = LocationMap["Office"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Vehicle Warehouse"))
		{
			DesiredLocation = LocationMap["Vehicle Warehouse"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Hanger"))
		{
			DesiredLocation = LocationMap["Hanger"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Warehouse 1"))
		{
			DesiredLocation = LocationMap["Warehouse 1"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Warehouse 2"))
		{
			DesiredLocation = LocationMap["Warehouse 2"];
			bRequestedTeleport = true;
		}

		//Next line
		if (ImGui::Button("Doomsday"))
		{
			DesiredLocation = LocationMap["Doomsday"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Salvage"))
		{
			DesiredLocation = LocationMap["Salvage"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Doc Forgery"))
		{
			DesiredLocation = LocationMap["Doc Forgery"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Bail Office"))
		{
			DesiredLocation = LocationMap["Bail Office"];
			bRequestedTeleport = true;
		}
		
	}

	if (ImGui::CollapsingHeader("Friend Businesses"))
	{
		if (ImGui::Button("Clubhouse2"))
		{
			DesiredLocation = LocationMap["Clubhouse2"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Coke2"))
		{
			DesiredLocation = LocationMap["Coke2"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Fake Cash2"))
		{
			DesiredLocation = LocationMap["Fake Cash2"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Meth2"))
		{
			DesiredLocation = LocationMap["Meth2"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Weed2"))
		{
			DesiredLocation = LocationMap["Weed2"];
			bRequestedTeleport = true;
		}

		//Next line
		if (ImGui::Button("Bunker2"))
		{
			DesiredLocation = LocationMap["Bunker2"];
			bRequestedTeleport = true;
		}	 
		ImGui::SameLine();
		if (ImGui::Button("Nightclub2"))
		{
			DesiredLocation = LocationMap["Nightclub2"];
			bRequestedTeleport = true;
		}
	}

	if (ImGui::CollapsingHeader("Cluckin' Bell"))
		{
		ImGui::Indent();
		if (ImGui::Button("Vance"))
		{
			DesiredLocation = LocationMap["Vance"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Office"))
		{
			DesiredLocation = LocationMap["Office"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Keypad1"))
		{
			DesiredLocation = LocationMap["Keypad1"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Keypad2"))
		{
			DesiredLocation = LocationMap["Keypad2"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Garage"))
		{
			DesiredLocation = LocationMap["Garage"];
			bRequestedTeleport = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Drop Off"))
		{
			DesiredLocation = LocationMap["Drop Off"];
			bRequestedTeleport = true;
		}
		ImGui::Unindent();
	}

	if (ImGui::CollapsingHeader("Cayo Perico"))
	{
		ImGui::Indent();

		if (ImGui::CollapsingHeader("Out of Compound"))
		{

			if (ImGui::Button("Radio Tower"))
			{
				DesiredLocation = LocationMap["Radio Tower"];
				bRequestedTeleport = true;
			}

			if (ImGui::Button("Upper Radio Tower"))
			{
				DesiredLocation = LocationMap["Upper Radio Tower"];
				bRequestedTeleport = true;
			}
		}

		if (ImGui::CollapsingHeader("Inside Compound"))
		{


			if (ImGui::Button("First Room"))
			{
				DesiredLocation = LocationMap["First Room"];
				bRequestedTeleport = true;
			}

			if (ImGui::Button("Main Exit"))
			{
				DesiredLocation = LocationMap["Main Exit"];
				bRequestedTeleport = true;
			}
		}

		if (ImGui::CollapsingHeader("Secondary Loot"))
		{
			for (auto Name : CayoSecondaryLocationStrings)
			{
				if (ImGui::Button(Name.c_str()))
				{
					DesiredLocation = LocationMap[Name.c_str()];
					bRequestedTeleport = true;
				}
			}
		}

		ImGui::Unindent();
	}

	if (ImGui::CollapsingHeader("Casino Heist TPs"))
	{
		ImGui::Indent();

		if (ImGui::CollapsingHeader("Vault TP's"))
		{
			if (ImGui::Button("A"))
			{
				DesiredLocation = LocationMap["A"];
				bRequestedTeleport = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("A1"))
			{
				DesiredLocation = LocationMap["A1"];
				bRequestedTeleport = true;
			}

			if (ImGui::Button("B"))
			{
				DesiredLocation = LocationMap["B"];
				bRequestedTeleport = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("B1"))
			{
				DesiredLocation = LocationMap["B1"];
				bRequestedTeleport = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("B2"))
			{
				DesiredLocation = LocationMap["B2"];
				bRequestedTeleport = true;
			}

			if (ImGui::Button("C"))
			{
				DesiredLocation = LocationMap["C"];
				bRequestedTeleport = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("C1"))
			{
				DesiredLocation = LocationMap["C1"];
				bRequestedTeleport = true;
			}



		}

		ImGui::Unindent();
	}


	ImGui::End();

	return 1;
}

void Teleport::PreparePlayerWrites(VMMDLL_SCATTER_HANDLE vmsh)
{
	uintptr_t LocationAddress = DMA::NavigationAddress + offsetof(CNavigation, Position);
	VMMDLL_Scatter_PrepareWrite(vmsh, LocationAddress, (BYTE*)&DesiredLocation, sizeof(Vec3));
}

void Teleport::PrepareVehicleWrites(VMMDLL_SCATTER_HANDLE vmsh)
{
	uintptr_t LocationAddress = DMA::VehicleNavigationAddress + offsetof(CNavigation, Position);
	VMMDLL_Scatter_PrepareWrite(vmsh, LocationAddress, (BYTE*)&DesiredLocation, sizeof(Vec3));
}

#define BLIP_NUM 1000

Vec3 Teleport::GetWaypointCoords()
{
	Vec3 WaypointCoordinates = { 0.0f,0.0f,0.0f };

	std::println("[Waypoint] BlipPtr offset: {:X} (from {})",
		Offsets::BlipPtr, Offsets::BlipPtr == 0x3e7bf50 ? "fallback" : "pattern scan");

	uintptr_t BlipsArrayAddress = DMA::BaseAddress + Offsets::BlipPtr;
	std::println("[Waypoint] BlipsArrayAddress: {:X} (base {:X} + offset {:X})",
		BlipsArrayAddress, DMA::BaseAddress, Offsets::BlipPtr);

	uintptr_t BlipAddresses[BLIP_NUM];

	DWORD BytesRead = 0x0;

	VMMDLL_MemReadEx(DMA::vmh, DMA::PID, BlipsArrayAddress, (BYTE*)&BlipAddresses, sizeof(uintptr_t) * BLIP_NUM, &BytesRead, VMMDLL_FLAG_NOCACHE);

	if (BytesRead != (sizeof(uintptr_t) * BLIP_NUM))
	{
		std::println("[Waypoint] Failed reading blip array (read {} of {} bytes)", BytesRead, sizeof(uintptr_t) * BLIP_NUM);
		return WaypointCoordinates;
	}

	// Count non-null blip pointers for diagnostics
	int nonNullCount = 0;
	for (int i = 0; i < BLIP_NUM; i++)
	{
		if (BlipAddresses[i])
			nonNullCount++;
	}
	std::println("[Waypoint] Read {} blip pointers, {} non-null", BLIP_NUM, nonNullCount);

	if (nonNullCount == 0)
	{
		std::println("[Waypoint] All blip pointers are null -- BlipPtr offset is likely wrong!");
		// Show first few raw values for debugging
		std::println("[Waypoint] First 8 raw values: {:X} {:X} {:X} {:X} {:X} {:X} {:X} {:X}",
			BlipAddresses[0], BlipAddresses[1], BlipAddresses[2], BlipAddresses[3],
			BlipAddresses[4], BlipAddresses[5], BlipAddresses[6], BlipAddresses[7]);
		return WaypointCoordinates;
	}

	// Show first few non-null pointers for diagnostics
	int shown = 0;
	for (int i = 0; i < BLIP_NUM && shown < 3; i++)
	{
		if (BlipAddresses[i])
		{
			std::println("[Waypoint] BlipAddresses[{}] = {:X}", i, BlipAddresses[i]);
			shown++;
		}
	}

	auto pBlips = std::make_unique<Blip[]>(BLIP_NUM);
	ZeroMemory(pBlips.get(), sizeof(Blip) * BLIP_NUM);

	VMMDLL_SCATTER_HANDLE vmsh = VMMDLL_Scatter_Initialize(DMA::vmh, DMA::PID, VMMDLL_FLAG_NOCACHE);

	for (int i = 0; i < BLIP_NUM; i++)
	{
		if (!BlipAddresses[i])
			continue;

		VMMDLL_Scatter_PrepareEx(vmsh, BlipAddresses[i], sizeof(Blip), (BYTE*)&pBlips[i], &BytesRead);
	}

	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_CloseHandle(vmsh);
	vmsh = 0;

	bool bFound = false;

	// Show first few blip IDs for diagnostics
	shown = 0;
	for (int i = 0; i < BLIP_NUM && shown < 10; i++)
	{
		if (BlipAddresses[i] && pBlips[i].ID != 0)
		{
			std::println("[Waypoint] Blip[{}] ID={} pos=({:.1f},{:.1f},{:.1f})",
				i, pBlips[i].ID, pBlips[i].Position.x, pBlips[i].Position.y, pBlips[i].Position.z);
			shown++;
		}
	}

	for (int i = 0; i < BLIP_NUM; i++)
	{
		if (pBlips[i].ID == 8)
		{
			std::println("[Waypoint] Found waypoint blip at index {}!", i);
			std::println("[Waypoint] Coords: {:.2f} {:.2f} {:.2f}", pBlips[i].Position.x, pBlips[i].Position.y, pBlips[i].Position.z);

			WaypointCoordinates = pBlips[i].Position;

			// Blip Z is unreliable (often 0 or 20) -- use heightmap if available.
			float groundZ = Heightmap::GetZ(WaypointCoordinates.x, WaypointCoordinates.y, 0.0f);
			if (groundZ > 0.0f)
			{
				WaypointCoordinates.z = groundZ + 2.0f; // slightly above ground
				std::println("[Waypoint] Heightmap Z={:.1f} for ({:.1f},{:.1f})",
					groundZ, WaypointCoordinates.x, WaypointCoordinates.y);
			}
			else
			{
				// No heightmap or out of bounds -- fall from sky
				WaypointCoordinates.z = 300.0f;
				std::println("[Waypoint] No heightmap data, using Z=300");
			}

			bFound = true;
			break;
		}
	}

	if (!bFound)
		std::println("[Waypoint] No blip with ID=8 (waypoint) found among {} non-null blips", nonNullCount);

	return WaypointCoordinates;
}