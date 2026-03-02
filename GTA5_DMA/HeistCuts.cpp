#include "pch.h"
#include "HeistCuts.h"

// -------------------------------------------------------------------
// Cayo Perico Heist
// Global indices from YimMenu: ScriptGlobal(1980035).At(831).At(56)
// -------------------------------------------------------------------

static constexpr DWORD CAYO_CUTS_BASE = 1980035 + 831 + 56; // = 1980922
// Each player cut: .At(playerIdx, 1) = base + 1 + playerIdx

static constexpr DWORD CAYO_READY_BASE = 1981146;
// Force ready: .At(i, 27).At(7).At(i, 1) = base + 1 + i*27 + 7 + 1 + i = base + 9 + 28*i

// -------------------------------------------------------------------
// Diamond Casino Heist
// Global indices from YimMenu: ScriptGlobal(1973231).At(1497).At(736).At(92)
// -------------------------------------------------------------------

static constexpr DWORD CASINO_CUTS_BASE = 1973231 + 1497 + 736 + 92; // = 1975556
// Each player cut: .At(playerIdx, 1) = base + 1 + playerIdx

static constexpr DWORD CASINO_READY_BASE = 1977594;
// Force ready: .At(i, 68).At(7).At(i, 1) = base + 1 + i*68 + 7 + 1 + i = base + 9 + 69*i

void HeistCuts::SetCayoCuts()
{
	static bool logged = false;
	if (!logged) { std::println("[HeistCuts] Writing Cayo cuts to globals {}-{}", CAYO_CUTS_BASE + 1, CAYO_CUTS_BASE + 4); logged = true; }

	for (int i = 0; i < 4; i++)
	{
		DWORD cutIndex = CAYO_CUTS_BASE + 1 + i;
		DMA::SetGlobalInt(cutIndex, CayoCuts[i]);
	}
}

void HeistCuts::CayoForceReady()
{
	for (int i = 0; i < 4; i++)
	{
		DWORD readyIndex = CAYO_READY_BASE + 9 + 28 * i;
		DMA::SetGlobalInt(readyIndex, 1);
	}
}

void HeistCuts::SetCasinoCuts()
{
	static bool logged = false;
	if (!logged) { std::println("[HeistCuts] Writing Casino cuts to globals {}-{}", CASINO_CUTS_BASE + 1, CASINO_CUTS_BASE + 4); logged = true; }

	for (int i = 0; i < 4; i++)
	{
		DWORD cutIndex = CASINO_CUTS_BASE + 1 + i;
		DMA::SetGlobalInt(cutIndex, CasinoCuts[i]);
	}
}

void HeistCuts::CasinoForceReady()
{
	for (int i = 0; i < 4; i++)
	{
		DWORD readyIndex = CASINO_READY_BASE + 9 + 69 * i;
		DMA::SetGlobalInt(readyIndex, 1);
	}
}

bool HeistCuts::OnDMAFrame()
{
	if (bCayoSetCuts) SetCayoCuts();
	if (bCayoForceReady) CayoForceReady();
	if (bCasinoSetCuts) SetCasinoCuts();
	if (bCasinoForceReady) CasinoForceReady();

	return true;
}

void HeistCuts::Render()
{
	if (ImGui::CollapsingHeader("Cayo Perico Heist"))
	{
		ImGui::Checkbox("Set Cayo Cuts", &bCayoSetCuts);
		if (bCayoSetCuts)
		{
			ImGui::SliderInt("Player 1 Cut##cayo", &CayoCuts[0], 0, 100);
			ImGui::SliderInt("Player 2 Cut##cayo", &CayoCuts[1], 0, 100);
			ImGui::SliderInt("Player 3 Cut##cayo", &CayoCuts[2], 0, 100);
			ImGui::SliderInt("Player 4 Cut##cayo", &CayoCuts[3], 0, 100);
		}
		ImGui::Checkbox("Force Ready (Cayo)", &bCayoForceReady);
	}

	if (ImGui::CollapsingHeader("Diamond Casino Heist"))
	{
		ImGui::Checkbox("Set Casino Cuts", &bCasinoSetCuts);
		if (bCasinoSetCuts)
		{
			ImGui::SliderInt("Player 1 Cut##casino", &CasinoCuts[0], 0, 100);
			ImGui::SliderInt("Player 2 Cut##casino", &CasinoCuts[1], 0, 100);
			ImGui::SliderInt("Player 3 Cut##casino", &CasinoCuts[2], 0, 100);
			ImGui::SliderInt("Player 4 Cut##casino", &CasinoCuts[3], 0, 100);
		}
		ImGui::Checkbox("Force Ready (Casino)", &bCasinoForceReady);
	}
}
