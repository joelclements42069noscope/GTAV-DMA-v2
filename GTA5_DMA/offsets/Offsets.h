#pragma once
#include <cstdint>

namespace Offsets
{
	// Runtime-populated by pattern scanner. Fall back to hard-coded if scanning fails.
	// Hard-coded defaults are from the last known working version.

	// Core pointers (resolved by pattern scanner)
	inline uintptr_t PedFactory = 0;       // CPedFactory** -- equivalent to old WorldPtr
	inline uintptr_t ScriptGlobals = 0;    // int64_t** -- script global array base
	inline uintptr_t BlipPtr = 0;          // BlipArray*

	// Network pointers
	inline uintptr_t NetworkPlayerMgr = 0; // CNetworkPlayerMgr**
	inline uintptr_t NetworkObjectMgr = 0; // CNetworkObjectMgr**
	inline uintptr_t IsSessionStarted = 0; // bool*
	inline uintptr_t NetworkTime = 0;      // uint32_t*

	// Entity pools
	inline uintptr_t PedPool = 0;          // PoolEncryption*
	inline uintptr_t VehiclePool = 0;      // fwVehiclePool***
	inline uintptr_t ObjectPool = 0;       // PoolEncryption*

	// Script system
	inline uintptr_t ScriptThreads = 0;    // rage::atArray<rage::scrThread*>*
	inline uintptr_t ScriptPrograms = 0;   // rage::scrProgram**

	// Misc
	inline uintptr_t GameVersion = 0;      // const char*
	inline uintptr_t GameTimer = 0;        // uint32_t*
	inline uintptr_t StatsMgr = 0;         // CStatsMgr*
	inline uintptr_t HasGTAPlus = 0;       // int*

	// Status
	inline bool Initialized = false;

	// Hard-coded fallback values (last known working offsets)
	namespace Fallback
	{
		static constexpr uintptr_t WorldPtr = 0x43dbc98;
		static constexpr uintptr_t GlobalPtr = 0x47c87a8;
		static constexpr uintptr_t BlipPtr = 0x3e7bf50;
	}

	// Apply fallback values if pattern scanning failed
	inline void ApplyFallbacks()
	{
		if (!PedFactory)
		{
			PedFactory = Fallback::WorldPtr;
			std::println("[Offsets] Using fallback PedFactory: {:X}", PedFactory);
		}
		if (!ScriptGlobals)
		{
			ScriptGlobals = Fallback::GlobalPtr;
			std::println("[Offsets] Using fallback ScriptGlobals: {:X}", ScriptGlobals);
		}
		if (!BlipPtr)
		{
			BlipPtr = Fallback::BlipPtr;
			std::println("[Offsets] Using fallback BlipPtr: {:X}", BlipPtr);
		}
	}
}
