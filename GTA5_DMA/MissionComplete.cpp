#include "pch.h"
#include "MissionComplete.h"

// Script local indices (flat, computed from YimMenu's ScriptLocal().At() chains)
// .At(offset) = +offset, .At(index, size) = +1 + index*size

namespace MC
{
	// Common locals for fm_mission_controller heists
	static constexpr size_t MISSION_STATE = 20395;       // Mission state machine
	static constexpr size_t RESULT_CODE = 21457;          // 20395 + 1062 = success code
	static constexpr size_t PAYOUT = 29017;               // 29016 + 1 = payout value
	static constexpr size_t COMPLETION_CTR = 32541;       // 32472 + 1 + 68 = completion counter
	static constexpr size_t TAKE_VALUE = 23081;           // 20395 + 2686 = take value

	// Heist-specific completion flags
	static constexpr size_t CASINO_COMPLETION = 22136;    // 20395 + 1740 + 1
	static constexpr size_t APT_COMPLETION = 22121;       // 20395 + 1725 + 1

	// Cayo Perico (fm_mission_controller_2020)
	static constexpr size_t CAYO_STATE = 56223;
	static constexpr size_t CAYO_COMPLETION = 58000;      // 56223 + 1776 + 1

	static constexpr uint32_t MC_HASH = DMAScript::Joaat("fm_mission_controller");
	static constexpr uint32_t MC2020_HASH = DMAScript::Joaat("fm_mission_controller_2020");
}

bool MissionComplete::FinishCayo()
{
	auto thread = DMAScript::FindScriptThread(MC::MC2020_HASH);
	if (!thread)
	{
		LastResult = "fm_mission_controller_2020 not running (not in Cayo heist)";
		return false;
	}

	auto stack = DMAScript::GetScriptStack(thread);
	if (!stack)
	{
		LastResult = "Failed to read Cayo script stack";
		return false;
	}

	bool ok = true;
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::CAYO_STATE, 9);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::CAYO_COMPLETION, 50);

	LastResult = ok ? "Cayo Perico: Instant finish applied!" : "Cayo Perico: Write failed";
	std::println("[MissionComplete] {}", LastResult);
	return ok;
}

bool MissionComplete::FinishCasino()
{
	auto thread = DMAScript::FindScriptThread(MC::MC_HASH);
	if (!thread)
	{
		LastResult = "fm_mission_controller not running (not in heist)";
		return false;
	}

	auto stack = DMAScript::GetScriptStack(thread);
	if (!stack)
	{
		LastResult = "Failed to read mission controller stack";
		return false;
	}

	bool ok = true;
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::CASINO_COMPLETION, 80);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::TAKE_VALUE, 4443220);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::RESULT_CODE, 5);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::MISSION_STATE, 12);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::PAYOUT, 99999);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::COMPLETION_CTR, 99999);

	LastResult = ok ? "Diamond Casino: Instant finish applied!" : "Diamond Casino: Write failed";
	std::println("[MissionComplete] {}", LastResult);
	return ok;
}

bool MissionComplete::FinishApartment()
{
	auto thread = DMAScript::FindScriptThread(MC::MC_HASH);
	if (!thread)
	{
		LastResult = "fm_mission_controller not running (not in heist)";
		return false;
	}

	auto stack = DMAScript::GetScriptStack(thread);
	if (!stack)
	{
		LastResult = "Failed to read mission controller stack";
		return false;
	}

	bool ok = true;
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::APT_COMPLETION, 80);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::MISSION_STATE, 12);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::PAYOUT, 99999);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::COMPLETION_CTR, 99999);

	LastResult = ok ? "Apartment Heist: Instant finish applied!" : "Apartment Heist: Write failed";
	std::println("[MissionComplete] {}", LastResult);
	return ok;
}

bool MissionComplete::FinishApartmentPacific()
{
	auto thread = DMAScript::FindScriptThread(MC::MC_HASH);
	if (!thread)
	{
		LastResult = "fm_mission_controller not running (not in heist)";
		return false;
	}

	auto stack = DMAScript::GetScriptStack(thread);
	if (!stack)
	{
		LastResult = "Failed to read mission controller stack";
		return false;
	}

	bool ok = true;
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::TAKE_VALUE, 1875000);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::RESULT_CODE, 5);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::MISSION_STATE, 12);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::PAYOUT, 99999);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::COMPLETION_CTR, 99999);

	LastResult = ok ? "Pacific Standard: Instant finish applied!" : "Pacific Standard: Write failed";
	std::println("[MissionComplete] {}", LastResult);
	return ok;
}

bool MissionComplete::FinishDoomsday()
{
	auto thread = DMAScript::FindScriptThread(MC::MC_HASH);
	if (!thread)
	{
		LastResult = "fm_mission_controller not running (not in heist)";
		return false;
	}

	auto stack = DMAScript::GetScriptStack(thread);
	if (!stack)
	{
		LastResult = "Failed to read mission controller stack";
		return false;
	}

	bool ok = true;
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::APT_COMPLETION, 80);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::MISSION_STATE, 12);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::PAYOUT, 99999);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::COMPLETION_CTR, 99999);

	LastResult = ok ? "Doomsday Heist: Instant finish applied!" : "Doomsday Heist: Write failed";
	std::println("[MissionComplete] {}", LastResult);
	return ok;
}

bool MissionComplete::FinishDoomsdayAct3()
{
	auto thread = DMAScript::FindScriptThread(MC::MC_HASH);
	if (!thread)
	{
		LastResult = "fm_mission_controller not running (not in heist)";
		return false;
	}

	auto stack = DMAScript::GetScriptStack(thread);
	if (!stack)
	{
		LastResult = "Failed to read mission controller stack";
		return false;
	}

	bool ok = true;
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::MISSION_STATE, 12);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::CASINO_COMPLETION, 150);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::RESULT_CODE, 5);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::PAYOUT, 99999);
	ok &= DMAScript::WriteScriptLocal<int>(stack, MC::COMPLETION_CTR, 99999);

	LastResult = ok ? "Doomsday Act III: Instant finish applied!" : "Doomsday Act III: Write failed";
	std::println("[MissionComplete] {}", LastResult);
	return ok;
}

void MissionComplete::InstantFinish(HeistType type)
{
	if (type == Auto)
	{
		// Try Cayo first (fm_mission_controller_2020)
		if (DMAScript::FindScriptThread(MC::MC2020_HASH))
		{
			FinishCayo();
			return;
		}
		// Try fm_mission_controller (Casino/Apartment/Doomsday)
		if (DMAScript::FindScriptThread(MC::MC_HASH))
		{
			// Default to Casino finish as it's the most comprehensive
			FinishCasino();
			return;
		}
		LastResult = "No mission controller running (not in a heist finale)";
		std::println("[MissionComplete] {}", LastResult);
		return;
	}

	switch (type)
	{
	case Cayo:             FinishCayo(); break;
	case Casino:           FinishCasino(); break;
	case Apartment:        FinishApartment(); break;
	case ApartmentPacific: FinishApartmentPacific(); break;
	case Doomsday:         FinishDoomsday(); break;
	case DoomsdayAct3:     FinishDoomsdayAct3(); break;
	default:               LastResult = "Unknown heist type"; break;
	}
}
