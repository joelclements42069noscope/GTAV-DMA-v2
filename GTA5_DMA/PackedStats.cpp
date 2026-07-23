#include "pch.h"
#include "PackedStats.h"
#include "StatsWriter.h"

namespace
{
	enum class Prefix { Char, Global };

	struct Range
	{
		int start;        // inclusive
		int end;          // exclusive
		const char* tag;  // family tag, name = prefix + tag + subNumber
		bool isBool;      // true => packed bool (64 bits/stat), false => packed int (8x 8-bit fields)
		Prefix prefix;    // Char => MP0_/MP1_, Global => MP_
	};

	// Range table derived from decompiled mp_awards.c (build 1.54). [start, end).
	// UNVERIFIED for the Enhanced build -- verify via the Packed Debug UI before bulk use.
	// Bool and int ranges are interleaved; the matched range's isBool decides the layout.
	constexpr Range kRanges[] = {
		// --- bool ranges ---
		{     0,   192, "PSTAT_BOOL",            true,  Prefix::Char   },
		{   192,   384, "PSTAT_BOOL",            true,  Prefix::Char   },
		{   513,   705, "PSTAT_BOOL",            true,  Prefix::Global },
		{   705,  1281, "PSTAT_BOOL",            true,  Prefix::Char   },
		{  2919,  3111, "TUPSTAT_BOOL",          true,  Prefix::Global },
		{  3111,  3879, "TUPSTAT_BOOL",          true,  Prefix::Char   },
		{  4207,  4335, "NGPSTAT_BOOL",          true,  Prefix::Char   },
		{  4335,  4399, "NGPSTAT_BOOL",          true,  Prefix::Global },
		{  6029,  6413, "NGTATPSTAT_BOOL",       true,  Prefix::Char   },
		{  7321,  7385, "NGDLCPSTAT_BOOL",       true,  Prefix::Global },
		{  7385,  7641, "NGDLCPSTAT_BOOL",       true,  Prefix::Char   },
		{  9361,  9553, "DLCBIKEPSTAT_BOOL",     true,  Prefix::Char   },
		{ 15369, 15561, "DLCGUNPSTAT_BOOL",      true,  Prefix::Char   },
		{ 15562, 15946, "GUNTATPSTAT_BOOL",      true,  Prefix::Char   },
		{ 15946, 16010, "DLCSMUGCHARPSTAT_BOOL", true,  Prefix::Char   },
		{ 18098, 18162, "GANGOPSPSTAT_BOOL",     true,  Prefix::Char   },
		{ 22066, 22194, "BUSINESSBATPSTAT_BOOL", true,  Prefix::Char   },
		{ 24962, 25538, "ARENAWARSPSTAT_BOOL",   true,  Prefix::Char   },
		{ 26810, 27258, "CASINOPSTAT_BOOL",      true,  Prefix::Char   },
		{ 28098, 28354, "CASINOHSTPSTAT_BOOL",   true,  Prefix::Char   },
		{ 28355, 28483, "HEIST3TATTOOSTAT_BOOL", true,  Prefix::Char   },
		{ 30227, 30355, "SU20PSTAT_BOOL",        true,  Prefix::Char   },
		{ 30355, 30483, "SU20TATTOOSTAT_BOOL",   true,  Prefix::Char   },
		{ 30515, 30707, "HISLANDPSTAT_BOOL",     true,  Prefix::Char   },
		{ 31707, 32283, "TUNERPSTAT_BOOL",       true,  Prefix::Char   },
		{ 32283, 32411, "FIXERPSTAT_BOOL",       true,  Prefix::Char   },
		{ 32411, 32475, "FIXERTATTOOSTAT_BOOL",  true,  Prefix::Char   },
		{ 34251, 34763, "DLC12022PSTAT_BOOL",    true,  Prefix::Char   },
		// --- int ranges ---
		{   384,   457, "PSTAT_INT",             false, Prefix::Char   },
		{   457,   513, "PSTAT_INT",             false, Prefix::Char   },
		{  1281,  1305, "PSTAT_INT",             false, Prefix::Global },
		{  1305,  1361, "PSTAT_INT",             false, Prefix::Char   },
		{  1361,  1393, "TUPSTAT_INT",           false, Prefix::Global },
		{  1393,  2919, "TUPSTAT_INT",           false, Prefix::Char   },
		{  3879,  4143, "NGPSTAT_INT",           false, Prefix::Char   },
		{  4143,  4207, "NGPSTAT_INT",           false, Prefix::Global },
		{  4399,  6028, "LRPSTAT_INT",           false, Prefix::Char   },
		{  6413,  7262, "APAPSTAT_INT",          false, Prefix::Char   },
		{  7262,  7313, "LR2PSTAT_INT",          false, Prefix::Char   },
		{  7313,  7321, "NGDLCPSTAT_INT",        false, Prefix::Global },
		{  7641,  7681, "NGDLCPSTAT_INT",        false, Prefix::Char   },
		{  7681,  9361, "BIKEPSTAT_INT",         false, Prefix::Char   },
		{  9553, 15265, "IMPEXPPSTAT_INT",       false, Prefix::Char   },
		{ 15265, 15369, "GUNRPSTAT_INT",         false, Prefix::Char   },
		{ 16010, 18098, "DLCSMUGCHARPSTAT_INT",  false, Prefix::Char   },
		{ 18162, 19018, "GANGOPSPSTAT_INT",      false, Prefix::Char   },
		{ 19018, 22066, "BUSINESSBATPSTAT_INT",  false, Prefix::Char   },
		{ 22194, 24962, "ARENAWARSPSTAT_INT",    false, Prefix::Char   },
		{ 25538, 26810, "CASINOPSTAT_INT",       false, Prefix::Char   },
		{ 27258, 28098, "CASINOHSTPSTAT_INT",    false, Prefix::Char   },
		{ 28483, 30227, "SU20PSTAT_INT",         false, Prefix::Char   },
		{ 30483, 30515, "HISLANDPSTAT_INT",      false, Prefix::Char   },
		{ 30707, 31707, "TUNERPSTAT_INT",        false, Prefix::Char   },
		{ 32475, 34123, "FIXERPSTAT_INT",        false, Prefix::Char   },
		{ 34763, 36627, "DLC12022PSTAT_INT",     false, Prefix::Char   },
		// --- continuation (post-1.54 DLCs) from decompiled Enhanced freemode.c func_784 ---
		// INT ranges + major boundaries HIGH confidence; some BOOL sub-splits inferred.
		// Mapping: DLC22022=SA Mercenaries, DLC12023=Chop Shop, DLC22023=Bottom Dollar Bounties,
		//          DLC12024=Agents of Sabotage/Money Fronts, DLC22024/DLC22025=2024H2/2025.
		// All Prefix::Char (no MP_ globals among the new families). Re-export to confirm found=Y.
		{ 36627, 36947, "DLC22022PSTAT_BOOL",      true,  Prefix::Char },
		{ 36947, 41251, "DLC22022PSTAT_INT",       false, Prefix::Char },
		{ 41251, 41315, "DLC22022TATTOOSTAT_BOOL", true,  Prefix::Char },
		{ 41315, 42083, "DLC12023PSTAT_BOOL",      true,  Prefix::Char },
		{ 42083, 42107, "DLC12023PSTAT_INT",       false, Prefix::Char },
		{ 42107, 42299, "DLC22023PSTAT_BOOL",      true,  Prefix::Char },
		{ 42299, 51059, "DLC22023PSTAT_INT",       false, Prefix::Char },
		{ 51059, 51187, "DLC22023TATTOOSTAT_BOOL", true,  Prefix::Char },
		{ 51187, 51379, "DLC12024PSTAT_BOOL",      true,  Prefix::Char }, // medium confidence
		{ 51379, 51555, "DLC12024PSTAT_INT",       false, Prefix::Char },
		{ 51555, 54051, "DLC22024PSTAT_INT",       false, Prefix::Char },
		{ 54051, 54819, "DLC22024PSTAT_BOOL",      true,  Prefix::Char }, // low confidence (may sub-split)
		{ 54819, 59907, "DLC22025PSTAT_INT",       false, Prefix::Char },

		// The Kortz Center Heist (2026 DLC) packed family lives at 60000+, but the
		// exact backing stat names AND the bool/int sub-split are unknown: the
		// career-progress ops mix bools (60011, 60021-60028, 60105) with ints
		// (60049, 60050) in a way that doesn't fit a single contiguous type range,
		// so any guessed boundary risks writing an int as a bit (or vice versa)
		// into a real stat. Left OUT deliberately -- unresolved indices skip
		// safely. To activate once verified on the live build, add entries like:
		//   { 60000, 600XX, "DLC12026PSTAT_BOOL", true,  Prefix::Char },
		//   { 600XX, 601XX, "DLC12026PSTAT_INT",  false, Prefix::Char },
		// Verify names + boundaries first with the Packed Debug UI (resolve ->
		// read -> compare). The named MPX_AWD_* Kortz awards already work without this.
	};

