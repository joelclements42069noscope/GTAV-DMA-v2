#include "pch.h"
#include "TunableService.h"
#include "offsets/Offsets.h"

bool TunableService::LoadFromCache(const std::string& filePath)
{
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open())
	{
		std::println("[Tunables] Could not open {}", filePath);
		return false;
	}

	// Read cache header (16 bytes)
	CacheHeader header{};
	file.read(reinterpret_cast<char*>(&header), sizeof(CacheHeader));
	if (!file.good())
	{
		std::println("[Tunables] Failed to read cache header");
		return false;
	}

	// Read payload data
	std::vector<uint8_t> data(header.m_DataSize);
	file.read(reinterpret_cast<char*>(data.data()), header.m_DataSize);
	if (!file.good())
	{
		std::println("[Tunables] Failed to read payload (expected {} bytes)", header.m_DataSize);
		return false;
	}

	file.close();

	// Parse payload: first 4 bytes = count, then TunableSaveStruct[]
	if (data.size() < sizeof(uint32_t))
	{
		std::println("[Tunables] Payload too small");
		return false;
	}

	uint32_t count = *reinterpret_cast<uint32_t*>(data.data());
	size_t expectedSize = sizeof(uint32_t) + count * sizeof(TunableSaveStruct);

	if (data.size() < expectedSize)
	{
		std::println("[Tunables] Payload size mismatch: have {}, need {}", data.size(), expectedSize);
		return false;
	}

	TunableMap.clear();
	TunableMap.reserve(count);

	const TunableSaveStruct* entries = reinterpret_cast<const TunableSaveStruct*>(data.data() + sizeof(uint32_t));
	for (uint32_t i = 0; i < count; i++)
	{
		TunableMap[entries[i].m_Hash] = entries[i].m_Offset;
	}

	TunableCount = static_cast<int>(count);
	LoadedFileVersion = header.m_FileVersion;
	bLoaded = true;
	std::println("[Tunables] Loaded {} tunables from cache (header: ver={}, fileVer={}, dataSize={})",
		count, header.m_CacheVersion, header.m_FileVersion, header.m_DataSize);

	// Show first 3 entries for verification
	for (uint32_t i = 0; i < count && i < 3; i++)
		std::println("[Tunables]   entry[{}]: hash={:08X} offset={}", i, entries[i].m_Hash, entries[i].m_Offset);

	return true;
}

DWORD TunableService::GetTunableGlobalIndex(uint32_t hash)
{
	auto it = TunableMap.find(hash);
	if (it == TunableMap.end())
		return 0;

	// Cache stores FULL global index (base + offset), not just the offset
	return it->second;
}

bool TunableService::SetTunableInt(uint32_t hash, int value)
{
	DWORD globalIndex = GetTunableGlobalIndex(hash);
	if (!globalIndex)
		return false;

	return DMA::SetGlobalInt(globalIndex, static_cast<DWORD>(value));
}

bool TunableService::SetTunableFloat(uint32_t hash, float value)
{
	DWORD globalIndex = GetTunableGlobalIndex(hash);
	if (!globalIndex)
		return false;

	return DMA::SetGlobalFloat(globalIndex, value);
}

bool TunableService::GetTunableInt(uint32_t hash, int& out)
{
	DWORD globalIndex = GetTunableGlobalIndex(hash);
	if (!globalIndex)
		return false;

	return DMA::GetGlobalValue(globalIndex, out);
}

bool TunableService::GetTunableFloat(uint32_t hash, float& out)
{
	DWORD globalIndex = GetTunableGlobalIndex(hash);
	if (!globalIndex)
		return false;

	return DMA::GetGlobalValue(globalIndex, out);
}

