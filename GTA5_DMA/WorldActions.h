#pragma once
#include "DMA.h"
#include "PoolIterator.h"
#include "core/DMAScriptHelper.h"

// World actions: kill peds, kill enemies, destroy cameras via DMA pool iteration.

class WorldActions
{
public:
	// One-shot actions (called from UI buttons)
	static void KillAllPeds();
	static void KillAllEnemies();
	static void DestroyCameras();

	// Loop toggles (run every DMA frame)
	static inline bool bLoopKillPeds = false;
	static inline bool bLoopKillEnemies = false;
	static inline bool bLoopDestroyCameras = false;

	// Called from DMA thread loop
	static bool OnDMAFrame();

	// Status output for UI
	static inline std::string LastResult;

private:
	static inline ULONGLONG LastLoopTickMs = 0;

	// Camera model hashes (from YimMenu Object::IsCamera)
	static bool IsCameraHash(uint32_t hash);

	// Check if ped is hostile to local player via relationship group memory
	static bool IsEnemyPed(uintptr_t pedAddr);
};
