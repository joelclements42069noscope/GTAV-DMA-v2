#include "pch.h"
#include "Invisibility.h"
#include "offsets/Offsets.h"

// YimMenu uses SET_ENTITY_VISIBLE native (game function call) for invisibility.
// Since DMA cannot call game functions, we try two approaches:
// 1. Write PED.InvisibilityFlag directly (local entity visibility)
// 2. Write GlobalPlayerBD.IsInvisible (broadcast data to other players)
// Neither is guaranteed to fully work -- native calls are the correct approach.

bool Invisibility::OnDMAFrame()
{
	if (!bEnable)
	{
		// Restore visibility when disabled
		if (WasEnabled && DMA::LocalPlayerAddress)
		{
			// Restore original PED flag value
			DMA::Write(DMA::LocalPlayerAddress + PED_INVISIBILITY_FLAG_OFFSET, OriginalPedFlag);

			// Also clear the GlobalPlayerBD IsInvisible field
			if (LastPlayerId >= 0 && Offsets::ScriptGlobals)
			{
				DWORD globalIndex = GLOBAL_PLAYER_BD_BASE + 1 + (LastPlayerId * ENTRY_SIZE) + IS_INVISIBLE;
				DMA::SetGlobalInt(globalIndex, 0);
			}

			std::println("[Invisibility] Restored visibility (PED flag={}, player={})", OriginalPedFlag, LastPlayerId);
			WasEnabled = false;
			LastPlayerId = -1;
		}
		return true;
	}

	if (!DMA::LocalPlayerAddress)
		return true;

	// Save original PED flag before first modification
	if (!WasEnabled)
	{
		DMA::Read(DMA::LocalPlayerAddress + PED_INVISIBILITY_FLAG_OFFSET, OriginalPedFlag);
		std::println("[Invisibility] Saved original PED flag = {}", OriginalPedFlag);
	}

	WasEnabled = true;

	// Approach 1: Write PED entity InvisibilityFlag directly
	// This controls local rendering of the ped
	int8_t invisible = 1;
	DMA::Write(DMA::LocalPlayerAddress + PED_INVISIBILITY_FLAG_OFFSET, invisible);

	// Approach 2: Also write GlobalPlayerBD IsInvisible (broadcast to other players)
	if (Offsets::NetworkPlayerMgr && Offsets::ScriptGlobals)
	{
		int playerId = GetLocalPlayerIndex();
		if (playerId >= 0)
		{
			LastPlayerId = playerId;
			DWORD globalIndex = GLOBAL_PLAYER_BD_BASE + 1 + (playerId * ENTRY_SIZE) + IS_INVISIBLE;

			static bool logged = false;
			if (!logged)
			{
				std::println("[Invisibility] Player {} -> PED flag @ {:X}, global {}",
					playerId, DMA::LocalPlayerAddress + PED_INVISIBILITY_FLAG_OFFSET, globalIndex);
				logged = true;
			}

			DMA::SetGlobalInt(globalIndex, 1);
		}
	}

	return true;
}

int Invisibility::GetLocalPlayerIndex()
{
	if (!Offsets::NetworkPlayerMgr)
		return -1;

	uintptr_t pMgr = 0;
	if (!DMA::Read(DMA::BaseAddress + Offsets::NetworkPlayerMgr, pMgr) || !pMgr)
		return -1;

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
