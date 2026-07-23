#pragma once
#include <vector>

// UnlockEverything -- native DMA port of ShinyWasabi's "UnlockEverything" script
// (updated by ImagineNothing). Unlocks clothing/vehicles/weapons/awards via stat writes.
//
// GRADUAL APPLY: writing all ~5000 stats in one frame makes R* drop/timeout some of them,
// so not everything sticks. Instead we build a flat action queue and drain it at a
// configurable rate (items per tick, tick interval) so the game's stat sync keeps up.
//
// Reliable: named int/bool stats, heist-strand tunable stats, and packed ranges (the packed
// table is verified on the Enhanced build). Float stats use raw bits (float XOR unverified).

class UnlockEverything
{
public:
	// --- Block 1: global loop (zeroes globals 4538671 int + 262146 float each frame) ---
	static inline bool bGlobalLoop = false;

	// --- Category selection (which groups to include in a run) ---
	static inline bool bIncludeNamed = true;
	static inline bool bIncludePacked = true;   // packed table verified on this build
	static inline bool bIncludeFloats = true;   // pilot-school floats
	// Brute force: set EVERY packed bool true (all clothing/items/challenges/awards), not just
	// the curated script list. Guarantees all "complete X to unlock" clothing. Huge -- use gradual.
	static inline bool bBruteForceAllBools = false;

	// --- Gradual rate control ---
	static inline int ItemsPerTick = 20;        // stat writes per tick
	static inline int TickIntervalMs = 250;     // ms between ticks

	static bool OnDMAFrame();
	static void Run();   // build queue + start gradual apply (online-gated)
	static void Stop();  // cancel an in-progress run
	static void Render();

	static inline std::string LastResult;

private:
	struct Action
	{
		enum Type { StatInt, StatFloat, PackedBool, PackedInt } type;
		const char* name;  // StatInt/StatFloat
		float fval;        // StatFloat
		int idx;           // PackedBool/PackedInt
		int val;           // StatInt value / packed value / packed bool 0|1
	};

	static void BuildQueue();
	static void ApplyOne(const Action& a);

	static inline std::vector<Action> Queue;
	static inline size_t QueuePos = 0;
	static inline bool Running = false;
	static inline ULONGLONG NextTickMs = 0;

	static inline int AppliedOk = 0;
	static inline int StrandOk = 0;
	static inline bool LastWasMale = false;
};
