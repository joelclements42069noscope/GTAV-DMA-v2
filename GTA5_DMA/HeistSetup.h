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

	// Status
	static inline const char* StatusText = "";
};
