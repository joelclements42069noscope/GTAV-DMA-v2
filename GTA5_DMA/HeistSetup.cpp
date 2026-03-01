#include "pch.h"
#include "HeistSetup.h"
#include "StatsWriter.h"
#include "core/DMAScriptHelper.h"
#include "TunableService.h"

// ========================================================
// Diamond Casino Heist -- stat values from YimMenu V2
// ========================================================

// Target stat values (MPX_H3OPT_TARGET)
static constexpr int CASINO_TARGET_CASH     = 0;
static constexpr int CASINO_TARGET_GOLD     = 1;
static constexpr int CASINO_TARGET_ART      = 2;
static constexpr int CASINO_TARGET_DIAMONDS = 3;

// Approach stat values (MPX_H3OPT_APPROACH) -- stored as UI index + 1
// 1 = Silent & Sneaky, 2 = The Big Con, 3 = Aggressive

// Gunman UI index -> stat value (MPX_H3OPT_CREWWEAP)
static constexpr int GUNMAN_STAT[] = { 4, 2, 5, 3, 1, 6 };
// 0=Chester(4), 1=Gustavo(2), 2=Patrick(5), 3=Charlie(3), 4=Karl(1), 5=Remove(6)

// Driver UI index -> stat value (MPX_H3OPT_CREWDRIVER)
static constexpr int DRIVER_STAT[] = { 5, 3, 2, 4, 1, 6 };
// 0=Chester(5), 1=Eddie(3), 2=Taliana(2), 3=Zach(4), 4=Karim(1), 5=Remove(6)

// Hacker stat values (MPX_H3OPT_CREWHACKER)
static constexpr int HACKER_STAT[] = { 4, 5, 2, 3, 1, 6 };
// 0=Avi(4), 1=Paige(5), 2=Christian(2), 3=Yohan(3), 4=Rickie(1), 5=Remove(6)

// Target UI labels (ordered by value: best first)
static const char* CASINO_TARGET_LABELS[] = { "Diamonds", "Gold", "Artwork", "Cash" };
static constexpr int CASINO_TARGET_VALUES[] = { 3, 1, 2, 0 };

// ========================================================
// Cayo Perico -- stat values from YimMenu V2
// ========================================================

// Target stat values (MPX_H4CNF_TARGET)
static const char* CAYO_TARGET_LABELS[] = { "Panther Statue", "Pink Diamond", "Madrazo Files", "Bearer Bonds", "Ruby Necklace", "Sinsimito Tequila" };
static constexpr int CAYO_TARGET_VALUES[] = { 5, 3, 4, 2, 1, 0 };

// Weapon stat values (MPX_H4CNF_WEAPONS)
static const char* CAYO_WEAPON_LABELS[] = { "Aggressor", "Conspirator", "Crack Shot", "Saboteur", "Marksman" };
static constexpr int CAYO_WEAPON_VALUES[] = { 1, 2, 3, 4, 5 };

// Difficulty stat values (MPX_H4_PROGRESS)
static constexpr int CAYO_DIFFICULTY_NORMAL = 126823;
static constexpr int CAYO_DIFFICULTY_HARD   = 131055;

// ========================================================
// Implementation
// ========================================================

