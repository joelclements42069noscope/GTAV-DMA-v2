#include "pch.h"
#include "BusinessFeatures.h"
#include "StatsWriter.h"
#include "PackedStats.h"
#include "TunableService.h"
#include "core/DMAScriptHelper.h"
#include "offsets/Offsets.h"

// ===========================================================================
// Script-global layout constants (LEGACY 1.x indices -- UNVERIFIED on Enhanced).
// Verify/correct these with the "Script Global Inspector" debug panel.
// ===========================================================================
namespace
{
	// dingus/bizteroids base for the business properties broadcast block.
	constexpr int BIZ_BASE   = 1845299 + 1 + 260;     // 1845560
	constexpr int WH_PROP    = BIZ_BASE + 128;        // warehouses
	constexpr int HANGAR_PROP= BIZ_BASE + 304;        // hangar
	constexpr int MC_PROP    = BIZ_BASE + 205;        // MC businesses
	constexpr int NC_PROP    = BIZ_BASE + 364;        // nightclub
	constexpr int SY_PROP    = BIZ_BASE + 504;        // salvage yard
	constexpr int CW_PROP    = 1882717 + 1 + 158 + 27;// hands-on car wash (money fronts)

	constexpr int HG_FILL_GLOBAL  = 1882707 + 7;      // Legacy (wrong on Enhanced) -- unused
	constexpr int HG_GOODS_GLOBAL = 1882707 + 8;      // hangar goods type (Legacy)
	constexpr int WH_FILL_GLOBAL  = 1882682 + 13;     // set 111 to fill warehouse stock (Legacy)
	constexpr int WH_GOODS_GLOBAL = 1882682 + 16;     // warehouse goods type (Legacy)

	// Hangar (from YimMenu GPBD_FM HANGAR_DATA struct, anchored to bizteroids HANGAR_PROP):
	//   HangarData.Index = HANGAR_PROP (1845864), TotalContraband = HANGAR_PROP+3 (1845867).
	// TotalContraband is the REAL sellable stock scalar; 1853814 was only a display count.
	constexpr int HANGAR_CONTRABAND  = HANGAR_PROP + 3; // 1845867 -- real stock
	constexpr int HANGAR_CARGO_COUNT = 1853814;         // displayed count (kept in sync for UI)
	constexpr int HANGAR_CARGO_MAX   = 50;

	// MC/bunker factories: FactoryInfos[i] (SCR_ARRAY<FACTORY_INFO,7> at MC_PROP).
	//   Index        = MC_PROP + 1 + i*13
	//   TotalProduct = MC_PROP + 2 + i*13   <- stock (what we fill)
	//   TotalSupplies= MC_PROP + 3 + i*13
	inline int FactoryProductIdx(int slot)  { return MC_PROP + 2 + slot * 13; }
	inline int FactorySuppliesIdx(int slot) { return MC_PROP + 3 + slot * 13; }

	constexpr int MC_SUPPLY_BASE  = 1673814;          // +1..+7 = supplies for each MC biz (Legacy)

	constexpr int GUARD_GLOBAL = 2655288;             // run only if != -1

	// Packed indices (version-dependent -- verify with Packed Debug).
	constexpr int PK_HANGAR        = 36828;
	constexpr int PK_WAREHOUSE_LO  = 32359, PK_WAREHOUSE_HI = 32363;
	constexpr int PK_SALVAGE_POP   = 51051;           // packed int = 100
	constexpr int PK_MFRONT_HEAT_LO= 24924, PK_MFRONT_HEAT_HI = 24926; // packed int = 0

	// ScriptGlobal :at(i, sz) == base + 1 + i*sz
	inline int GG(int idx) { return (int)DMA::GetGlobalInt(idx); }
	inline float GGf(int idx) { return DMA::GetGlobalFloat(idx); }
	inline int GGat(int base, int i, int sz) { return (int)DMA::GetGlobalInt(base + 1 + i * sz); }

