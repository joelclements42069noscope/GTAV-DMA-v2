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

	// Force planning board to refresh by writing script local 1580 = 2
	// in heist_island_planning (this is what YimMenu does after stat writes)
	// b1158.13: was 1570 pre-1.73.
	bool boardRefreshed = DMAScript::WriteScriptLocal<int>(
		DMAScript::Joaat("heist_island_planning"), 1580, 2);
	if (boardRefreshed)
		std::println("[HeistSetup] Planning board refresh triggered (heist_island_planning local 1580 = 2)");
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

// ScriptLocal(thread, 59986).At(1376).At(53) = 59986 + 1376 + 53 = 61415
// b1158.13: base was 59705 pre-1.73.
static constexpr size_t CAYO_SECONDARY_TAKE_LOCAL = 59986 + 1376 + 53;

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
// Kortz Center Heist (K26) -- instant setup via named stats
// Bit maps (from newstuff.txt / the community setup script):
//   MPX_K26_GENERAL_BS  bits 5-8  = Guard Routes / Glass Cutter / Power Drills / EMP Charges
//   MPX_K26_ROBBERY_PROG bits 0-15 = the 16 prep-work items (see labels below)
//   MPX_K26_SCOPING_BS  = -1 -> all secondary targets scoped
//   MPX_K26_POI_BS      = -1 -> all points of interest scoped (unlocks optional preps)
//   MPX_K26_HEIST_TARGET = 0..26 (primary painting)
//   MPX_K26_GENERAL_BS2 = -1 (updates itself off the planning board)
//   MPX_K26_TARGETS_OWNED_BS = -1 -> all 26 mansion paintings unlocked
// Approach: start each bitset at -1 (everything set) and clear the boxes the
// user left unchecked -- matches the community KortzCenterSetup() exactly.
// ========================================================

static const char* KORTZ_TARGET_LABELS[27] = {
	"La Derniere Debauche", "Hare Oneself Think", "The Downfall of Rome", "Brother Brother",
	"A Cast of Characters", "Gone To Seed", "True Love", "Breathless", "Consumato",
	"I Hear Voices", "Winter, Nowhere in Particular", "The Girl With the Pearl Necklace",
	"Chat on Fruit", "Pumpkin", "Twindifference", "Stacks Study V", "I, Fruit",
	"To Beat About the Bush", "In Excess of Success", "Juiced", "A Winding Road Home",
	"Teckels", "Trust", "Until Death", "What Are Melons?", "The Outcome of Endeavour",
	"Mi O Melee"
};

// Prep-work labels for MPX_K26_ROBBERY_PROG bits 0-15.
static const char* KORTZ_PREP_LABELS[16] = {
	"Scope Out Kortz Center",  // 0
	"Alpha Mail Disguise",     // 1
	"Hazmat Suit",             // 2
	"Staff Key Card",          // 3
	"Tactical Equipment",      // 4
	"Hacking Device",          // 5
	"Access Code",             // 6
	"Unmarked Weapons",        // 7
	"Armored Caracara",        // 8
	"Annihilator Stealth",     // 9
	"Manchez",                 // 10
	"EMP Charges (Prep)",      // 11
	"Guard Shipments",         // 12
	"Guard Routes (Prep)",     // 13
	"Glass Cutter (Prep)",     // 14
	"Power Drills (Prep)",     // 15
};