void HeistSetup::ApplyCasinoSetup()
{
	int approachStat = CasinoApproach + 1; // 1=Silent, 2=BigCon, 3=Aggressive
	int targetStat = CASINO_TARGET_VALUES[CasinoTarget];
	int gunmanStat = GUNMAN_STAT[CasinoGunman];
	int driverStat = DRIVER_STAT[CasinoDriver];
	int hackerStat = HACKER_STAT[CasinoHacker];

	int ok = 0, total = 0;

	auto setStat = [&](const char* name, int val) {
		total++;
		if (StatsWriter::SetStatInt(name, val)) ok++;
	};

	// Core setup
	setStat("MPX_H3_COMPLETEDPOSIX", -1);
	setStat("MPX_H3OPT_MASKS", 4);
	setStat("MPX_H3OPT_WEAPS", CasinoWeapon);
	setStat("MPX_H3OPT_VEHS", 0);
	setStat("MPX_CAS_HEIST_FLOW", -1);
	setStat("MPX_H3_LAST_APPROACH", 0);
	setStat("MPX_H3OPT_APPROACH", approachStat);

	// Difficulty
	if (CasinoDifficulty == 0)
		setStat("MPX_H3_HARD_APPROACH", 0);
	else
		setStat("MPX_H3_HARD_APPROACH", approachStat);

	// Target
	setStat("MPX_H3OPT_TARGET", targetStat);

	// Setup completion (all POIs and access points discovered)
	setStat("MPX_H3OPT_POI", 1023);
	setStat("MPX_H3OPT_ACCESSPOINTS", 2047);

	// Crew
	setStat("MPX_H3OPT_CREWWEAP", gunmanStat);
	setStat("MPX_H3OPT_CREWDRIVER", driverStat);
	setStat("MPX_H3OPT_CREWHACKER", hackerStat);

	// Security & equipment
	setStat("MPX_H3OPT_DISRUPTSHIP", 3);    // Weakest security
	setStat("MPX_H3OPT_BODYARMORLVL", -1);
	setStat("MPX_H3OPT_KEYLEVELS", 2);      // Level 2 security pass

	// Refresh board (bitsets)
	setStat("MPX_H3OPT_BITSET0", -1);
	setStat("MPX_H3OPT_BITSET1", -1);

	std::println("[HeistSetup] Casino Heist: {}/{} stats written (approach={}, target={}, difficulty={})",
		ok, total, approachStat, targetStat, CasinoDifficulty ? "Hard" : "Normal");

	StatusText = (ok == total) ? "Casino setup applied!" : "Casino setup: some stats failed (check log)";
}

void HeistSetup::ApplyCayoSetup()
{
	int targetStat = CAYO_TARGET_VALUES[CayoTarget];
	int weaponStat = CAYO_WEAPON_VALUES[CayoWeapon];
	int difficultyStat = (CayoDifficulty == 0) ? CAYO_DIFFICULTY_NORMAL : CAYO_DIFFICULTY_HARD;

	int ok = 0, total = 0;

	auto setStat = [&](const char* name, int val) {
		total++;
		if (StatsWriter::SetStatInt(name, val)) ok++;
	};

	// Primary target
	setStat("MPX_H4CNF_TARGET", targetStat);

	// Loot placement -- Gold in compound, coke on island (highest value combo).
	// Each loot type has its own spawn points (different bitmask ranges per stat).

	// Island loot -- coke at all coke spawns, clear other types
	setStat("MPX_H4LOOT_CASH_I", 0);
	setStat("MPX_H4LOOT_CASH_I_SCOPED", 0);
	setStat("MPX_H4LOOT_CASH_C", 0);
	setStat("MPX_H4LOOT_CASH_C_SCOPED", 0);
	setStat("MPX_H4LOOT_COKE_I", 255);              // 0xFF -- coke at all island coke spawns
	setStat("MPX_H4LOOT_COKE_I_SCOPED", 255);
	setStat("MPX_H4LOOT_COKE_C", 0);
	setStat("MPX_H4LOOT_COKE_C_SCOPED", 0);
	setStat("MPX_H4LOOT_WEED_I", 0);
	setStat("MPX_H4LOOT_WEED_I_SCOPED", 0);
	setStat("MPX_H4LOOT_WEED_C", 0);
	setStat("MPX_H4LOOT_WEED_C_SCOPED", 0);
	setStat("MPX_H4LOOT_PAINT", 0);
	setStat("MPX_H4LOOT_PAINT_SCOPED", 0);
	// Gold in compound only
	setStat("MPX_H4LOOT_GOLD_I", 0);
	setStat("MPX_H4LOOT_GOLD_I_SCOPED", 0);
	setStat("MPX_H4LOOT_GOLD_C", 255);              // 0xFF -- all compound slots gold
	setStat("MPX_H4LOOT_GOLD_C_SCOPED", 255);

	// Loot values per type
	setStat("MPX_H4LOOT_CASH_V", 83250);
	setStat("MPX_H4LOOT_COKE_V", 202500);
	setStat("MPX_H4LOOT_GOLD_V", 333333);
	setStat("MPX_H4LOOT_WEED_V", 135000);
	setStat("MPX_H4LOOT_PAINT_V", 180000);

	// Difficulty / progress
	setStat("MPX_H4_PROGRESS", difficultyStat);

	// All setups complete
	setStat("MPX_H4CNF_BS_GEN", 262143);
	setStat("MPX_H4CNF_BS_ENTR", 63);
	setStat("MPX_H4CNF_BS_ABIL", 63);

	// All disruptions done (weakest guards)
	setStat("MPX_H4CNF_WEP_DISRP", 3);
	setStat("MPX_H4CNF_ARM_DISRP", 3);
	setStat("MPX_H4CNF_HEL_DISRP", 3);

	// Approach / equipment
	setStat("MPX_H4CNF_APPROACH", -1);
	setStat("MPX_H4CNF_BOLTCUT", 4424);
	setStat("MPX_H4CNF_UNIFORM", 5256);
	setStat("MPX_H4CNF_GRAPPEL", 5156);
	setStat("MPX_H4_MISSIONS", -1);

	// Weapon loadout
	setStat("MPX_H4CNF_WEAPONS", weaponStat);

	// Supply drop
	setStat("MPX_H4CNF_TROJAN", 5);

	// Playthrough complete
	setStat("MPX_H4_PLAYTHROUGH_STATUS", 100);

	std::println("[HeistSetup] Cayo Perico: {}/{} stats written (target={}, weapon={}, difficulty={})",
		ok, total, targetStat, weaponStat, CayoDifficulty ? "Hard" : "Normal");

	// Read back to verify — read from BOTH offsets to compare
	{
		std::string resolved = "mp0_h4cnf_target";
		if (StatsWriter::GetCharIndex() == 1) resolved = "mp1_h4cnf_target";
		uint32_t hash = DMAScript::Joaat(resolved.c_str());
		uintptr_t dataPtr = StatsWriter::FindStatDataPtr(hash);
		if (dataPtr)
		{
			int v10 = 0, v14 = 0;
			DMA::Read(dataPtr + 0x10, v10);
			DMA::Read(dataPtr + 0x14, v14);
			std::println("[HeistSetup] Verify H4CNF_TARGET: +0x10={}, +0x14={} (expected {})", v10, v14, targetStat);
		}
	}

	// Force planning board to refresh by writing script local 1570 = 2
	// in heist_island_planning (this is what YimMenu does after stat writes)
	bool boardRefreshed = DMAScript::WriteScriptLocal<int>(
		DMAScript::Joaat("heist_island_planning"), 1570, 2);
	if (boardRefreshed)
		std::println("[HeistSetup] Planning board refresh triggered (heist_island_planning local 1570 = 2)");
	else
		std::println("[HeistSetup] Could not refresh planning board (heist_island_planning not running - are you at the planning screen?)");

	StatusText = (ok == total) ?
		(boardRefreshed ? "Cayo setup applied & board refreshed!" : "Cayo stats written! Leave and return to board.") :
		"Cayo setup: some stats failed (check console)";
}