	// MC business definitions (id -> threshold + manufacture/cost tunable hashes).
	struct McBiz { const char* name; std::vector<int> ids; int threshold; std::vector<int> tunables; };
	const McBiz MC_BIZ[] = {
		{ "meth", {1,6,11,16},          19, {1370024930, 1944848251, 1577999189, 1678460062, -730135062, -660914094} },
		{ "weed", {2,7,12,17},          79, {-635596193, -1694873660, 1575359233, 102029883, -373027461, 1195564032} },
		{ "coke", {3,8,13,18},           9, {702413484, 2070857577, -1539796661, 396217128, -161187879, 1500658261} },
		{ "cash", {4,9,14,19},          39, {1310272402, 1690071006, -1454958662, -1913260493, 631857857, -891680742} },
		{ "docs", {5,10,15,20},         59, {-959721585, 1672482518, -518264160, 489023341, -1839004359, -192060672} },
		{ "bunk", {21,22,23,24,25,26,27,28,29,30,31}, 99, {215868155, 631477612, 818645907, -1652502760, 1647327744} },
	};
	const int ACID_TUNABLES[] = { -672998848, 494316332, -40235252, -1506354854, -993236072 };
	// 7 nightclub goods tunables (one per linked business). Names are best-guess standard
	// in-game order -- if a wrong good fills, the order needs swapping (verify on rig).
	const int NC_TUNABLES[7]  = { -147565853, -1390027611, -1292210552, 1007184806, 18969287, -863328938, 1607981264 };
	const char* NC_GOOD_NAMES[7] = {
		"Cargo and Shipments",            // Special Cargo / CEO warehouse
		"Sporting Goods",                 // Bunker
		"South American Imports (Coke)",  // Cocaine Lockup
		"Pharmaceutical Research (Meth)", // Meth Lab
		"Organic Produce (Weed)",         // Weed Farm
		"Printing & Copying (Forgery)",   // Document Forgery
		"Cash Creation (Counterfeit)",    // Counterfeit Cash
	};

	void SetTunables(const int* hashes, size_t n, int& okOut)
	{
		for (size_t i = 0; i < n; i++)
			if (TunableService::SetTunableInt((uint32_t)hashes[i], 1)) okOut++;
	}
}

// ===========================================================================