void HeistSetup::ApplyKortzSetup()
{
	// GENERAL_BS: start at -1 (all bits set) and clear everything not selected.
	//
	// CRITICAL: the loadout (bits 9-11) and Manchez colour (bits 17-20) are
	// MUTUALLY EXCLUSIVE. Leaving -1 in place sets all three loadouts and all
	// four bike colours at once, which is invalid state and makes the planning
	// board refuse to set the heist up. Every exclusive bit must be cleared
	// except the one selected.
	// Bit math is done unsigned (bit 31 would be UB on a signed int).
	uint32_t generalBits = 0xFFFFFFFFu;
	auto clearBit = [&](int b) { generalBits &= ~(1u << b); };

	// Purchases (bits 5-8).
	if (!KortzGuardRoutes) clearBit(5);
	if (!KortzGlassCutter) clearBit(6);
	if (!KortzPowerDrills) clearBit(7);
	if (!KortzEmpCharges)  clearBit(8);

	// Loadout (9=Street, 10=Security, 11=Military) -- keep only the selected one.
	if (KortzLoadoutType != 1) clearBit(9);
	if (KortzLoadoutType != 2) clearBit(10);
	if (KortzLoadoutType != 3) clearBit(11);

	// Manchez colour (17=Red, 18=Blue, 19=Green, 20=Yellow) -- only when the
	// Manchez getaway prep (ROBBERY_PROG bit 10) is actually enabled.
	const bool manchez = KortzPrep[10];
	for (int c = 0; c < 4; c++)
		if (!(manchez && KortzManchezColor == c)) clearBit(17 + c);

	// Misc flags.
	if (!KortzManholeKey) clearBit(27);
	if (!KortzHardMode)   clearBit(28);
	if (!KortzWeakGuards) clearBit(31);

	// ROBBERY_PROG: start at -1, clear unchecked prep bits (0-15).
	uint32_t robberyProg = 0xFFFFFFFFu;
	for (int i = 0; i < 16; i++)
		if (!KortzPrep[i]) robberyProg &= ~(1u << i);

	int scopingBs = KortzScopeSecondary ? -1 : 0;
	int poiBs     = KortzScopePoi ? -1 : 0;
	int target    = (KortzPrimaryTarget >= 0 && KortzPrimaryTarget <= 26) ? KortzPrimaryTarget : 0;

	int ok = 0, total = 0;
	auto setStat = [&](const char* name, int val) {
		total++;
		if (StatsWriter::SetStatInt(name, val)) ok++;
	};

	setStat("MPX_K26_GENERAL_BS", (int)generalBits);
	setStat("MPX_K26_GENERAL_BS2", -1);
	setStat("MPX_K26_ROBBERY_PROG", (int)robberyProg);
	setStat("MPX_K26_HEIST_TARGET", target);
	setStat("MPX_K26_SCOPING_BS", scopingBs);
	setStat("MPX_K26_POI_BS", poiBs);

	std::println("[HeistSetup] Kortz Center: {}/{} stats written (target={} '{}', GENERAL_BS=0x{:08X}, ROBBERY_PROG=0x{:08X})",
		ok, total, target, KORTZ_TARGET_LABELS[target], (uint32_t)generalBits, (uint32_t)robberyProg);

	StatusText = (ok == total) ? "Kortz setup applied!" : "Kortz setup: some stats failed (check log)";
}

void HeistSetup::UnlockKortzPaintings()
{
	// All 26 mansion paintings owned (bits 1..26). -1 sets every bit.
	bool ok = StatsWriter::SetStatInt("MPX_K26_TARGETS_OWNED_BS", -1);
	std::println("[HeistSetup] Kortz paintings unlock: {}", ok ? "OK" : "failed");
	StatusText = ok ? "All Kortz paintings unlocked!" : "Paintings unlock failed (check log)";
}

// ========================================================
// Kortz extras -- documented K26 stats the setup itself doesn't touch.
// All named/hash-based, so version-independent.
// ========================================================

void HeistSetup::KortzRemoveCooldown()
{
	// K26_HEIST_COOLDOWN      = "Tracking the kortz heist cooldown time"
	// K26_HEIST_COOLDOWN_HARD = "Tracking time in which setting up the kortz heist sets hard mode"
	int ok = 0;
	if (StatsWriter::SetStatInt("MPX_K26_HEIST_COOLDOWN", 0)) ok++;
	if (StatsWriter::SetStatInt("MPX_K26_HEIST_COOLDOWN_HARD", 0)) ok++;
	std::println("[Kortz] Remove cooldown: {}/2 stats written", ok);
	StatusText = (ok == 2) ? "Kortz cooldown cleared!" : "Cooldown: partial write (check log)";
}

void HeistSetup::KortzWeeklyBoost()
{
	// "Bitset for tracking weekly boosted payouts" -- the thread reports writing
	// -1 enables every boost. NOTE: this stat is ServerAuthoritative, so R* may
	// correct it back; treat it as best-effort.
	bool ok = StatsWriter::SetStatInt("MPX_WEEKLY_BOOST_BS", -1);
	std::println("[Kortz] Weekly boosted payouts: {}", ok ? "OK" : "failed");
	StatusText = ok ? "Weekly boosted payouts set (server may revert)" : "Weekly boost write failed";
}