	const Range* FindRange(int index)
	{
		for (const auto& r : kRanges)
			if (index >= r.start && index < r.end)
				return &r;
		return nullptr;
	}

	std::string PrefixStr(Prefix p, int charIndex)
	{
		if (p == Prefix::Global)
			return "MP_";
		return std::string("MP") + std::to_string(charIndex < 0 ? 0 : charIndex) + "_";
	}
}

PackedStats::Resolved PackedStats::Resolve(int index, bool readValue)
{
	Resolved r{};

	const Range* range = FindRange(index);
	if (!range)
		return r; // valid stays false

	int charIndex = StatsWriter::GetCharIndex();
	if (charIndex < 0)
	{
		StatsWriter::EnsureInitialized();
		charIndex = StatsWriter::GetCharIndex();
	}

	r.valid = true;
	r.isBool = range->isBool;
	r.tag = range->tag;
	r.rangeStart = range->start;

	int rel = index - range->start;
	std::string prefix = PrefixStr(range->prefix, charIndex);

	if (range->isBool)
	{
		r.sub = rel / 64;
		r.bit = rel % 64;
		r.dwordOffset = (r.bit >= 32) ? 4 : 0;
		r.shift = r.bit & 31;
		r.mask = (1u << r.shift);
	}
	else
	{
		r.sub = rel / 8;
		int field = rel % 8;       // 0..7
		int bitStart = field * 8;  // 0,8,...,56
		r.bit = field;
		r.dwordOffset = (bitStart >= 32) ? 4 : 0;
		r.shift = bitStart & 31;
		r.mask = (0xFFu << r.shift);
	}

	r.statName = prefix + range->tag + std::to_string(r.sub);
	r.dataPtr = StatsWriter::FindStatDataPtrByName(r.statName);

	if (readValue && r.dataPtr)
	{
		uint32_t dec = 0;
		if (StatsWriter::ReadStatDword(r.dataPtr, r.dwordOffset, dec))
		{
			r.valueRead = true;
			r.curValue = (dec & r.mask) >> r.shift; // 0/1 for bool, 0..255 for int
		}
	}

	return r;
}