// ========================================================
// Cayo Perico -- Take Value Override (use during heist finale)
// ========================================================

// IH_PRIMARY_TARGET_VALUE tunable hashes (JOAAT)
static constexpr uint32_t IH_TARGET_HASHES[] = {
	DMAScript::Joaat("IH_PRIMARY_TARGET_VALUE_TEQUILA"),               // target 0
	DMAScript::Joaat("IH_PRIMARY_TARGET_VALUE_PEARL_NECKLACE"),        // target 1
	DMAScript::Joaat("IH_PRIMARY_TARGET_VALUE_BEARER_BONDS"),          // target 2
	DMAScript::Joaat("IH_PRIMARY_TARGET_VALUE_PINK_DIAMOND"),          // target 3
	DMAScript::Joaat("IH_PRIMARY_TARGET_VALUE_MADRAZO_FILES"),         // target 4
	DMAScript::Joaat("IH_PRIMARY_TARGET_VALUE_SAPPHIRE_PANTHER_STATUE") // target 5
};

// ScriptLocal(thread, 59705).At(1376).At(53) = 59705 + 1376 + 53 = 61134
static constexpr size_t CAYO_SECONDARY_TAKE_LOCAL = 59705 + 1376 + 53;

void HeistSetup::ApplyCayoTakeOverride()
{
	// 1. Override secondary take value via script local
	bool ok1 = DMAScript::WriteScriptLocal<int>(
		DMAScript::Joaat("fm_mission_controller_2020"), CAYO_SECONDARY_TAKE_LOCAL, CayoSecondaryTakeValue);

	if (ok1)
		std::println("[HeistSetup] Secondary take set to ${}", CayoSecondaryTakeValue);
	else
		std::println("[HeistSetup] Failed to write secondary take (fm_mission_controller_2020 not running?)");

	// 2. Override primary target value via tunable
	int targetStat = CAYO_TARGET_VALUES[CayoTarget];
	if (targetStat >= 0 && targetStat <= 5)
	{
		if (TunableService::SetTunableInt(IH_TARGET_HASHES[targetStat], CayoPrimaryTargetValue))
			std::println("[HeistSetup] Primary target value set to ${}", CayoPrimaryTargetValue);
		else
			std::println("[HeistSetup] Failed to write primary target tunable (hash {:08X})", IH_TARGET_HASHES[targetStat]);
	}

	StatusText = ok1 ? "Take override applied!" : "Take override failed (not in heist?)";
}

