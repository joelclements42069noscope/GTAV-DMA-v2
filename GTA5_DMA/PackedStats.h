#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <utility>

// Packed-stat read/write over DMA -- replicates SET_PACKED_STAT_BOOL_CODE /
// SET_PACKED_STAT_INT_CODE without calling game natives.
//
// A packed index maps to a backing stat + bit/field via a [start,end) range table.
//   bool: sub = (index-start)/64, bit = (index-start)%64  (value is 64-bit: bits 0..63)
//   int : sub = (index-start)/8,  field = (index-start)%8 -> 8-bit field, shift = field*8
//   name = prefix + tag + sub      (prefix = MP0_/MP1_ for char stats, MP_ for global)
// Each 32-bit dword of the value is XOR-encoded with the low32 of its own address,
// so we do a per-dword masked read-modify-write via StatsWriter (no native call).
//
// IMPORTANT: the range table below is derived from decompiled build 1.54 and is NOT yet
// verified against the current Enhanced build. Boundaries/tags for high indices (>~30000)
// are especially likely to have shifted. Use the Packed Debug UI to verify a few indices
// (resolve -> read -> compare) before trusting bulk writes. See [[gtav-packed-stats]].

class PackedStats
{
public:
	struct Resolved
	{
		bool valid = false;          // index fell inside a known range
		bool isBool = false;         // bool packed stat vs int packed stat
		std::string statName;        // fully-qualified backing stat (e.g. MP0_CASINOPSTAT_BOOL3)
		int rangeStart = 0;          // matched range start
		const char* tag = "";        // matched family tag
		int sub = 0;                 // sub-stat number (suffix)
		int bit = 0;                 // bool: bit 0..63 | int: field index 0..7
		int dwordOffset = 0;         // 0 => value+0x10, 4 => value+0x14
		int shift = 0;               // bit shift within the dword
		uint32_t mask = 0;           // field/bit mask within the dword
		uintptr_t dataPtr = 0;       // resolved sStatData* (0 = stat not found in CStatsMgr)
		bool valueRead = false;      // whether curValue is populated
		uint32_t curValue = 0;       // current decoded bit (0/1) or 8-bit field value
	};

	// Resolve a packed index to its backing stat + position. If readValue, also reads
	// the current value over DMA. Does not write anything.
	static Resolved Resolve(int index, bool readValue = true);

	// Apply operations. Return true/count on success (stat found + written).
	static bool SetPackedBool(int index, bool value);
	static bool SetPackedInt(int index, int value);
	static int  SetPackedBoolRange(int startIndex, int endIndex, bool value); // inclusive; returns #written

	// The seeded table has not been verified on the live Enhanced build.
	static bool TableVerified() { return false; }

	// Build a diagnostic report: resolves the first index of every range in the table
	// (backing stat name + whether it exists in CStatsMgr + current value), plus the
	// extra out-of-table indices the scripts use. For the offset-export button.
	static std::string TableReport();

	// All bool-family [start,end) ranges (for brute-force "unlock everything" sweeps).
	static std::vector<std::pair<int, int>> BoolRanges();
};
