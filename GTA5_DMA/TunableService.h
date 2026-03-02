#pragma once
#include <cstdint>
#include <unordered_map>
#include <string>

class TunableService
{
public:
	// Load tunables from YimMenu's tunables.bin cache file
	// Returns true if successfully loaded
	static bool LoadFromCache(const std::string& filePath = "tunables.bin");

	// Check if the tunable service has loaded data
	static bool IsLoaded() { return bLoaded; }

	// Get the global index for a tunable by its JOAAT hash
	// Returns 0 if not found
	static DWORD GetTunableGlobalIndex(uint32_t hash);

	// Set a tunable value (int)
	static bool SetTunableInt(uint32_t hash, int value);

	// Set a tunable value (float)
	static bool SetTunableFloat(uint32_t hash, float value);

	// Get a tunable value (int)
	static bool GetTunableInt(uint32_t hash, int& out);

	// Get a tunable value (float)
	static bool GetTunableFloat(uint32_t hash, float& out);

	// Dump a few sample hashes for diagnostics
	static void DumpSampleHashes();

	// Scan all tunable globals for a specific float value, returns matching global indices
	static std::vector<DWORD> ScanForFloat(float targetValue, float tolerance = 0.01f);

	// Set a tunable by direct global index (bypass hash lookup)
	static bool SetGlobalFloat(DWORD globalIndex, float value);

	static inline int TunableCount = 0;

private:
	static constexpr int TUNABLE_BASE_ADDRESS = 0x40001; // 262145

	static inline bool bLoaded = false;
	static inline std::unordered_map<uint32_t, uint32_t> TunableMap; // hash -> offset

#pragma pack(push, 1)
	struct CacheHeader
	{
		uint32_t m_CacheVersion;
		uint32_t m_FileVersion;
		uint64_t m_DataSize;
	};

	struct TunableSaveStruct
	{
		uint32_t m_Hash;
		uint32_t m_Offset;
	};
#pragma pack(pop)
};
