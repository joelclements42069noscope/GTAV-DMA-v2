#pragma once

// Player List - reads connected players from CNetworkPlayerMgr
// Shows player index, Rockstar ID, and connection status
// Readable over DMA without native calls

struct PlayerEntry
{
	bool Connected = false;
	uint8_t PlayerIndex = 0;
	int64_t RockstarId = 0;
};

class PlayerList
{
public:
	static inline bool bEnable = false;
	static inline PlayerEntry Players[32] = {};
	static inline int ConnectedCount = 0;

	static bool OnDMAFrame();
	static void Render(); // ImGui render

private:
	// CNetworkPlayerMgr offsets
	static constexpr uintptr_t PLAYERS_ARRAY_OFFSET = 0x188; // CNetGamePlayer* m_Players[32]

	// CNetGamePlayer (netPlayer) offsets
	static constexpr uintptr_t PLAYER_INDEX_OFFSET = 0x61;   // uint8_t m_PlayerIndex
	static constexpr uintptr_t ROCKSTAR_ID_OFFSET = 0x10;    // int64_t m_RockstarId
};
