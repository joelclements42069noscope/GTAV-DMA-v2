#pragma once

// Career Progress -- native DMA port of the "Career Progress" YimMenu Lua script
// (stats by ShinyWasabi / ImagineNothing). The DMA cannot run Lua or call natives,
// so every operation is translated into raw memory writes:
//
//   Lua                                  -> DMA
//   --------------------------------------------------------------------------
//   ScriptGlobal(n):set_int/set_float    -> DMA::SetGlobalInt/SetGlobalFloat   (loop, OnDMAFrame)
//   stats.set_int / set_bool             -> StatsWriter::SetStatInt (XOR encoded) [SUPPORTED]
//   stats.set_packed_bool / *_range      -> packed-stat metadata write           [NOT YET SUPPORTED]
//   stats.set_packed_int                 -> packed-stat metadata write           [NOT YET SUPPORTED]
//
// Packed stats are data-table-driven in the game (index -> backing stat + bit via
// GetPackedStatData). Replicating that over DMA is being researched separately; until
// it lands, packed ops are counted and logged but skipped. See LastResult for a summary.
//
// Two independent behaviours, matching the script's two script.run_in_callback blocks:
//   1. Global loop  -- continuously zeroes a few script globals (bGlobalLoop, per-frame)
//   2. One-shot Run -- applies all the career/award stat writes once (online only)

class CareerProgress
{
public:
	// Block 1: persistent loop that zeroes the career-reset script globals each frame.
	static inline bool bGlobalLoop = false;

	// Also apply the packed-stat ops during a run (uses PackedStats; table unverified).
	static inline bool bIncludePacked = false;

	// Per-frame entry (drives the global loop and the one-shot run state machine).
	static bool OnDMAFrame();

	// Block 2: request the one-shot stat application. Executed on the DMA thread so the
	// UI does not stall and so the script's 5s mid-run pause can be honoured.
	static void Run();

	// ImGui UI.
	static void Render();

	// Human-readable summary of the last run (shown in the menu).
	static inline std::string LastResult;

private:
	// One-shot run state machine (mirrors the script: batch -> yield(5000) -> final batch).
	enum class Phase { Idle, ApplyMain, WaitYield, ApplyFinal };
	static inline Phase RunState = Phase::Idle;
	static inline ULONGLONG YieldUntilMs = 0;

	// Counters for the in-progress / last run.
	static inline int NamedOk = 0;
	static inline int NamedFail = 0;
	static inline int PackedSkipped = 0;
	static inline int PackedOk = 0;
	static inline int PackedTotal = 0;

	static void ApplyNamedStats();
	static void LogPackedSkips();
};