void HeistSetup::KortzBuyerRequests()
{
	// "Tracking bitset for kortz heist loot thats part of buyer request bonus"
	bool ok = StatsWriter::SetStatInt("MPX_K26_BUYREQ_BS", -1);
	std::println("[Kortz] Buyer requests: {}", ok ? "OK" : "failed");
	StatusText = ok ? "All buyer requests satisfied!" : "Buyer request write failed";
}

void HeistSetup::KortzMaxApproachPlays()
{
	// Per-approach completion counters. Feeds the approach-variety awards
	// (Making an Entrance / all 4 entry points) and career-progress objectives.
	static const char* PLAY_STATS[] = {
		"MPX_K26_PLAYS_ARENA_CAR",       "MPX_K26_PLAYS_HELICOPTER",
		"MPX_K26_PLAYS_OFF_ROAD",        "MPX_K26_PLAYS_ENTRY",
		"MPX_K26_PLAYS_INSIDE_STEALTH",  "MPX_K26_PLAYS_INSIDE_AGGRO",
		"MPX_K26_PLAYS_VAULT_STEALTH",   "MPX_K26_PLAYS_VAULT_AGGRO",
		"MPX_K26_PLAYS_GETAWAY_STEALTH", "MPX_K26_PLAYS_GETAWAY_AGGRO",
	};

	int ok = 0;
	for (const char* s : PLAY_STATS)
		if (StatsWriter::SetStatInt(s, 20)) ok++;

	std::println("[Kortz] Approach play counts: {}/{} stats written", ok, (int)std::size(PLAY_STATS));
	StatusText = (ok == (int)std::size(PLAY_STATS))
		? "All approach play counts maxed!" : "Approach plays: partial write (check log)";
}

// ========================================================
// Kortz advanced -- the remaining K26 stats. Their value semantics are NOT
// documented anywhere, so instead of hard-coding guesses these are exposed as
// live-read + explicit-write fields: read what the game currently holds, try a
// value, see what changes. The Comment= text from the stat definitions is kept
// as the tooltip so the intent of each is visible.
// ========================================================

struct KortzAdvStat { const char* name; const char* label; const char* hint; };
static const KortzAdvStat KORTZ_ADV_STATS[] = {
	{ "MPX_K26_PRIMARY_OVERRIDE_ID", "Primary Override ID",
	  "Override ID for whenever live ops run a promo for a specific target.\n"
	  "Likely the same 0-26 painting index as HEIST_TARGET, but unconfirmed --\n"
	  "use 'Sync to selected target' to try the current Primary Target." },
	{ "MPX_K26_HEIST_SEED", "Heist Seed",
	  "The seed for grabbing secondary loot value on the Kortz Center Heist.\n"
	  "Seed -> value mapping is unknown; vary it and re-scope to compare takes." },
	{ "MPX_K26_MANHOLE_KEY_LOC", "Manhole Key Location",
	  "Location of the manhole key. Small index; read it during a real setup\n"
	  "to learn the valid range." },
	{ "MPX_K26_PRIMARY_LOOP", "Primary Loop",
	  "Looping stat for the rotating weekly targets." },
	{ "MPX_K26_WEEK_ID", "Week ID",
	  "Syncs to a tunable for week ID, so weekly changes to heist data get\n"
	  "processed. Changing this may re-roll the weekly target rotation." },
	{ "MPX_K26_STOLENLAST_BS", "Stolen Last (bitset)",
	  "Tracking bitset for the loot stolen in the most recent play.\n"
	  "-1 sets every bit (as with the other _BS stats)." },
	{ "MPX_K26_SESSIONID_MAC", "Session ID (MAC)",
	  "Telemetry only -- included for completeness. Changing it should have\n"
	  "no gameplay effect." },
	{ "MPX_K26_SESSIONID_POS", "Session ID (POS)",
	  "Telemetry only -- included for completeness." },
	{ "MPX_K26_GENERAL_BS2", "General BS2",
	  "Updates itself when using the Planning Board. Reported: after paying for\n"
	  "the Hard Mode setup, bit 3 (value 8) got cleared. The setup writes -1." },
};

