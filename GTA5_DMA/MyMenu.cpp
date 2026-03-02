#include "pch.h"

#include "MyMenu.h"
#include "MyImGui.h"
#include "Features.h"
#include "offsets/Offsets.h"

bool MyMenu::Render(HWND hwnd)
{
	ImGui::Begin("GTA V Enhanced Edition DMA", nullptr, ImGuiWindowFlags_AlwaysAutoResize);;

	ImGui::SeparatorText("Menu Toggles");

	if (ImGui::Button(g_IsFullscreen ? "Exit Fullscreen" : "Go Fullscreen"))
	{
		ToggleFullscreen(hwnd);
	}

	ImGui::Checkbox("Weapon Inspector", &WeaponInspector::bEnable);
	ImGui::Spacing();
	ImGui::Checkbox("Car Inspector", &CarInspector::bEnable);
	ImGui::Spacing();
	ImGui::Checkbox("Teleport", &Teleport::bEnable);
	ImGui::Spacing();
	ImGui::SeparatorText("Player");
	ImGui::Spacing();
	ImGui::Checkbox("Run Speed Modifier", &RunSpeed::bEnable);
	ImGui::SameLine();
	ImGui::InputFloat("##RunSpeedValue", &RunSpeed::DesiredSpeed, 0.1f, 1.0f, "%.1f");
	ImGui::Checkbox("Swim Speed Modifier", &SwimSpeed::bEnable); // Add this
	ImGui::SameLine();
	ImGui::InputFloat("##SwimSpeedValue", &SwimSpeed::DesiredSpeed, 0.1f, 1.0f, "%.1f");
	ImGui::Checkbox("Refresh Health", &RefreshHealth::bEnable);
	ImGui::SameLine();
	ImGui::InputFloat("##RefreshHealthThreshold", &RefreshHealth::HealThresholdPercent);
	ImGui::Checkbox("Never Wanted", &NoWanted::bEnable);
	ImGui::Spacing();
	ImGui::Checkbox("God Mode", &GodMode::bEnable);
	ImGui::Spacing();
	ImGui::Checkbox("Off The Radar", &OffTheRadar::bEnable);
	ImGui::Spacing();
	ImGui::Checkbox("Invisibility", &Invisibility::bEnable);
	ImGui::Spacing();
	Noclip::Render();
	ImGui::Spacing();
	ImGui::SeparatorText("Vehicle");
	ImGui::Checkbox("Vehicle God Mode", &CarGodMode::bEnable);
	ImGui::Spacing();
	ImGui::Checkbox("Seat Belt", &Seatbelt::bEnable);
	ImGui::Spacing();
	ImGui::Checkbox("Enable Car Control", &CarControl::bEnable);


	if (CarControl::bEnable)  // Only show dropdown when enabled
	{
		static const WORD values[] = {
			0x0000, 0x0040, 0x0020, 0x0100, 0x0200,
			0x0140, 0x0120, 0x0060, 0x0160, 0x0340, 0x0360
		};

		const char* items =
			"OFF\0Rocket Boost (RB)\0Vehicle Jump (VJ)\0Parachute (P)\0"
			"Ramp Buggy (RmB)\0RB+P\0VJ+P\0RB+VJ\0RB+VJ+P\0"
			"RB+P+RmB (Best)\0RB+VJ+P+RmB (All)\0";

		// Find current selection index
		int current_index = 0;
		for (int i = 0; i < IM_ARRAYSIZE(values); i++) {
			if (values[i] == CarControl::CurrentValue) {
				current_index = i;
				break;
			}
		}

		// Display the combo box only when enabled
		if (ImGui::Combo("Car Features", &current_index, items)) {
			CarControl::CurrentValue = values[current_index];
		}
	}
	ImGui::Spacing();
	ImGui::Checkbox("Vehicle Hash Changer", &HashChanger::bEnable);
	if (HashChanger::bEnable)
	{
		ImGui::Indent();
		ImGui::SeparatorText("Beware if you set a invalid hash you will crash");
		ImGui::InputInt("Model Hash (Decimal)", &HashChanger::NewHash);

		if (ImGui::Button("Set to Drag"))
		{
			HashChanger::NewHash = -255678177; // Hakachou Drag hash
		}

		ImGui::SameLine();

		if (ImGui::Button("Set to Sultan RS"))
		{
			HashChanger::NewHash = -295689028; // Sultan RS hash
		}

		ImGui::SameLine();
		if (ImGui::Button("Set to Kuruma"))
		{
			HashChanger::NewHash = 410882957; // Karuma Armored hash
		}

		ImGui::SameLine();
		if (ImGui::Button("Set to Oppressor Mk2"))
		{
			HashChanger::NewHash = 2069146067; // Oppressor Mk2 hash
		}

		ImGui::SameLine();
		if (ImGui::Button("Set to Draugur"))
		{
			HashChanger::NewHash = 3526730918; // Drauger hash
		}
		ImGui::Unindent();

	}

	ImGui::Spacing();
	ImGui::SeparatorText("Recovery");
	CasinoSlotRigger::Render();
	ImGui::Spacing();
	HeistCuts::Render();
	ImGui::Spacing();
	HeistSetup::Render();
	ImGui::Spacing();
	VehicleRequest::Render();
	ImGui::Spacing();
	if (ImGui::CollapsingHeader("Daily Objectives"))
	{
		ImGui::Indent();
		DailyObjectives::Render();
		ImGui::Unindent();
	}

	ImGui::Spacing();
	ImGui::SeparatorText("Tunables");
	if (TunableService::IsLoaded())
	{
		ImGui::Text("Tunables loaded: %d entries", TunableService::TunableCount);
		ImGui::Checkbox("RP Multiplier", &RPMultiplier::bEnable);
		if (RPMultiplier::bEnable)
		{
			ImGui::SameLine();
			ImGui::SliderFloat("##RPMult", &RPMultiplier::Multiplier, 1.0f, 100.0f, "x%.1f");
		}
		ImGui::Checkbox("No Idle Kick", &NoIdleKick::bEnable);

		// Free Appearance Change
		{
			static bool bFreeAppearance = false;
			ImGui::Checkbox("Free Appearance Change", &bFreeAppearance);
			if (bFreeAppearance)
			{
				TunableService::SetTunableInt(0x5B6092F1, 0); // CHARACTER_APPEARANCE_CHARGE
				TunableService::SetTunableInt(0x5C0C54C5, 0); // CHARACTER_APPEARANCE_COOLDOWN
			}
		}
	}
	else
	{
		ImGui::TextDisabled("Tunables not loaded (place tunables.bin next to exe)");
	}

	ImGui::Spacing();
	ImGui::SeparatorText("World");
	if (Offsets::PedPool)
	{
		if (ImGui::Button("Kill All Peds"))
			WorldActions::KillAllPeds();
		ImGui::SameLine();
		if (ImGui::Button("Kill Armed NPCs"))
			WorldActions::KillAllEnemies();
	}
	if (Offsets::ObjectPool)
	{
		if (Offsets::PedPool) ImGui::SameLine();
		if (ImGui::Button("Destroy Cameras"))
			WorldActions::DestroyCameras();
	}
	if (!Offsets::PedPool && !Offsets::ObjectPool)
		ImGui::TextDisabled("Pool offsets not resolved (pattern scan failed)");
	if (Offsets::PedPool || Offsets::ObjectPool)
	{
		if (Offsets::PedPool)
		{
			ImGui::Checkbox("Loop Kill Peds", &WorldActions::bLoopKillPeds);
			ImGui::SameLine();
			ImGui::Checkbox("Loop Kill Armed", &WorldActions::bLoopKillEnemies);
		}
		if (Offsets::ObjectPool)
		{
			if (Offsets::PedPool) ImGui::SameLine();
			ImGui::Checkbox("Loop Cameras", &WorldActions::bLoopDestroyCameras);
		}
	}
	if (!WorldActions::LastResult.empty())
		ImGui::TextWrapped("%s", WorldActions::LastResult.c_str());

	ImGui::Spacing();
	if (Offsets::ScriptThreads)
	{
		ImGui::Combo("##HeistType", &MissionComplete::SelectedHeist,
			"Auto-Detect\0Cayo Perico\0Diamond Casino\0Apartment Heist\0Apartment (Pacific Standard)\0Doomsday Heist\0Doomsday Act III\0");
		ImGui::SameLine();
		if (ImGui::Button("Instant Finish"))
			MissionComplete::InstantFinish((MissionComplete::HeistType)MissionComplete::SelectedHeist);
		if (!MissionComplete::LastResult.empty())
			ImGui::TextWrapped("%s", MissionComplete::LastResult.c_str());
		ImGui::TextDisabled("Must be mission host (works in solo)");
	}

	ImGui::Spacing();
	ImGui::SeparatorText("Misc");
	// GTA Plus unlock
	{
		static bool bGTAPlus = false;
		if (ImGui::Checkbox("GTA+ Unlock", &bGTAPlus))
		{
			if (bGTAPlus && Offsets::HasGTAPlus)
				DMA::Write(DMA::BaseAddress + Offsets::HasGTAPlus, (int)1);
		}
	}

	ImGui::Spacing();
	ImGui::SeparatorText("Info");
	ImGui::Checkbox("Show Player List", &PlayerList::bEnable);
	if (PlayerList::bEnable)
	{
		PlayerList::Render();
	}

	ImGui::End();
	return 1;
}