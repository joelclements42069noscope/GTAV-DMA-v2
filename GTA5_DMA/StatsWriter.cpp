#include "pch.h"
#include "StatsWriter.h"
#include "core/DMAScriptHelper.h"
#include "offsets/Offsets.h"

bool StatsWriter::EnsureInitialized()
{
	if (bInitialized)
		return true;

	if (!Offsets::StatsMgr)
	{
		static bool warned = false;
		if (!warned) { std::println("[StatsWriter] StatsMgr offset not resolved (pattern scan failed)"); warned = true; }
		return false;
	}

	uintptr_t statsMgrAddr = DMA::BaseAddress + Offsets::StatsMgr;

	// Check if CStatsMgr is initialized
	bool initialized = false;
	if (!DMA::Read(statsMgrAddr + CSTATSMGR_INITIALIZED, initialized) || !initialized)
	{
		static int retryCount = 0;
		if (retryCount++ % 10 == 0)
			std::println("[StatsWriter] CStatsMgr not initialized yet @ {:X} (retry #{})", statsMgrAddr, retryCount);
		return false;
	}

	// Read atArray header
	DMAScript::DMA_atArrayHeader header{};
	if (!DMA::Read(statsMgrAddr + CSTATSMGR_STATS_ARRAY, header))
	{
		std::println("[StatsWriter] Failed to read atArray header from {:X}", statsMgrAddr + CSTATSMGR_STATS_ARRAY);
		return false;
	}

	if (!header.m_Data || header.m_Size == 0)
	{
		std::println("[StatsWriter] atArray empty (data={:X}, size={}, capacity={})",
			header.m_Data, header.m_Size, header.m_Capacity);
		return false;
	}

	std::println("[StatsWriter] CStatsMgr @ {:X}, atArray: data={:X} size={} capacity={}",
		statsMgrAddr, header.m_Data, header.m_Size, header.m_Capacity);

	// Read the entire stat map array into local cache
	CachedStats.resize(header.m_Size);
	DWORD totalBytes = header.m_Size * sizeof(StatMapEntry);
	DWORD bytesRead = 0;
	VMMDLL_MemReadEx(DMA::vmh, DMA::PID, header.m_Data,
		(BYTE*)CachedStats.data(), totalBytes, &bytesRead, VMMDLL_FLAG_NOCACHE);

	if (bytesRead != totalBytes)
	{
		std::println("[StatsWriter] Failed to read stat map array ({}/{} bytes)", bytesRead, totalBytes);
		CachedStats.clear();
		return false;
	}

	bInitialized = true;
	std::println("[StatsWriter] Cached {} stats from CStatsMgr", CachedStats.size());

	// Cache character index
	CachedCharIndex = GetCharacterIndex();
	std::println("[StatsWriter] Character index: {}", CachedCharIndex);

	// Diagnostic: verify XOR decoding by reading a known stat
	DiagnoseStat("MPPLY_LAST_MP_CHAR");

	return true;
}

std::string StatsWriter::ResolveMPX(const std::string& name)
{
	std::string resolved = name;
	for (auto& c : resolved)
		c = (char)tolower((unsigned char)c);

	if (resolved.length() > 3 && resolved[0] == 'm' && resolved[1] == 'p' && resolved[2] == 'x')
	{
		if (CachedCharIndex < 0)
			CachedCharIndex = GetCharacterIndex();
		resolved[2] = '0' + CachedCharIndex;
	}

	return resolved;
}

int StatsWriter::GetCharacterIndex()
{
	// MPPLY_LAST_MP_CHAR is a global stat (no MPX prefix)
	uint32_t hash = DMAScript::Joaat("mpply_last_mp_char");
	uintptr_t dataPtr = FindStatDataPtr(hash);
	if (!dataPtr)
	{
		std::println("[StatsWriter] MPPLY_LAST_MP_CHAR not found, defaulting to 0");
		return 0;
	}

	// Value at +0x10 is XOR-encoded: stored = value ^ low32(dataPtr + 0x10)
	uint32_t raw = 0;
	if (!DMA::Read(dataPtr + 0x10, raw))
		return 0;

	uint32_t key = (uint32_t)((dataPtr + 0x10) & 0xFFFFFFFF);
	int value = (int)(raw ^ key);

	std::println("[StatsWriter] MPPLY_LAST_MP_CHAR: raw={:08X} key={:08X} decoded={}", raw, key, value);

	if (value < 0 || value > 1)
	{
		std::println("[StatsWriter] MPPLY_LAST_MP_CHAR = {} (unexpected), defaulting to 0", value);
		return 0;
	}

	return value;
}

uintptr_t StatsWriter::FindStatDataPtr(uint32_t hash)
{
	if (CachedStats.empty())
		return 0;

	// Binary search (array is sorted by hash)
	int lo = 0, hi = (int)CachedStats.size() - 1;
	while (lo <= hi)
	{
		int mid = lo + (hi - lo) / 2;
		if (CachedStats[mid].Hash == hash)
			return CachedStats[mid].DataPtr;
		else if (CachedStats[mid].Hash < hash)
			lo = mid + 1;
		else
			hi = mid - 1;
	}

	return 0; // Not found
}