void TunableService::DumpSampleHashes()
{
	std::println("[Tunables] Map contains {} entries", TunableMap.size());
	int shown = 0;
	for (auto& [hash, offset] : TunableMap)
	{
		if (shown >= 5) break;
		std::println("[Tunables]   hash={:08X} -> globalIdx={}", hash, offset);
		shown++;
	}

	// Specifically check XP_MULTIPLIER hash
	auto it = TunableMap.find(0xB6E46AB9);
	if (it != TunableMap.end())
		std::println("[Tunables] XP_MULTIPLIER (B6E46AB9) FOUND -> globalIdx={}", it->second);
	else
		std::println("[Tunables] XP_MULTIPLIER (B6E46AB9) NOT FOUND in map");

	// Also check IDLEKICK_WARNING1 hash (for cross-reference)
	auto it2 = TunableMap.find(0xB3A4D684);
	if (it2 != TunableMap.end())
		std::println("[Tunables] IDLEKICK_WARNING1 (B3A4D684) FOUND -> globalIdx={}", it2->second);
	else
		std::println("[Tunables] IDLEKICK_WARNING1 (B3A4D684) NOT FOUND in map");
}

std::vector<DWORD> TunableService::ScanForFloat(float targetValue, float tolerance)
{
	std::vector<DWORD> matches;
	if (!bLoaded || !Offsets::ScriptGlobals)
		return matches;

	std::println("[Tunables] Scanning {} tunables for float value {:.1f} (tolerance {:.2f})...",
		TunableMap.size(), targetValue, tolerance);

	int scanned = 0;
	for (auto& [hash, globalIdx] : TunableMap)
	{
		float val = 0.0f;
		if (DMA::GetGlobalValue(globalIdx, val))
		{
			if (std::abs(val - targetValue) <= tolerance)
			{
				std::println("[Tunables]   MATCH: hash={:08X} globalIdx={} value={:.2f}", hash, globalIdx, val);
				matches.push_back(globalIdx);
			}
		}
		scanned++;
	}

	std::println("[Tunables] Scan complete: {} matches found (scanned {})", matches.size(), scanned);
	return matches;
}

bool TunableService::SetGlobalFloat(DWORD globalIndex, float value)
{
	return DMA::SetGlobalFloat(globalIndex, value);
}

bool TunableService::ValidateFreshness()
{
	LikelyStale = false;
	if (!bLoaded || !Offsets::ScriptGlobals)
		return true; // nothing to validate against

	// tunables.bin maps hash -> FULL global index. Those indices shift with
	// every game title update, so a bin built on another build points its
	// entries at the wrong globals. We can't know the build directly, but we
	// can read a few well-known tunables and check they still hold plausible
	// values. If none do, the bin is almost certainly stale.
	int plausible = 0, probed = 0;

	// XP_MULTIPLIER -- a float multiplier, realistically in (0, 10000].
	if (DWORD gi = GetTunableGlobalIndex(0xB6E46AB9)) // JOAAT("XP_MULTIPLIER")
	{
		float f = 0.0f;
		probed++;
		if (DMA::GetGlobalValue(gi, f) && std::isfinite(f) && f > 0.0f && f <= 10000.0f)
			plausible++;
		std::println("[Tunables] Freshness probe XP_MULTIPLIER -> globalIdx={} value={:.3f}", gi, f);
	}

	// IDLEKICK_WARNING1 -- an int timeout in ms, realistically (0, 3_600_000].
	if (DWORD gi = GetTunableGlobalIndex(0xB3A4D684)) // JOAAT("IDLEKICK_WARNING1")
	{
		int v = 0;
		probed++;
		if (DMA::GetGlobalValue(gi, v) && v > 0 && v <= 3'600'000)
			plausible++;
		std::println("[Tunables] Freshness probe IDLEKICK_WARNING1 -> globalIdx={} value={}", gi, v);
	}

	if (probed > 0 && plausible == 0)
	{
		LikelyStale = true;
		std::println("[Tunables] *** WARNING: tunables.bin looks STALE for this game build ***");
		std::println("[Tunables]   None of the probed tunables hold plausible values, which means");
		std::println("[Tunables]   its hash->globalIndex map no longer matches the running game.");
		std::println("[Tunables]   RP Multiplier / No Idle Kick / Free Appearance Change will not work.");
		std::println("[Tunables]   Regenerate tunables.bin from a current YimMenu (see HOW_TO_GET_TUNABLES.txt).");
	}
	else
	{
		std::println("[Tunables] Freshness check: {}/{} probes plausible -- data looks OK.", plausible, probed);
	}

	return !LikelyStale;
}
