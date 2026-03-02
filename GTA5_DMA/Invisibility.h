#pragma once

// Invisibility via dual approach:
// 1. PED entity InvisibilityFlag (direct memory write for local visibility)
// 2. GlobalPlayerBD IsInvisible (broadcast data to other players)
// Note: YimMenu uses SET_ENTITY_VISIBLE native which cannot be called over DMA

class Invisibility
{
public:
	static inline bool bEnable = false;

	static bool OnDMAFrame();

private:
	// PED struct: InvisibilityFlag at offset 0x2C (int8_t)
	static constexpr uintptr_t PED_INVISIBILITY_FLAG_OFFSET = 0x2C;

	// GlobalPlayerBD structure constants (broadcast to other players)
	static constexpr DWORD GLOBAL_PLAYER_BD_BASE = 2658294;
	static constexpr int ENTRY_SIZE = 468;       // slots per player entry
	static constexpr int IS_INVISIBLE = 258;     // slot offset within entry (verified: +1 from original count)

	static inline bool WasEnabled = false;
	static inline int8_t OriginalPedFlag = 0;   // saved original value at 0x2C before we modify it
	static inline int LastPlayerId = -1;         // saved player ID for global restore

	static int GetLocalPlayerIndex();
};