// ========================================================
// In-Heist Actions (skip drilling, hacking, etc.)
// These write to script locals in the active mission controller.
// ========================================================

// Cayo Perico mission controller: fm_mission_controller_2020
static constexpr uint32_t CAYO_MC_HASH = DMAScript::Joaat("fm_mission_controller_2020");
// Casino Heist mission controller: fm_mission_controller
static constexpr uint32_t CASINO_MC_HASH = DMAScript::Joaat("fm_mission_controller");

void HeistSetup::SkipCayoHacking()
{
	if (DMAScript::WriteScriptLocal<int>(CAYO_MC_HASH, 26486, 5))
		StatusText = "Cayo hacking skipped!";
	else
		StatusText = "Failed (not in Cayo heist?)";
}

void HeistSetup::SkipCayoSewer()
{
	if (DMAScript::WriteScriptLocal<int>(CAYO_MC_HASH, 31349, 6))
		StatusText = "Cayo sewer cut skipped!";
	else
		StatusText = "Failed (not in Cayo heist?)";
}

void HeistSetup::SkipCayoGlass()
{
	if (DMAScript::WriteScriptLocal<float>(CAYO_MC_HASH, 32589 + 3, 100.0f))
		StatusText = "Cayo glass cut skipped!";
	else
		StatusText = "Failed (not in Cayo heist?)";
}

void HeistSetup::SkipCasinoHacking()
{
	bool ok = DMAScript::WriteScriptLocal<int>(CASINO_MC_HASH, 54042, 5);
	ok = DMAScript::WriteScriptLocal<int>(CASINO_MC_HASH, 55108, 5) || ok;
	StatusText = ok ? "Casino hacking skipped!" : "Failed (not in Casino heist?)";
}

void HeistSetup::SkipCasinoDrilling()
{
	// Read the target value from local 10551+37, write to local 10551+7
	auto thread = DMAScript::FindScriptThread(CASINO_MC_HASH);
	if (!thread) { StatusText = "Failed (not in Casino heist?)"; return; }
	auto stack = DMAScript::GetScriptStack(thread);
	if (!stack) { StatusText = "Failed (no stack)"; return; }

	int targetVal = 0;
	DMAScript::ReadScriptLocal<int>(stack, 10551 + 37, targetVal);
	DMAScript::WriteScriptLocal<int>(stack, 10551 + 7, targetVal);
	StatusText = "Casino drilling skipped!";
}

void HeistSetup::RenderCasinoSetup()
{
	// Approach
	const char* approachItems = "Silent & Sneaky\0The Big Con\0Aggressive\0";
	ImGui::Combo("Approach##casino", &CasinoApproach, approachItems);

	// Target
	const char* targetItems = "Diamonds\0Gold\0Artwork\0Cash\0";
	ImGui::Combo("Target##casino", &CasinoTarget, targetItems);

	// Gunman
	const char* gunmanItems = "Chester McCoy\0Gustavo Mota\0Patrick McReary\0Charlie Reed\0Karl Abolaji\0Remove\0";
	ImGui::Combo("Gunman##casino", &CasinoGunman, gunmanItems);

	// Driver
	const char* driverItems = "Chester McCoy\0Eddie Toh\0Taliana Martinez\0Zach Nelson\0Karim Denz\0Remove\0";
	ImGui::Combo("Driver##casino", &CasinoDriver, driverItems);

	// Hacker
	const char* hackerItems = "Avi Schwartzman\0Paige Harris\0Christian Feltz\0Yohan Blair\0Rickie Lukens\0Remove\0";
	ImGui::Combo("Hacker##casino", &CasinoHacker, hackerItems);

	// Weapon (0 or 1, loadout depends on gunman+approach but stat is just the index)
	ImGui::SliderInt("Weapon Loadout##casino", &CasinoWeapon, 0, 1, "Loadout %d");

	// Difficulty
	const char* diffItems = "Normal\0Hard\0";
	ImGui::Combo("Difficulty##casino", &CasinoDifficulty, diffItems);

	if (ImGui::Button("Apply Casino Setup"))
		ApplyCasinoSetup();

	ImGui::Spacing();
	ImGui::SeparatorText("In-Heist Actions (Casino)");
	ImGui::TextDisabled("Use these during the heist finale");
	if (ImGui::Button("Skip Hacking##casino"))
		SkipCasinoHacking();
	ImGui::SameLine();
	if (ImGui::Button("Skip Drilling##casino"))
		SkipCasinoDrilling();
}