bool StatsWriter::SetStatInt(const std::string& statName, int value)
{
	if (!EnsureInitialized())
		return false;

	std::string resolved = ResolveMPX(statName);
	uint32_t hash = DMAScript::Joaat(resolved.c_str());

	uintptr_t dataPtr = FindStatDataPtr(hash);
	if (!dataPtr)
	{
		static int warnCount = 0;
		if (warnCount++ < 20)
			std::println("[StatsWriter] Stat '{}' (hash {:08X}) not found in CStatsMgr", resolved, hash);
		return false;
	}

	// Value at +0x10 is XOR-encoded: stored = value ^ low32(dataPtr + 0x10)
	// This matches the game's virtual SetInt:
	//   lea rax, [rcx+0x10]   ; rax = this + 0x10
	//   xor eax, edx          ; eax = low32(this+0x10) ^ value
	//   mov [rcx+0x10], eax   ; store encoded value
	uint32_t key = (uint32_t)((dataPtr + 0x10) & 0xFFFFFFFF);
	uint32_t encoded = (uint32_t)value ^ key;
	bool ok = DMA::Write(dataPtr + 0x10, encoded);

	// Follow linked list and write with per-entry XOR keys
	uintptr_t current = dataPtr;
	for (int depth = 0; depth < 4; depth++)
	{
		uintptr_t nextPtr = 0;
		if (!DMA::Read(current + 0x18, nextPtr) || nextPtr < 0x10000 || nextPtr == dataPtr)
			break;

		uintptr_t vtable = 0;
		if (!DMA::Read(nextPtr, vtable) || (vtable >> 32) < 0x7FF0 || (vtable >> 32) > 0x7FFF)
			break;

		uint32_t linkKey = (uint32_t)((nextPtr + 0x10) & 0xFFFFFFFF);
		uint32_t linkEncoded = (uint32_t)value ^ linkKey;
		DMA::Write(nextPtr + 0x10, linkEncoded);
		current = nextPtr;
	}

	return ok;
}

int StatsWriter::GetStatInt(const std::string& statName, int fallback)
{
	if (!EnsureInitialized())
		return fallback;

	std::string resolved = ResolveMPX(statName);
	uint32_t hash = DMAScript::Joaat(resolved.c_str());

	uintptr_t dataPtr = FindStatDataPtr(hash);
	if (!dataPtr)
		return fallback;

	uint32_t raw = 0;
	if (!DMA::Read(dataPtr + 0x10, raw))
		return fallback;

	uint32_t key = (uint32_t)((dataPtr + 0x10) & 0xFFFFFFFF);
	return (int)(raw ^ key);
}

void StatsWriter::DiagnoseStat(const std::string& statName)
{
	std::string resolved = ResolveMPX(statName);
	uint32_t hash = DMAScript::Joaat(resolved.c_str());

	uintptr_t dataPtr = FindStatDataPtr(hash);
	if (!dataPtr)
	{
		std::println("[StatsWriter] Diagnose: '{}' not found (hash {:08X})", resolved, hash);
		return;
	}

	// Read 48 bytes from the sStatData pointer (covers base class + derived fields)
	uint8_t bytes[48] = {};
	DWORD bytesRead = 0;
	VMMDLL_MemReadEx(DMA::vmh, DMA::PID, dataPtr, bytes, 48, &bytesRead, VMMDLL_FLAG_NOCACHE);

	uintptr_t vtable = *reinterpret_cast<uintptr_t*>(&bytes[0]);
	uint32_t flags = *reinterpret_cast<uint32_t*>(&bytes[8]);

	// XOR decode: value = stored ^ low32(dataPtr + 0x10)
	uint32_t raw10 = *reinterpret_cast<uint32_t*>(&bytes[0x10]);
	uint32_t xorKey = (uint32_t)((dataPtr + 0x10) & 0xFFFFFFFF);
	int decoded = (int)(raw10 ^ xorKey);

	std::println("[StatsWriter] === Diagnose '{}' (hash {:08X}) @ {:X} ===", resolved, hash, dataPtr);
	std::println("  vtable={:X} flags={:08X}", vtable, flags);
	std::println("  +0x10 raw={:08X} XOR key={:08X} => decoded value = {}", raw10, xorKey, decoded);

	// Show linked entries with their XOR-decoded values
	uintptr_t current = dataPtr;
	for (int depth = 0; depth < 4; depth++)
	{
		uintptr_t nextPtr = *reinterpret_cast<uintptr_t*>(&bytes[0x18]);
		if (depth > 0)
		{
			if (!DMA::Read(current + 0x18, nextPtr))
				break;
		}
		if (nextPtr < 0x10000 || nextPtr == dataPtr)
			break;

		uint32_t linkRaw = 0;
		DMA::Read(nextPtr + 0x10, linkRaw);
		uint32_t linkKey = (uint32_t)((nextPtr + 0x10) & 0xFFFFFFFF);
		int linkDecoded = (int)(linkRaw ^ linkKey);

		std::println("  Linked[{}] @ {:X}: raw={:08X} key={:08X} => {}", depth, nextPtr, linkRaw, linkKey, linkDecoded);
		current = nextPtr;
	}
}