bool PackedStats::SetPackedBool(int index, bool value)
{
	Resolved r = Resolve(index, false);
	if (!r.valid || !r.isBool || !r.dataPtr)
		return false;

	return StatsWriter::WriteStatDwordMasked(r.dataPtr, r.dwordOffset, r.mask, value ? r.mask : 0);
}

bool PackedStats::SetPackedInt(int index, int value)
{
	Resolved r = Resolve(index, false);
	if (!r.valid || r.isBool || !r.dataPtr)
		return false;

	uint32_t valueBits = ((uint32_t)value & 0xFF) << r.shift;
	return StatsWriter::WriteStatDwordMasked(r.dataPtr, r.dwordOffset, r.mask, valueBits);
}

int PackedStats::SetPackedBoolRange(int startIndex, int endIndex, bool value)
{
	int ok = 0;
	for (int i = startIndex; i <= endIndex; i++)
	{
		if (SetPackedBool(i, value))
			ok++;
	}
	return ok;
}

std::vector<std::pair<int, int>> PackedStats::BoolRanges()
{
	std::vector<std::pair<int, int>> out;
	for (const auto& r : kRanges)
		if (r.isBool)
			out.emplace_back(r.start, r.end);
	return out;
}

std::string PackedStats::TableReport()
{
	std::string out;
	out += "--- PACKED TABLE CHECK (start index of each family) ---\n";
	out += "found=Y means the resolved stat name exists in CStatsMgr (range/name likely correct)\n";

	for (const auto& r : kRanges)
	{
		Resolved res = Resolve(r.start, true);
		out += std::format("[idx {:>6}] {:<26} -> {:<28} found={} value={}\n",
			r.start, r.tag, res.statName,
			res.dataPtr ? "Y" : "N",
			res.dataPtr && res.valueRead ? std::to_string(res.curValue) : "-");
	}

	// Spot-check the newer-DLC indices the scripts use (should now resolve found=Y).
	out += "-- newer-DLC script index spot-checks (should resolve found=Y) --\n";
	for (int idx : { 41507, 42000, 51051, 51052, 51278, 54653, 54773, 59999 })
	{
		Resolved res = Resolve(idx, true);
		if (!res.valid)
			out += std::format("[idx {:>6}] NO RANGE (needs newer table)\n", idx);
		else
			out += std::format("[idx {:>6}] {:<26} -> {:<28} found={} value={}\n",
				idx, res.tag, res.statName, res.dataPtr ? "Y" : "N",
				res.dataPtr && res.valueRead ? std::to_string(res.curValue) : "-");
	}
	return out;
}