void BusinessFeatures::DoRestock()
{
	int tunOk = 0, packedOk = 0, globalWrites = 0;
	std::string summary;

	const bool packed = bRestockIncludePacked;

	const bool ignore = bRestockIgnoreOwnership; // skip volatile-global ownership gates

	// ---- Hangar (VERIFIED: direct cargo-count write) + Warehouse (still Legacy/unverified) ----
	if (bRestockHangarWarehouse)
	{
		// Hangar: write real stock (TotalContraband) + display count to max.
		DMA::SetGlobalInt(HANGAR_CONTRABAND, HANGAR_CARGO_MAX); globalWrites++;
		DMA::SetGlobalInt(HANGAR_CARGO_COUNT, HANGAR_CARGO_MAX); globalWrites++;
		summary += "hangar(50) ";

		// Warehouse fill global is still the Legacy index (unverified on Enhanced) -- only
		// attempt under the global-writes opt-in until we capture the real one via diff.
		if (packed && (ignore || GG(WH_PROP + 1) >= 1))
		{
			if (bRestockSetGoodsType) { DMA::SetGlobalInt(WH_GOODS_GLOBAL, (DWORD)WarehouseGoodType); globalWrites++; }
			DMA::SetGlobalInt(WH_FILL_GLOBAL, 111); globalWrites++;
			for (int wh = PK_WAREHOUSE_LO; wh <= PK_WAREHOUSE_HI; wh++)
				if (PackedStats::SetPackedBool(wh, true)) packedOk++;
			summary += "warehouses(legacy) ";
		}
	}

	// ---- MC businesses (production tunables -- SAFE, the reliable core of the restocker) ----
	if (bRestockMC)
	{
		if (ignore)
		{
			// Volatile ownership globals can't be trusted -- just set every business's
			// production tunables (harmless if you don't own one).
			for (const auto& biz : MC_BIZ)
				SetTunables(biz.tunables.data(), biz.tunables.size(), tunOk);
			SetTunables(ACID_TUNABLES, std::size(ACID_TUNABLES), tunOk);
			summary += "all-mc+acid(tunables) ";
		}
		else
		{
			for (int i = 0; i <= 5; i++)
			{
				int slotId = GGat(MC_PROP, i, 13);
				if (slotId <= 0) continue;
				for (const auto& biz : MC_BIZ)
				{
					bool match = false;
					for (int id : biz.ids) if (slotId == id) { match = true; break; }
					if (match && GGat(MC_PROP + 1, i, 13) <= biz.threshold)
					{
						SetTunables(biz.tunables.data(), biz.tunables.size(), tunOk);
						summary += biz.name; summary += " ";
					}
				}
			}
			if (GGat(MC_PROP, 6, 13) >= 1 && GGat(MC_PROP + 1, 6, 13) <= 159)
			{
				SetTunables(ACID_TUNABLES, std::size(ACID_TUNABLES), tunOk);
				summary += "acid ";
			}
		}

		// MC supply globals are a fragile broadcast write -- only under the global opt-in.
		if (packed)
		{
			for (int s = 1; s <= 7; s++) { DMA::SetGlobalInt(MC_SUPPLY_BASE + s, 1); globalWrites++; }
			summary += "mc-supplies ";
		}
	}

	// ---- Nightclub (popularity stat + goods tunables -- SAFE, unconditional in ignore mode) ----
	if (bRestockNightclub)
	{
		if (ignore || GG(NC_PROP) >= 1)
		{
			StatsWriter::SetStatInt("MPX_CLUB_POPULARITY", 1000);
			for (int i = 0; i < 7; i++)
				if (NcGoodEnabled[i] && TunableService::SetTunableInt((uint32_t)NC_TUNABLES[i], 1))
					tunOk++;
			summary += "nightclub ";
		}
	}

	// ---- Salvage yard popularity + money-fronts heat (packed) ----
	if (bRestockSalvageMoneyFronts && packed)
	{
		if (ignore || GG(SY_PROP) >= 1)
		{
			if (PackedStats::SetPackedInt(PK_SALVAGE_POP, 100)) packedOk++;
			summary += "salvage ";
		}
		if (ignore || (GG(CW_PROP + 1) >= 1 && GG(CW_PROP + 13) >= 0))
		{
			for (int t = PK_MFRONT_HEAT_LO; t <= PK_MFRONT_HEAT_HI; t++)
				if (PackedStats::SetPackedInt(t, 0)) packedOk++;
			summary += "moneyfronts-heat ";
		}
	}

	RestockResult = std::format("Restock done. Tunables:{} Globals:{} Packed:{}{}. [{}]",
		tunOk, globalWrites, packedOk, packed ? "" : " (packed off)",
		summary.empty() ? "nothing matched -- verify business globals in debug" : summary);
	std::println("[Restocker] {}", RestockResult);
}

void BusinessFeatures::RunRestocker()
{
	RestockPending = true;
	RestockResult = "Restocker: queued...";
}

// Bunker / MOC research unlocks = weapon-component bitsets (from the verified UnlockEverything
// script) + the weaponized-vehicle / MkII / ammo / mine research items, which are packed bools
// in the Gunrunning DLC family (DLCGUNPSTAT_BOOL / GUNTATPSTAT_BOOL, all verified found=Y).
namespace
{
	const char* RESEARCH_WEAP_STATS[] = {
		"MPX_CHAR_WEAP_UNLOCKED",      "MPX_CHAR_WEAP_UNLOCKED2",
		"MPX_CHAR_WEAP_ADDON_1_UNLCK", "MPX_CHAR_WEAP_ADDON_2_UNLCK",
		"MPX_CHAR_WEAP_ADDON_3_UNLCK", "MPX_CHAR_WEAP_ADDON_4_UNLCK",
		"MPX_CHAR_FM_WEAP_UNLOCKED",   "MPX_CHAR_FM_WEAP_UNLOCKED2",
		"MPX_CHAR_FM_WEAP_UNLOCKED3",  "MPX_CHAR_FM_WEAP_UNLOCKED4",
		"MPX_CHAR_FM_WEAP_UNLOCKED5",  "MPX_CHAR_FM_WEAP_UNLOCKED6",
		"MPX_CHAR_WEAP_EQUIPPED",      "MPX_CHAR_FM_WEAP_EQUIPPED",
	};
	// Packed bool ranges covering the gunrunning RESEARCH items (weaponized vehicle mods,
	// MOC turrets, MkII weapon components/liveries, ammo types, proximity/kinetic mines).
	// A few cosmetic items share these ranges -- harmless to unlock alongside.
	const int RESEARCH_PACKED[][2] = {
		{ 15381, 15382 }, // APC SAM Battery, Ballistic Equipment
		{ 15425, 15439 }, // weaponized Tampa / Dune FAV / Insurgent / Technical
		{ 15447, 15474 }, // Oppressor missiles, MOC turrets, ammo types, MkII scopes/barrels, proximity mines
		{ 15456, 15460 }, // all Mk II ammo types (subset, explicit)
		{ 15491, 15499 }, // weaponized Tampa heavy armor, MkII weapon liveries
		{ 28099, 28148 }, // Signal Jammers (unlocks Combat MG + trade prices)
	};
}