void HeistSetup::RenderCayoSetup()
{
	// Target
	const char* targetItems = "Panther Statue\0Pink Diamond\0Madrazo Files\0Bearer Bonds\0Ruby Necklace\0Sinsimito Tequila\0";
	ImGui::Combo("Target##cayo", &CayoTarget, targetItems);

	// Weapon
	const char* weaponItems = "Aggressor\0Conspirator\0Crack Shot\0Saboteur\0Marksman\0";
	ImGui::Combo("Weapon##cayo", &CayoWeapon, weaponItems);

	// Difficulty
	const char* diffItems = "Normal\0Hard\0";
	ImGui::Combo("Difficulty##cayo", &CayoDifficulty, diffItems);

	if (ImGui::Button("Apply Cayo Setup"))
		ApplyCayoSetup();

	ImGui::Spacing();
	ImGui::SeparatorText("In-Heist Actions (Cayo)");
	ImGui::TextDisabled("Use these during the heist finale");

	ImGui::InputInt("Secondary Take $##cayo", &CayoSecondaryTakeValue, 100000, 500000);
	ImGui::InputInt("Primary Target $##cayo", &CayoPrimaryTargetValue, 100000, 500000);
	if (ImGui::Button("Apply Take Override##cayo"))
		ApplyCayoTakeOverride();

	ImGui::Spacing();
	if (ImGui::Button("Skip Hacking##cayo"))
		SkipCayoHacking();
	ImGui::SameLine();
	if (ImGui::Button("Skip Sewer##cayo"))
		SkipCayoSewer();
	ImGui::SameLine();
	if (ImGui::Button("Skip Glass##cayo"))
		SkipCayoGlass();
}

void HeistSetup::Render()
{
	if (!StatsWriter::IsReady())
	{
		ImGui::TextDisabled("Stats not available (CStatsMgr not initialized)");
		if (ImGui::Button("Retry Init##heist"))
			StatsWriter::EnsureInitialized();
		return;
	}

	if (ImGui::CollapsingHeader("Casino Heist Setup"))
	{
		ImGui::Indent();
		RenderCasinoSetup();
		ImGui::Unindent();
	}

	if (ImGui::CollapsingHeader("Cayo Perico Setup"))
	{
		ImGui::Indent();
		RenderCayoSetup();
		ImGui::Unindent();
	}

	// Diagnostic info
	ImGui::Text("Stats: %d entries, Char index: %d (MP%d_)",
		StatsWriter::GetStatCount(), StatsWriter::GetCharIndex(), StatsWriter::GetCharIndex());
	if (ImGui::Button("Diagnose Stats##heist"))
	{
		std::println("[HeistSetup] === Full stat diagnostic ===");
		StatsWriter::DiagnoseStat("MPX_H4CNF_TARGET");
		StatsWriter::DiagnoseStat("MPX_H4CNF_WEAPONS");
		StatsWriter::DiagnoseStat("MPX_H4_PROGRESS");
		StatsWriter::DiagnoseStat("MPX_H4_PLAYTHROUGH_STATUS");
		StatsWriter::DiagnoseStat("MPX_H4LOOT_GOLD_C");
		StatsWriter::DiagnoseStat("MPPLY_LAST_MP_CHAR");
		// Read a stat we haven't written to check if reads work generally
		StatsWriter::DiagnoseStat("MPX_WALLET_BALANCE");
		StatusText = "Check console for diagnose output";
	}

	if (StatusText && StatusText[0])
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%s", StatusText);
}