void HeistSetup::KortzReadAdvanced()
{
	static_assert(std::size(KORTZ_ADV_STATS) == KORTZ_ADV_COUNT,
		"KORTZ_ADV_STATS must stay in sync with KORTZ_ADV_COUNT");

	for (int i = 0; i < KORTZ_ADV_COUNT; i++)
		KortzAdvLive[i] = StatsWriter::GetStatInt(KORTZ_ADV_STATS[i].name, 0);
	KortzAdvHaveRead = true;

	std::println("[Kortz] Advanced stat read-back:");
	for (int i = 0; i < KORTZ_ADV_COUNT; i++)
		std::println("   {:<30} = {} (0x{:08X})",
			KORTZ_ADV_STATS[i].name, KortzAdvLive[i], (uint32_t)KortzAdvLive[i]);

	StatusText = "Advanced stats read -- see console for the full dump";
}

void HeistSetup::RenderKortzAdvanced()
{
	ImGui::TextWrapped("These K26 stats have undocumented value semantics. Read the live value "
		"first, then write to experiment. Console logs every read.");

	if (ImGui::Button("Read Live Values##kortzadv"))
		KortzReadAdvanced();
	ImGui::SameLine();
	if (ImGui::Button("Write All##kortzadv"))
	{
		int ok = 0;
		for (int i = 0; i < KORTZ_ADV_COUNT; i++)
			if (StatsWriter::SetStatInt(KORTZ_ADV_STATS[i].name, KortzAdvWrite[i])) ok++;
		std::println("[Kortz] Advanced write-all: {}/{} stats written", ok, KORTZ_ADV_COUNT);
		StatusText = (ok == KORTZ_ADV_COUNT) ? "All advanced stats written!"
		                                     : "Advanced write: partial (check log)";
	}

	ImGui::Separator();

	for (int i = 0; i < KORTZ_ADV_COUNT; i++)
	{
		ImGui::PushID(i);

		ImGui::Text("%s", KORTZ_ADV_STATS[i].label);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s\n\n%s", KORTZ_ADV_STATS[i].name, KORTZ_ADV_STATS[i].hint);

		ImGui::SameLine(210.f);
		if (KortzAdvHaveRead)
			ImGui::TextColored(ImVec4(0.4f, 1.f, 0.6f, 1.f), "live: %d", KortzAdvLive[i]);
		else
			ImGui::TextDisabled("live: ?");

		ImGui::SameLine(330.f);
		ImGui::SetNextItemWidth(120.f);
		ImGui::InputInt("##val", &KortzAdvWrite[i]);

		ImGui::SameLine();
		if (ImGui::Button("Write"))
		{
			bool ok = StatsWriter::SetStatInt(KORTZ_ADV_STATS[i].name, KortzAdvWrite[i]);
			std::println("[Kortz] {} = {} -> {}", KORTZ_ADV_STATS[i].name,
				KortzAdvWrite[i], ok ? "OK" : "FAILED");
			StatusText = ok ? "Advanced stat written" : "Advanced stat write failed";
			if (ok) { KortzAdvLive[i] = KortzAdvWrite[i]; KortzAdvHaveRead = true; }
		}

		// The override ID is most plausibly the same 0-26 index as the primary
		// target, so offer a one-click sync rather than making the user retype it.
		if (i == 0)
		{
			ImGui::SameLine();
			if (ImGui::SmallButton("Sync to selected target"))
				KortzAdvWrite[0] = KortzPrimaryTarget;
		}

		ImGui::PopID();
	}
}

// ========================================================
// Kortz Center Cracker -- in-heist actions (fm_mission_controller_v3)
// Script locals from the community "Kortz Center Cracker" (ImagineNothing).
// .At(i, size) flattens to base + 1 + i*size, matching ScriptLocal semantics.
//
// NOTE: three of these (access code, take primary/secondary target) pair the
// memory write with a PAD.SET_CONTROL_VALUE_NEXT_FRAME button press in the Lua.
// DMA cannot call natives, so we do the memory half and you press the interact
// key yourself -- the tooltips say so.
// ========================================================

static constexpr uint32_t KORTZ_MC_HASH = DMAScript::Joaat("fm_mission_controller_v3");