void BusinessFeatures::RunFillHangar()
{
	// Write the REAL stock (HangarData.TotalContraband) + the display count to match.
	DMA::SetGlobalInt(HANGAR_CONTRABAND, HANGAR_CARGO_MAX);
	DMA::SetGlobalInt(HANGAR_CARGO_COUNT, HANGAR_CARGO_MAX);
	HangarResult = std::format("Set hangar TotalContraband (global {}) + display count to {}. "
		"This is the real stock field per the YimMenu struct -- test a sale to confirm payout.",
		HANGAR_CONTRABAND, HANGAR_CARGO_MAX);
	std::println("[Hangar] {}", HangarResult);
}

void BusinessFeatures::RunFillStock()
{
	// DISABLED: writing FactoryInfos.TotalProduct directly is reconciled by the business script
	// and consumes the player's supplies (destructive). Kept as a no-op; product must be made
	// via supplies + production tunables + manufacturing staff.
	StockResult = "Disabled -- direct product writes wipe supplies. Use supplies + production tunables.";
}

void BusinessFeatures::RunNightclubGoods()
{
	if (!TunableService::IsLoaded())
	{
		NcGoodsResult = "Nightclub goods: tunables.bin not loaded.";
		return;
	}
	int ok = 0, sel = 0;
	for (int i = 0; i < 7; i++)
	{
		if (!NcGoodEnabled[i]) continue;
		sel++;
		if (TunableService::SetTunableInt((uint32_t)NC_TUNABLES[i], 1)) ok++;
	}
	NcGoodsResult = std::format("Nightclub goods restocked: {}/{} selected applied.", ok, sel);
	std::println("[Nightclub] {}", NcGoodsResult);
}

void BusinessFeatures::RunUnlockResearch()
{
	if (!StatsWriter::IsReady())
	{
		ResearchResult = "Research: stats not ready (join a session).";
		return;
	}

	int statOk = 0;
	for (const char* s : RESEARCH_WEAP_STATS)
		if (StatsWriter::SetStatInt(s, -1)) statOk++;

	// Minigun weapon tints (gated by enemy kills with the minigun).
	StatsWriter::SetStatInt("MPX_MINIGUNS_ENEMY_KILLS", 600);

	int packedOk = 0, packedTotal = 0;
	for (const auto& rng : RESEARCH_PACKED)
	{
		packedTotal += (rng[1] - rng[0] + 1);
		packedOk += PackedStats::SetPackedBoolRange(rng[0], rng[1], true);
	}

	ResearchResult = std::format("Bunker/MOC research: weapon stats {}/{}, vehicle/MkII packed {}/{}.",
		statOk, (int)std::size(RESEARCH_WEAP_STATS), packedOk, packedTotal);
	std::println("[Research] {}", ResearchResult);
}

void BusinessFeatures::RunSkipCluckinBell()
{
	if (CluckState != CluckPhase::Idle) return;
	CluckState = CluckPhase::Step;
	CluckStep = 0;
	CluckNextMs = 0;
	CluckinResult = "Skip Cluckin' Bell: running...";
}

