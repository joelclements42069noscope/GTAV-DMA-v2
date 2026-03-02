#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Direct CStatsMgr reader/writer over DMA.
// Finds stats by JOAAT hash via binary search in the game's sorted stat array,
// then reads/writes the XOR-encoded value in the sStatData object.
//
// sStatData layout (0x20 bytes):
//   0x00: vtable*    (8 bytes)
//   0x08: m_Flags    (uint32_t, 4 bytes)
//   0x0C: field_0C   (4 bytes)
//   0x10: m_Value    (uint32_t, XOR-encoded: stored = value ^ low32(&m_Value))
//   0x14: field_14   (4 bytes)
//   0x18: m_pNext    (8 bytes, linked list to next sStatData)
//
// XOR encoding (from vtable disassembly):
//   GetInt: lea rax,[rcx+10]; xor eax,[rcx+10]; ret → value = stored ^ low32(this+0x10)
//   SetInt: lea rax,[rcx+10]; xor eax,edx; mov [rcx+10],eax → stored = low32(this+0x10) ^ value

class StatsWriter
{
public:
	// Lazy initialization. Reads CStatsMgr and caches the stat map.
	// Safe to call repeatedly; only initializes once.
	static bool EnsureInitialized();

	// Set/Get int stat by name. Handles MPX_ prefix (resolved to MP0_/MP1_).
	static bool SetStatInt(const std::string& statName, int value);
	static int GetStatInt(const std::string& statName, int fallback = 0);

	// Diagnostic: dump 32 bytes around a stat's sStatData for offset discovery
	static void DiagnoseStat(const std::string& statName);

	// Returns true if CStatsMgr is ready. Retries initialization every 2 seconds.
	static bool IsReady()
	{
		if (bInitialized) return true;
		ULONGLONG now = GetTickCount64();
		if (now - LastInitRetryMs < 2000) return false;
		LastInitRetryMs = now;
		return EnsureInitialized();
	}
	static int GetStatCount() { return (int)CachedStats.size(); }
	static int GetCharIndex() { return CachedCharIndex; }

	// Public so HeistSetup can use it for verification reads
	static uintptr_t FindStatDataPtr(uint32_t hash);

private:
	// sStatMap as stored in game memory (16 bytes each)
	struct StatMapEntry
	{
		uint32_t Hash;      // 0x00
		uint32_t Unk;       // 0x04
		uintptr_t DataPtr;  // 0x08 - pointer to sStatData
	};
	static_assert(sizeof(StatMapEntry) == 0x10);

	static std::string ResolveMPX(const std::string& name);
	static int GetCharacterIndex();

	// CStatsMgr layout
	static constexpr int CSTATSMGR_INITIALIZED = 0x00;
	static constexpr int CSTATSMGR_STATS_ARRAY = 0x08; // atArray<sStatMap>

	static inline std::vector<StatMapEntry> CachedStats;
	static inline int CachedCharIndex = -1;
	static inline bool bInitialized = false;
	static inline ULONGLONG LastInitRetryMs = 0;
};
