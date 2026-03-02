#pragma once

// Casino Slot Machine Rigger
// Writes winning (6) or losing (random 3-5,7-9) values to the casino_slots script's local variable table
// Ported from YimMenu V2: src/game/features/recovery/Casino.cpp

class CasinoSlotRigger
{
public:
	static inline bool bRigWin = false;    // Force every spin to win
	static inline bool bAutoMode = false;  // Alternate: win, lose, win, lose...

	static bool OnDMAFrame();
	static void Render();

private:
	// Script local variable offsets (from YimMenu)
	static constexpr int SLOTS_RESULTS_TABLE = 1350;
	static constexpr int SPIN_STATE_VAR = 1668;
	static constexpr int WINNING_VALUE = 6;

	// Blacklisted slot indices that shouldn't be modified
	static bool IsBlacklisted(int index);

	// Spin states where it's safe to modify (8 = idle, 14 = reset)
	static bool IsSafeSpinState(int state);

	// Write winning values (all 6) or losing values (random non-6)
	static void WriteSlotValues(uintptr_t stack, bool win);

	// Generate a random losing value (3-5 or 7-9, never 6)
	static int RandomLosingValue();

	// State tracking
	static inline bool WasRigging = false;        // true if we were actively rigging last frame
	static inline int WinRate = 50;               // auto mode: win percentage (0-100)
	static inline int LastSpinState = -1;          // for detecting spin transitions
	static inline int AutoWinCount = 0;            // stats
	static inline int AutoLoseCount = 0;           // stats
};