bool BusinessFeatures::OnDMAFrame()
{
	// ---- Nightclub save loop (every ~5s) ----
	if (bNightclubLoop && StatsWriter::IsReady())
	{
		ULONGLONG now = GetTickCount64();
		if (now >= NightclubNextMs)
		{
			NightclubNextMs = now + 5000;
			bool online = true;
			if (Offsets::IsSessionStarted)
				DMA::Read(DMA::BaseAddress + Offsets::IsSessionStarted, online);
			if (online)
			{
				StatsWriter::SetStatInt("MPX_CLUB_POPULARITY", 1000);
				StatsWriter::SetStatInt("MPX_CLUB_PAY_TIME_LEFT", -1);
				TunableService::SetTunableInt(DMAScript::Joaat("nightclubincomeuptopop100"), 250000);
			}
		}
	}

	// ---- Restocker one-shot ----
	if (RestockPending)
	{
		RestockPending = false;
		if (!StatsWriter::IsReady())
			RestockResult = "Restocker: stats not ready (join a session).";
		else if (GG(GUARD_GLOBAL) == -1)
			RestockResult = "Restocker: not ready (guard global == -1). Join a session and retry.";
		else
			DoRestock();
	}

	// ---- Skip Cluckin' Bell stepped sequence ----
	if (CluckState == CluckPhase::Step)
	{
		static const int kSteps[] = { 0, 1, 3, 7, 15, 31 };
		ULONGLONG now = GetTickCount64();
		if (now >= CluckNextMs)
		{
			if (CluckStep < (int)std::size(kSteps))
			{
				StatsWriter::SetStatInt("MPX_SALV23_INST_PROG", kSteps[CluckStep]);
				CluckStep++;
				CluckNextMs = now + 150; // ~script.yield(100), a touch slower for DMA
			}
			else
			{
				CluckState = CluckPhase::Idle;
				CluckinResult = "Skip Cluckin' Bell: done (set MPX_SALV23_INST_PROG through 31).";
				std::println("[SkipCluckinBell] done");
			}
		}
	}

	return true;
}

