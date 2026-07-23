#pragma once

// Heist Setup / Recovery via stat writes
// Sets up Casino Heist and Cayo Perico heist parameters by writing MPX_H3OPT_* / MPX_H4CNF_* stats.
// All stat writes go through StatsWriter (CStatsMgr direct memory write).

class HeistSetup
{
public:
	static void Render(); // ImGui UI

private:
	// --- Diamond Casino Heist ---
	static void RenderCasinoSetup();
	static void ApplyCasinoSetup();
	static void SkipCasinoHacking();
	static void SkipCasinoDrilling();

	// Casino UI state
	static inline int CasinoApproach = 0;    // 0=Silent, 1=BigCon, 2=Aggressive
	static inline int CasinoTarget = 0;      // 0=Diamonds, 1=Gold, 2=Art, 3=Cash (UI order, mapped to stat value)
	static inline int CasinoGunman = 0;      // UI index -> stat value mapped in Apply
	static inline int CasinoDriver = 0;
	static inline int CasinoHacker = 0;      // 0=Avi, 1=Paige, 2=Christian, 3=Yohan, 4=Rickie
	static inline int CasinoWeapon = 0;
	static inline int CasinoDifficulty = 0;  // 0=Normal, 1=Hard

	// --- Cayo Perico ---
	static void RenderCayoSetup();
	static void ApplyCayoSetup();
	static void ApplyCayoTakeOverride();
	static void SkipCayoHacking();
	static void SkipCayoSewer();
	static void SkipCayoGlass();

	// Cayo UI state
	static inline int CayoTarget = 0;        // 0=Panther, 1=PinkDiamond, 2=MadrazoFiles, 3=Bearer, 4=Ruby, 5=Tequila
	static inline int CayoWeapon = 0;        // 0=Aggressor, 1=Conspirator, 2=CrackShot, 3=Saboteur, 4=Marksman
	static inline int CayoDifficulty = 0;    // 0=Normal, 1=Hard

	// Cayo take value overrides (use during heist finale)
	static inline int CayoSecondaryTakeValue = 2000000;
	static inline int CayoPrimaryTargetValue = 2500000;

	// --- Kortz Center (K26) ---
	// Instant setup via MPX_K26_* stats (all named/hash-based, so version-
	// independent like the Cayo/Casino setups). Bit-granular: mirrors the
	// community KortzCenterHeist setup -- start each bitset at -1 (all set) and
	// clear the boxes left unchecked. See newstuff.txt for the bit map.
	static void RenderKortzSetup();
	static void ApplyKortzSetup();
	static void UnlockKortzPaintings();

	// --- Kortz in-heist actions ("Kortz Center Cracker", fm_mission_controller_v3) ---
	static void RenderKortzActions();

	// --- Kortz extras (documented K26 stats not covered by the setup) ---
	static void KortzRemoveCooldown();
	static void KortzWeeklyBoost();
	static void KortzBuyerRequests();
	static void KortzMaxApproachPlays();

	// --- Kortz advanced: the remaining K26 stats whose value semantics aren't
	// documented. Exposed as live-read + editable-write so they can be probed
	// in-game rather than guessed. See KORTZ_ADV_STATS in the .cpp.
	static void RenderKortzAdvanced();
	static void KortzReadAdvanced();          // refresh live values
	static constexpr int KORTZ_ADV_COUNT = 9;
	static inline int  KortzAdvWrite[KORTZ_ADV_COUNT] = { 0, 0, 0, 0, 0, -1, 0, 0, -1 };
	static inline int  KortzAdvLive[KORTZ_ADV_COUNT] = {};
	static inline bool KortzAdvHaveRead = false;
	static void KortzSkipDataCrack();
	static void KortzSkipFingerprint();
	static void KortzEnterAccessCode();
	static void KortzDisableLasers();
	static void KortzSkipVaultHacking();
	static void KortzTakePrimaryTarget();
	static void KortzTakeSecondaryTarget();
	static void KortzCutGlass();
	static void KortzSoloSecondaryTargets();

	// Kortz UI state.
	static inline int KortzPrimaryTarget = 0;   // 0..26 (MPX_K26_HEIST_TARGET)

	// Mutually-exclusive loadout (GENERAL_BS bits 9/10/11).
	// 0=None, 1=Street, 2=Security, 3=Military.
	static inline int KortzLoadoutType = 2;

	// Mutually-exclusive Manchez colour (GENERAL_BS bits 17/18/19/20).
	// 0=Red, 1=Blue, 2=Green, 3=Yellow. Only applied when the Manchez prep is on.
	static inline int KortzManchezColor = 0;

	// Extra GENERAL_BS flags.
	static inline bool KortzManholeKey = true;  // bit 27 -- required for sewer entry
	static inline bool KortzHardMode   = false; // bit 28
	static inline bool KortzWeakGuards = true;  // bit 31

	// General purchases (MPX_K26_GENERAL_BS bits 5-8).
	static inline bool KortzGuardRoutes  = true;
	static inline bool KortzGlassCutter  = true;
	static inline bool KortzPowerDrills  = true;
	static inline bool KortzEmpCharges   = true;

	// Prep work (MPX_K26_ROBBERY_PROG bits 0-15).
	static inline bool KortzPrep[16] = {
		true, true, true, true, true, true, true, true,
		true, true, true, true, true, true, true, true
	};

	// Scoping (MPX_K26_SCOPING_BS / MPX_K26_POI_BS: -1 when set, else 0).
	static inline bool KortzScopeSecondary = true;
	static inline bool KortzScopePoi       = true;

	// Status
	static inline const char* StatusText = "";
};