void HeistSetup::KortzSkipDataCrack()
{
	// for b = 0..7: ScriptLocal(1388).At(b, 4) = 1388 + 1 + b*4  -> 1
	auto thread = DMAScript::FindScriptThread(KORTZ_MC_HASH);
	if (!thread) { StatusText = "Failed (Kortz heist not running?)"; return; }
	auto stack = DMAScript::GetScriptStack(thread);
	if (!stack) { StatusText = "Failed (no stack)"; return; }

	int ok = 0;
	for (int b = 0; b < 8; b++)
		if (DMAScript::WriteScriptLocal<int>(stack, 1388 + 1 + b * 4, 1)) ok++;

	std::println("[Kortz] Skip Data Crack: {}/8 nodes written", ok);
	StatusText = (ok == 8) ? "Data Crack skipped!" : "Data Crack: partial write (check log)";
}

void HeistSetup::KortzSkipFingerprint()
{
	bool ok = DMAScript::WriteScriptLocal<int>(KORTZ_MC_HASH, 26866, 5);
	StatusText = ok ? "Fingerprint hacking skipped!" : "Failed (Kortz heist not running?)";
}

void HeistSetup::KortzEnterAccessCode()
{
	// for i = 0..2: ScriptLocal(32820).At(i, 2) = 32820 + 1 + i*2 -> 0
	auto thread = DMAScript::FindScriptThread(KORTZ_MC_HASH);
	if (!thread) { StatusText = "Failed (Kortz heist not running?)"; return; }
	auto stack = DMAScript::GetScriptStack(thread);
	if (!stack) { StatusText = "Failed (no stack)"; return; }

	int ok = 0;
	for (int i = 0; i < 3; i++)
		if (DMAScript::WriteScriptLocal<int>(stack, 32820 + 1 + i * 2, 0)) ok++;

	std::println("[Kortz] Access code: {}/3 digits written", ok);
	StatusText = (ok == 3) ? "Access code entered -- press the interact key!" : "Access code: partial write";
}

void HeistSetup::KortzDisableLasers()
{
	bool a = DMAScript::WriteScriptLocal<int>(KORTZ_MC_HASH, 70416, 4294784);
	bool b = DMA::SetGlobalInt(1935711, 1);
	std::println("[Kortz] Disable lasers: local={} global={}", a, b);
	StatusText = a ? "Laser grid disabled (also clears forced stealth)" : "Failed (Kortz heist not running?)";
}

void HeistSetup::KortzSkipVaultHacking()
{
	bool ok = DMAScript::WriteScriptLocal<int>(KORTZ_MC_HASH, 27914, 5);
	StatusText = ok ? "Vault door (signal nodes) skipped!" : "Failed (Kortz heist not running?)";
}

void HeistSetup::KortzTakePrimaryTarget()
{
	// ScriptLocal(29355 + 11) = 29366: write 15 then 17 (state machine step).
	auto thread = DMAScript::FindScriptThread(KORTZ_MC_HASH);
	if (!thread) { StatusText = "Failed (Kortz heist not running?)"; return; }
	auto stack = DMAScript::GetScriptStack(thread);
	if (!stack) { StatusText = "Failed (no stack)"; return; }

	DMAScript::WriteScriptLocal<int>(stack, 29366, 15);
	bool ok = DMAScript::WriteScriptLocal<int>(stack, 29366, 17);
	StatusText = ok ? "Primary target taken -- press the interact key!" : "Primary target: write failed";
}

void HeistSetup::KortzTakeSecondaryTarget()
{
	bool ok = DMAScript::WriteScriptLocal<int>(KORTZ_MC_HASH, 29366, 3);
	StatusText = ok ? "Secondary target taken -- press the interact key!" : "Failed (Kortz heist not running?)";
}

void HeistSetup::KortzCutGlass()
{
	// ScriptLocal(32855 + 3).At(4, 13) = 32858 + 1 + 4*13 = 32911 -> 100.0f
	bool ok = DMAScript::WriteScriptLocal<float>(KORTZ_MC_HASH, 32858 + 1 + 4 * 13, 100.0f);
	StatusText = ok ? "Display case glass cut!" : "Failed (Kortz heist not running?)";
}

void HeistSetup::KortzSoloSecondaryTargets()
{
	// Resets interaction & loot flags for the Level 2 exhibit horizontal glass
	// cases (i = 5, 6, 7) and artwork/wall displays (i = 20, 21) so they can be
	// looted solo. base = 4980736 + 1 + 29174 + i*333; clear base+68 and base+143.
	constexpr DWORD BASE = 4980736 + 1 + 29174; // = 5009911
	constexpr int INDICES[] = { 5, 6, 7, 20, 21 };

	int ok = 0;
	for (int i : INDICES)
	{
		DWORD base = BASE + (DWORD)i * 333;
		if (DMA::SetGlobalInt(base + 68, 0)) ok++;
		if (DMA::SetGlobalInt(base + 143, 0)) ok++;
	}

	std::println("[Kortz] Solo secondary targets: {}/10 globals written", ok);
	StatusText = (ok == 10) ? "Solo secondary targets enabled!" : "Solo targets: partial write (check log)";
}

