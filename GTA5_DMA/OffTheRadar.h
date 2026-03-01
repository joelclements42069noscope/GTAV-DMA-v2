#pragma once

// Off The Radar via GlobalPlayerBD broadcast data
// Sets OffRadarActive = true for the local player, hiding from radar
// Ported from YimMenu V2: src/game/features/self/OffTheRadar.cpp

class OffTheRadar
{
public:
	static inline bool bEnable = false;

	static bool OnDMAFrame();

private:
	// GlobalPlayerBD structure constants
	static constexpr DWORD GLOBAL_PLAYER_BD_BASE = 2658294;
	static constexpr int ENTRY_SIZE = 468;       // slots per player entry
	static constexpr int OFF_RADAR_ACTIVE = 214; // slot offset within entry (verified: 213 = PlayerPosition.Z)

	// Helper to get local player ID from CNetworkPlayerMgr
	static int GetLocalPlayerIndex();
};
