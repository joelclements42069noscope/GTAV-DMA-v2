#include "pch.h"
#include "OffTheRadar.h"
#include "offsets/Offsets.h"

int OffTheRadar::GetLocalPlayerIndex()
{
	if (!Offsets::NetworkPlayerMgr)
	{
		static bool warned = false;
		if (!warned) { std::println("[OTR] NetworkPlayerMgr offset is 0 (pattern scan failed)"); warned = true; }
		return -1;
	}

	uintptr_t pMgr = 0;
	if (!DMA::Read(DMA::BaseAddress + Offsets::NetworkPlayerMgr, pMgr) || !pMgr)
	{
		static bool warned = false;
		if (!warned) { std::println("[OTR] CNetworkPlayerMgr* is null (not in online session?)"); warned = true; }
		return -1;
	}

	uintptr_t pLocalPlayer = 0;
	if (!DMA::Read(pMgr + 0xF0, pLocalPlayer) || !pLocalPlayer)
		return -1;

	uint8_t playerIndex = 0;
	if (!DMA::Read(pLocalPlayer + 0x61, playerIndex))
		return -1;

	if (playerIndex >= 32)
		return -1;

	return static_cast<int>(playerIndex);
}

bool OffTheRadar::OnDMAFrame()
{
	if (!bEnable)
		return true;

	// Check if session is started
	if (Offsets::IsSessionStarted)
	{
		bool sessionStarted = false;
		DMA::Read(DMA::BaseAddress + Offsets::IsSessionStarted, sessionStarted);
		if (!sessionStarted)
		{
			static bool warned = false;
			if (!warned) { std::println("[OTR] Session not started (story mode or loading)"); warned = true; }
			return true;
		}
	}

	int playerId = GetLocalPlayerIndex();
	if (playerId < 0)
		return true;

	DWORD globalIndex = GLOBAL_PLAYER_BD_BASE + 1 + (playerId * ENTRY_SIZE) + OFF_RADAR_ACTIVE;

	// One-time diagnostic: read before, write, read after
	static bool verified = false;
	if (!verified)
	{
		DWORD beforeVal = DMA::GetGlobalInt(globalIndex);
		DMA::SetGlobalInt(globalIndex, 1);
		DWORD afterVal = DMA::GetGlobalInt(globalIndex);
		std::println("[OTR] Player {} -> global {} | before={} after={} (expect 1)",
			playerId, globalIndex, beforeVal, afterVal);

		// Also dump a few nearby globals to help identify if offset is wrong
		std::println("[OTR] Nearby globals around index {}:", globalIndex);
		for (int offset = -3; offset <= 3; offset++)
		{
			DWORD val = DMA::GetGlobalInt(globalIndex + offset);
			std::println("[OTR]   [{}+{}] = {}", globalIndex, offset, val);
		}
		verified = true;
	}

	return DMA::SetGlobalInt(globalIndex, 1);
}