void HeistSetup::RenderKortzActions()
{
	ImGui::TextDisabled("Use these during the Kortz Center finale");

	if (ImGui::Button("Skip Data Crack##kortz"))        KortzSkipDataCrack();
	ImGui::SameLine();
	if (ImGui::Button("Skip Fingerprint##kortz"))       KortzSkipFingerprint();

	if (ImGui::Button("Skip Vault Door##kortz"))        KortzSkipVaultHacking();
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Skips the Signal Nodes hack");
	ImGui::SameLine();
	if (ImGui::Button("Disable Lasers##kortz"))         KortzDisableLasers();
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Disables the laser grid and the forced stealth state");

	if (ImGui::Button("Enter Access Code##kortz"))      KortzEnterAccessCode();
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Writes the code -- then press the interact key yourself\n(DMA cannot press buttons for you)");
	ImGui::SameLine();
	if (ImGui::Button("Cut Glass##kortz"))              KortzCutGlass();

	if (ImGui::Button("Take Primary Target##kortz"))    KortzTakePrimaryTarget();
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Be at the painting and start the steal first,\nthen press the interact key after clicking this");
	ImGui::SameLine();
	if (ImGui::Button("Take Secondary Target##kortz"))  KortzTakeSecondaryTarget();

	ImGui::Spacing();
	if (ImGui::Button("Enable Solo Secondary Targets##kortz")) KortzSoloSecondaryTargets();
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Resets loot flags on the Level 2 exhibit cases & artworks\n(i = 5, 6, 7, 20, 21) so they can be taken solo");
}

