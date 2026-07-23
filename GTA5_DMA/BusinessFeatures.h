#pragma once

// Business features ported from dinguscool4 / ImagineNothing YimMenu Lua scripts:
//   - Nightclub save loop      (nightclub save loop.lua)        -- popularity + safe income
//   - Skip Cluckin' Bell setup (skip cluckin bell.lua)          -- MPX_SALV23_INST_PROG steps
//   - Business Restocker       (bizteroids.lua / dingus restocker) -- fill hangar/WH/MC/NC/SY
//
// Reliable (version-independent): nightclub stats + income tunable, cluckin-bell stat steps,
//   and the restocker's tunable writes (tunables are hashed by name).
// UNVERIFIED on Enhanced build: the business ScriptGlobal indices (Legacy 1.x constants) used
//   to detect ownership/stock, and the packed-stat indices. Verify with the Script Global /
//   Tunable / Packed Debug panels before trusting; restocker packed + global writes are gated.

class BusinessFeatures
{
public:
	// --- Nightclub save loop (block runs every ~5s while enabled) ---
	static inline bool bNightclubLoop = false;

	// --- Restocker options (mirror the bizteroids control panel) ---
	static inline bool bRestockHangarWarehouse = true;
	static inline bool bRestockMC = true;            // resupply + restock MC businesses
	static inline bool bRestockNightclub = true;
	static inline bool bRestockSalvageMoneyFronts = true;
	static inline bool bRestockIncludePacked = false; // packed/global writes (unverified indices)
	// Business-ownership globals (GlobalPlayerBD) are VOLATILE -- they only populate in certain
	// session states, so gating on them causes "worked then broke". Default ON = skip the gates
	// and apply the (harmless) production tunables + popularity unconditionally.
	static inline bool bRestockIgnoreOwnership = true;
	static inline bool bRestockSetGoodsType = false;  // force a specific goods type
	static inline int  HangarGoodType = 6;
	static inline int  WarehouseGoodType = 6;

	// Per-good nightclub restock selection (7 goods, one per linked business).
	// Order is best-guess (standard in-game order) -- verify on rig and correct labels.
	static inline bool NcGoodEnabled[7] = { true, true, true, true, true, true, true };

	static void RunNightclubGoods();   // restock only the selected nightclub goods

	static bool OnDMAFrame();          // nightclub loop + run state machines
	static void RunRestocker();        // one-shot
	static void RunSkipCluckinBell();  // one-shot (stepped)
	static void RunUnlockResearch();   // bunker/MOC research: weapon bitsets + vehicle packed unlocks
	static void RunFillHangar();       // write hangar contraband (real) + count to max
	static void RunFillStock();        // set MC/bunker/acid product (FactoryInfos.TotalProduct) to max
	static void Render();

	static inline std::string RestockResult;
	static inline std::string CluckinResult;
	static inline std::string ResearchResult;
	static inline std::string NcGoodsResult;
	static inline std::string HangarResult;
	static inline std::string StockResult;

private:
	static void DoRestock();

	// Skip Cluckin' Bell stepped sequence state
	enum class CluckPhase { Idle, Step };
	static inline CluckPhase CluckState = CluckPhase::Idle;
	static inline int CluckStep = 0;
	static inline ULONGLONG CluckNextMs = 0;

	static inline bool RestockPending = false;
	static inline ULONGLONG NightclubNextMs = 0;
};