void BusinessFeatures::Render()
{
	if (!StatsWriter::IsReady())
	{
		ImGui::TextDisabled("Stats not available -- join a session (check console).");
		return;
	}

	// Nightclub save loop
	ImGui::Checkbox("Nightclub Max Popularity Loop", &bNightclubLoop);
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Every 5s: MPX_CLUB_POPULARITY=1000, MPX_CLUB_PAY_TIME_LEFT=-1, "
			"tunable nightclubincomeuptopop100=250000. (Reliable -- name-hashed stats/tunable.)");

	// Nightclub goods (per-type selection)
	if (ImGui::TreeNode("Nightclub Goods (pick types to restock)"))
	{
		ImGui::TextDisabled("Order is best-guess -- if the wrong good fills, tell Claude to swap it.");
		for (int i = 0; i < 7; i++)
			ImGui::Checkbox(NC_GOOD_NAMES[i], &NcGoodEnabled[i]);
		if (ImGui::Button("Restock Selected Nightclub Goods"))
			RunNightclubGoods();
		if (!NcGoodsResult.empty())
			ImGui::TextWrapped("%s", NcGoodsResult.c_str());
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
			"These set production RATE, not stock. After running, RE-ASSIGN your nightclub\n"
			"technicians (management screen) for the goods to fill -- same as the original script.");
		ImGui::TreePop();
	}

	ImGui::Separator();

	// Skip Cluckin' Bell
	const bool cluckRunning = (CluckState != CluckPhase::Idle);
	ImGui::BeginDisabled(cluckRunning);
	if (ImGui::Button(cluckRunning ? "Skipping..." : "Skip Cluckin' Bell Setups"))
		RunSkipCluckinBell();
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Steps MPX_SALV23_INST_PROG through 0,1,3,7,15,31 to clear the setup missions.");
	if (!CluckinResult.empty())
		ImGui::TextWrapped("%s", CluckinResult.c_str());

	ImGui::Separator();

	// Bunker / MOC research
	if (ImGui::Button("Unlock All Bunker / MOC Research"))
		RunUnlockResearch();
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Weapon-component bitsets (MkII, addons) + the weaponized-VEHICLE research\n"
			"packed unlocks (Tampa/Insurgent/Technical/Dune, MOC turrets, ammo types, mines).");
	if (!ResearchResult.empty())
		ImGui::TextWrapped("%s", ResearchResult.c_str());

	ImGui::Separator();

	// NOTE: direct product writes (FactoryInfos.TotalProduct) are DESTRUCTIVE -- the bunker/MC
	// script reconciles product against supplies on its next tick and CONSUMES your supplies.
	// Removed. Product must be produced the legit way (supplies + production tunables + staff).
	ImGui::TextDisabled("MC/Bunker product can't be set directly (game reconciles it & eats supplies).");
	ImGui::TextDisabled("Use: supplies full + production tunables + manufacturing staff -> it produces.");

	if (ImGui::Button("Fill Hangar Contraband (50)"))
		RunFillHangar();
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Writes HangarData.TotalContraband (global 1845867) -- the REAL stock field\n"
			"per the YimMenu struct (1853814 was only a display count). Test a sale to confirm.");
	if (!HangarResult.empty())
		ImGui::TextWrapped("%s", HangarResult.c_str());

	ImGui::Separator();

	// Restocker
	ImGui::Text("Business Restocker (bizteroids)");
	ImGui::TextDisabled("MC/Bunker/Acid/Nightclub = sets production tunables (may need a business\n"
		"restart to fill). Ownership globals are volatile, so 'Ignore ownership' is recommended.");
	ImGui::Checkbox("Ignore ownership checks (recommended)", &bRestockIgnoreOwnership);
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Business-ownership globals (GlobalPlayerBD) are volatile and often read 0,\n"
			"which made the restocker 'work then break'. With this ON, the production tunables\n"
			"+ nightclub popularity are applied unconditionally (harmless if you don't own one).");
	ImGui::Checkbox("Hangar + Warehouse (needs global writes ON)", &bRestockHangarWarehouse);
	ImGui::Checkbox("MC Businesses + Bunker + Acid Lab", &bRestockMC);
	ImGui::Checkbox("Nightclub goods + popularity", &bRestockNightclub);
	ImGui::Checkbox("Salvage Yard + Money Fronts (packed)", &bRestockSalvageMoneyFronts);
	ImGui::Checkbox("Include packed/global writes", &bRestockIncludePacked);
	ImGui::Checkbox("Force goods type", &bRestockSetGoodsType);
	if (bRestockSetGoodsType)
	{
		// Hangar (Air Freight) and Warehouse (Special Cargo) use DIFFERENT numbering (per bizteroids).
		// NOTE: sell value is QUANTITY-based, not type-based -- goods type is cosmetic, none is "best".
		static const char* hangarGoods =
			"0 Animal Materials\0" "1 Art & Antiques\0" "2 Chemicals\0" "3 Counterfeit Goods\0"
			"4 Jewelry & Gemstones\0" "5 Medical Supplies\0" "6 Narcotics\0" "7 Tobacco & Alcohol\0";
		static const char* warehouseGoods =
			"0 Medical Supplies\0" "1 Tobacco & Alcohol\0" "2 Art & Antiques\0" "3 Electronic Goods\0"
			"4 Weapons & Ammo\0" "5 Narcotics\0" "6 Gemstones\0" "7 Animal Materials\0"
			"8 Counterfeit Goods\0" "9 Jewelry\0" "10 Bullion\0";

		if (HangarGoodType < 0 || HangarGoodType > 7) HangarGoodType = 6;
		if (WarehouseGoodType < 0 || WarehouseGoodType > 10) WarehouseGoodType = 6;
		ImGui::SetNextItemWidth(220);
		ImGui::Combo("Hangar good", &HangarGoodType, hangarGoods);
		ImGui::SetNextItemWidth(220);
		ImGui::Combo("Warehouse good", &WarehouseGoodType, warehouseGoods);
		ImGui::TextDisabled("Sell value is quantity-based -- goods type is cosmetic.");
	}

	const bool restockBusy = RestockPending;
	ImGui::BeginDisabled(restockBusy);
	if (ImGui::Button(restockBusy ? "Restocking..." : "Run Restocker"))
		RunRestocker();
	ImGui::EndDisabled();
	if (!RestockResult.empty())
		ImGui::TextWrapped("%s", RestockResult.c_str());
}