void HeistSetup::RenderKortzSetup()
{
	ImGui::Combo("Primary Target##kortz", &KortzPrimaryTarget, KORTZ_TARGET_LABELS, 27);

	// Loadout is mutually exclusive -- selecting one clears the other two bits.
	const char* loadoutItems = "None\0Street\0Security\0Military\0";
	ImGui::Combo("Loadout##kortz", &KortzLoadoutType, loadoutItems);
	ImGui::TextDisabled("Needs the Unmarked Weapons prep; Security = weak guards + security guns");

	if (ImGui::TreeNode("General Purchases##kortz"))
	{
		ImGui::Checkbox("Guard Routes##kortz", &KortzGuardRoutes);
		ImGui::Checkbox("Glass Cutter##kortz", &KortzGlassCutter);
		ImGui::Checkbox("Power Drills##kortz", &KortzPowerDrills);
		ImGui::Checkbox("EMP Charges##kortz", &KortzEmpCharges);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Prep Work##kortz"))
	{
		if (ImGui::SmallButton("All##kortzprep"))  for (auto& b : KortzPrep) b = true;
		ImGui::SameLine();
		if (ImGui::SmallButton("None##kortzprep")) for (auto& b : KortzPrep) b = false;
		for (int i = 0; i < 16; i++)
		{
			ImGui::Checkbox(KORTZ_PREP_LABELS[i], &KortzPrep[i]);
			// The Manchez getaway bike also needs a colour bit in GENERAL_BS,
			// otherwise it never shows up in the finale.
			if (i == 10 && KortzPrep[10])
			{
				ImGui::Indent();
				const char* colorItems = "Red\0Blue\0Green\0Yellow\0";
				ImGui::Combo("Manchez Colour##kortz", &KortzManchezColor, colorItems);
				ImGui::Unindent();
			}
		}
		ImGui::TreePop();
	}

	ImGui::Checkbox("Manhole Key##kortz", &KortzManholeKey);
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Required for the sewer entrance");
	ImGui::SameLine();
	ImGui::Checkbox("Hard Mode##kortz", &KortzHardMode);
	ImGui::SameLine();
	ImGui::Checkbox("Weak Guards##kortz", &KortzWeakGuards);

	ImGui::Checkbox("Scope Secondary Targets##kortz", &KortzScopeSecondary);
	ImGui::Checkbox("Scope Points of Interest##kortz", &KortzScopePoi);
	ImGui::TextDisabled("POI must be scoped for Guard Routes / Glass Cutter / EMP to appear as preps");

	if (ImGui::Button("Apply Kortz Setup"))
		ApplyKortzSetup();
	ImGui::SameLine();
	if (ImGui::Button("Unlock All Paintings##kortz"))
		UnlockKortzPaintings();

	ImGui::Spacing();
	ImGui::SeparatorText("Extras");
	if (ImGui::Button("Remove Cooldown##kortz"))
		KortzRemoveCooldown();
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Clears the heist cooldown so you can replay immediately");
	ImGui::SameLine();
	if (ImGui::Button("Complete Buyer Requests##kortz"))
		KortzBuyerRequests();
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Marks all buyer-request bonus loot as satisfied");

	if (ImGui::Button("Weekly Boosted Payouts##kortz"))
		KortzWeeklyBoost();
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("MPX_WEEKLY_BOOST_BS = -1.\nServer-authoritative, so R* may revert it.");
	ImGui::SameLine();
	if (ImGui::Button("Max Approach Plays##kortz"))
		KortzMaxApproachPlays();
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sets all 10 per-approach completion counters\n(feeds the entry-point / approach-variety awards)");

	if (ImGui::TreeNode("Advanced (undocumented stats)##kortz"))
	{
		RenderKortzAdvanced();
		ImGui::TreePop();
	}

	ImGui::Spacing();
	ImGui::SeparatorText("In-Heist Actions (Kortz Center Cracker)");
	RenderKortzActions();
}

// ========================================================
// In-Heist Actions (skip drilling, hacking, etc.)
// These write to script locals in the active mission controller.
// ========================================================

// Cayo Perico mission controller: fm_mission_controller_2020
static constexpr uint32_t CAYO_MC_HASH = DMAScript::Joaat("fm_mission_controller_2020");
// Casino Heist mission controller: fm_mission_controller
static constexpr uint32_t CASINO_MC_HASH = DMAScript::Joaat("fm_mission_controller");

// b1158.13 in-heist locals -- all re-based from the pre-1.73 values noted inline.
void HeistSetup::SkipCayoHacking()
{
	if (DMAScript::WriteScriptLocal<int>(CAYO_MC_HASH, 26619, 5)) // was 26486
		StatusText = "Cayo hacking skipped!";
	else
		StatusText = "Failed (not in Cayo heist?)";
}

void HeistSetup::SkipCayoSewer()
{
	if (DMAScript::WriteScriptLocal<int>(CAYO_MC_HASH, 31511, 6)) // was 31349
		StatusText = "Cayo sewer cut skipped!";
	else
		StatusText = "Failed (not in Cayo heist?)";
}

void HeistSetup::SkipCayoGlass()
{
	if (DMAScript::WriteScriptLocal<float>(CAYO_MC_HASH, 32751 + 3, 100.0f)) // base was 32589
		StatusText = "Cayo glass cut skipped!";
	else
		StatusText = "Failed (not in Cayo heist?)";
}

void HeistSetup::SkipCasinoHacking()
{
	bool ok = DMAScript::WriteScriptLocal<int>(CASINO_MC_HASH, 55028, 5);       // was 54042
	ok = DMAScript::WriteScriptLocal<int>(CASINO_MC_HASH, 56098, 5) || ok;      // was 55108
	StatusText = ok ? "Casino hacking skipped!" : "Failed (not in Casino heist?)";
}

void HeistSetup::SkipCasinoDrilling()
{
	// Read the target value from local 10567+37, write to local 10567+7
	// b1158.13: base was 10551 pre-1.73.
	auto thread = DMAScript::FindScriptThread(CASINO_MC_HASH);
	if (!thread) { StatusText = "Failed (not in Casino heist?)"; return; }
	auto stack = DMAScript::GetScriptStack(thread);
	if (!stack) { StatusText = "Failed (no stack)"; return; }

	int targetVal = 0;
	DMAScript::ReadScriptLocal<int>(stack, 10567 + 37, targetVal);
	DMAScript::WriteScriptLocal<int>(stack, 10567 + 7, targetVal);
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

	if (ImGui::CollapsingHeader("Kortz Center Setup"))
	{
		ImGui::Indent();
		RenderKortzSetup();
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
