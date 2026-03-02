#pragma once

// Heist Cut Manipulation
// Writes custom cut percentages and force-ready via ScriptGlobal writes
// Ported from YimMenu V2: src/game/features/recovery/Heist/

class HeistCuts
{
public:
	// Cayo Perico
	static inline bool bCayoSetCuts = false;
	static inline bool bCayoForceReady = false;
	static inline int CayoCuts[4] = { 85, 5, 5, 5 }; // Default: host gets 85%

	// Diamond Casino
	static inline bool bCasinoSetCuts = false;
	static inline bool bCasinoForceReady = false;
	static inline int CasinoCuts[4] = { 85, 5, 5, 5 };

	static bool OnDMAFrame();

	// UI helpers
	static void Render();

private:
	static void SetCayoCuts();
	static void CayoForceReady();
	static void SetCasinoCuts();
	static void CasinoForceReady();

	// ScriptGlobal index helper: replaces ScriptGlobal(base).At(offset)
	// .At(offset) = base + offset
	// .At(index, size) = base + 1 + index * size
	static DWORD GlobalAt(DWORD base, int offset) { return base + offset; }
	static DWORD GlobalAtArray(DWORD base, int index, int elemSize) { return base + 1 + index * elemSize; }
};
