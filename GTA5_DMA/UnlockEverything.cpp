#include "pch.h"
#include "UnlockEverything.h"
#include "StatsWriter.h"
#include "PackedStats.h"
#include "TunableService.h"
#include "core/DMAScriptHelper.h"
#include "offsets/Offsets.h"

// ---- Script globals ----
static constexpr DWORD UE_LOOP_GLOBAL_INT = 4538671; // ScriptGlobal(4538671):set_int(0)
static constexpr DWORD UE_LOOP_GLOBAL_FLOAT = 262146; // ScriptGlobal(262146):set_float(0)
static constexpr DWORD UE_GUARD_GLOBAL = 2655293;     // run only if get_int() != -1
static constexpr DWORD UE_GENDER_GLOBAL = 1574927;    // == 0 => male

// ---- Named stat tables ----
struct NamedStat { const char* name; int value; };
static const NamedStat NAMED_INT[] = {
#include "UnlockEverything_NamedInt.inc"
};
struct NamedFloat { const char* name; float value; };
static const NamedFloat NAMED_FLOAT[] = {
#include "UnlockEverything_NamedFloat.inc"
};

// ---- Heist strand stats: MPX_HEIST_SAVED_STRAND_n = tunables.get_int(ROOT_ID_HASH_*) ----
struct StrandStat { const char* statName; const char* tunableName; };
static const StrandStat HEIST_STRANDS[] = {
	{ "MPX_HEIST_SAVED_STRAND_0", "ROOT_ID_HASH_THE_FLECCA_JOB" },
	{ "MPX_HEIST_SAVED_STRAND_1", "ROOT_ID_HASH_THE_PRISON_BREAK" },
	{ "MPX_HEIST_SAVED_STRAND_2", "ROOT_ID_HASH_THE_HUMANE_LABS_RAID" },
	{ "MPX_HEIST_SAVED_STRAND_3", "ROOT_ID_HASH_SERIES_A_FUNDING" },
	{ "MPX_HEIST_SAVED_STRAND_4", "ROOT_ID_HASH_THE_PACIFIC_STANDARD_JOB" },
};

// ---- Packed op tables ----
enum class PackedOp { Bool, BoolRange, Int };
struct PackedStatOp { PackedOp op; int a; int b; int value; };
static const PackedStatOp PACKED_COMMON[] = {
#include "UnlockEverything_PackedCommon.inc"
};
static const PackedStatOp PACKED_MALE[] = {
#include "UnlockEverything_PackedMale.inc"
};
static const PackedStatOp PACKED_FEMALE[] = {
#include "UnlockEverything_PackedFemale.inc"
};

// ===========================================================================

void UnlockEverything::ApplyOne(const Action& a)
{
	switch (a.type)
	{
	case Action::StatInt:    if (StatsWriter::SetStatInt(a.name, a.val)) AppliedOk++; break;
	case Action::StatFloat:  if (StatsWriter::SetStatFloat(a.name, a.fval)) AppliedOk++; break;
	case Action::PackedBool: if (PackedStats::SetPackedBool(a.idx, a.val != 0)) AppliedOk++; break;
	case Action::PackedInt:  if (PackedStats::SetPackedInt(a.idx, a.val)) AppliedOk++; break;
	}
}

void UnlockEverything::BuildQueue()
{
	Queue.clear();
	QueuePos = 0;
	AppliedOk = 0;

	if (bIncludeNamed)
		for (const auto& s : NAMED_INT)
			Queue.push_back({ Action::StatInt, s.name, 0.0f, 0, s.value });

	if (bIncludeFloats)
		for (const auto& s : NAMED_FLOAT)
			Queue.push_back({ Action::StatFloat, s.name, s.value, 0, 0 });

	// Brute-force: every packed bool index across all bool families -> guarantees all clothing
	// and "complete X to unlock" items. Overrides the curated packed list.
	if (bBruteForceAllBools)
	{
		for (auto [start, end] : PackedStats::BoolRanges())
			for (int idx = start; idx < end; idx++)
				Queue.push_back({ Action::PackedBool, nullptr, 0.0f, idx, 1 });
	}
	else if (bIncludePacked)
	{
		auto addPacked = [](const PackedStatOp* ops, size_t n) {
			for (size_t i = 0; i < n; i++)
			{
				const auto& o = ops[i];
				if (o.op == PackedOp::Int)
					Queue.push_back({ Action::PackedInt, nullptr, 0.0f, o.a, o.value });
				else if (o.op == PackedOp::Bool)
					Queue.push_back({ Action::PackedBool, nullptr, 0.0f, o.a, o.value });
				else // BoolRange -> expand so each index is one rate-limited action
					for (int x = o.a; x <= o.b; x++)
						Queue.push_back({ Action::PackedBool, nullptr, 0.0f, x, o.value });
			}
		};
		addPacked(PACKED_COMMON, std::size(PACKED_COMMON));
		if (LastWasMale) addPacked(PACKED_MALE, std::size(PACKED_MALE));
		else             addPacked(PACKED_FEMALE, std::size(PACKED_FEMALE));
	}
}

void UnlockEverything::Run()
{
	if (Running)
	{
		std::println("[UnlockEverything] Already running.");
		return;
	}

	if (!StatsWriter::IsReady())
	{
		LastResult = "UnlockEverything: stats not ready (join a session).";
		return;
	}
	if (Offsets::IsSessionStarted)
	{
		bool online = false;
		DMA::Read(DMA::BaseAddress + Offsets::IsSessionStarted, online);
		if (!online) { LastResult = "UnlockEverything: join any freemode session first."; return; }
	}
	if ((int)DMA::GetGlobalInt(UE_GUARD_GLOBAL) == -1)
	{
		LastResult = "UnlockEverything: player data not ready (guard == -1). Try again shortly.";
		return;
	}

	LastWasMale = ((int)DMA::GetGlobalInt(UE_GENDER_GLOBAL) == 0);

	// Heist strands are only 5 writes -- apply immediately (won't cause a timeout).
	StrandOk = 0;
	if (TunableService::IsLoaded())
		for (const auto& s : HEIST_STRANDS)
		{
			int tv = 0;
			if (TunableService::GetTunableInt(DMAScript::Joaat(s.tunableName), tv))
				if (StatsWriter::SetStatInt(s.statName, tv)) StrandOk++;
		}

	BuildQueue();
	if (Queue.empty())
	{
		LastResult = "UnlockEverything: nothing selected (enable a category).";
		return;
	}

	Running = true;
	NextTickMs = 0; // first batch next frame
	LastResult = std::format("UnlockEverything: starting [{}], {} writes queued...",
		LastWasMale ? "male" : "female", Queue.size());
}

void UnlockEverything::Stop()
{
	if (!Running) return;
	Running = false;
	LastResult = std::format("UnlockEverything STOPPED at {}/{} ({} applied).",
		QueuePos, Queue.size(), AppliedOk);
}

bool UnlockEverything::OnDMAFrame()
{
	// Block 1: continuous global zeroing loop.
	if (bGlobalLoop)
	{
		DMA::SetGlobalInt(UE_LOOP_GLOBAL_INT, 0);
		DMA::SetGlobalFloat(UE_LOOP_GLOBAL_FLOAT, 0.0f);
	}

	// Block 2: gradual queue drain.
	if (Running)
	{
		ULONGLONG now = GetTickCount64();
		if (now >= NextTickMs)
		{
			NextTickMs = now + (ULONGLONG)(TickIntervalMs < 1 ? 1 : TickIntervalMs);
			int budget = (ItemsPerTick < 1) ? 1 : ItemsPerTick;
			while (budget-- > 0 && QueuePos < Queue.size())
				ApplyOne(Queue[QueuePos++]);

			if (QueuePos >= Queue.size())
			{
				Running = false;
				LastResult = std::format(
					"UnlockEverything DONE [{}]. {}/{} writes applied (strands {}/5). Check in-game.",
					LastWasMale ? "male" : "female", AppliedOk, Queue.size(), StrandOk);
				std::println("[UnlockEverything] {}", LastResult);
			}
			else
			{
				LastResult = std::format("Unlocking... {}/{} ({} ok)", QueuePos, Queue.size(), AppliedOk);
			}
		}
	}

	return true;
}

void UnlockEverything::Render()
{
	ImGui::Checkbox("UnlockEverything Loop", &bGlobalLoop);
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Continuously zeroes globals 4538671 (int) + 262146 (float).");

	ImGui::SeparatorText("Categories to unlock");
	ImGui::Checkbox("Named award/progress stats", &bIncludeNamed);
	ImGui::Checkbox("Packed clothing/vehicle unlocks", &bIncludePacked);
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Packed range table is VERIFIED on this build (all families resolve).");
	ImGui::Checkbox("Float stats (pilot school)", &bIncludeFloats);
	ImGui::Checkbox("Brute-force ALL packed bools (every clothing/unlock)", &bBruteForceAllBools);
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Sets EVERY packed bool true across all families -- guarantees all clothing\n"
			"including 'complete X to unlock' items (e.g. Casino Heist rewards), plus all\n"
			"challenge/award/trophy bools. ~30k writes -- let the gradual apply run (~minutes).\n"
			"Overrides the curated packed list when on.");

	ImGui::SeparatorText("Gradual apply (avoids R* timeouts)");
	ImGui::SetNextItemWidth(200.f);
	ImGui::SliderInt("Writes per tick", &ItemsPerTick, 1, 100);
	ImGui::SetNextItemWidth(200.f);
	ImGui::SliderInt("Tick interval (ms)", &TickIntervalMs, 50, 1000);
	// Rough ETA estimate from current selection.
	{
		size_t est = 0;
		if (bIncludeNamed) est += std::size(NAMED_INT);
		if (bIncludeFloats) est += std::size(NAMED_FLOAT);
		if (bBruteForceAllBools)
		{
			for (auto [s, e] : PackedStats::BoolRanges()) est += (e - s);
		}
		else if (bIncludePacked) est += 4000; // approx packed indices
		float secs = (ItemsPerTick > 0)
			? (float)est / ItemsPerTick * (TickIntervalMs / 1000.0f) : 0.0f;
		ImGui::Text("~%zu writes, est. %.0fs at this rate", est, secs);
	}

	if (!StatsWriter::IsReady())
	{
		ImGui::TextDisabled("(stats not ready -- join a session)");
		return;
	}

	if (Running)
	{
		float frac = Queue.empty() ? 0.0f : (float)QueuePos / (float)Queue.size();
		ImGui::ProgressBar(frac, ImVec2(300, 0));
		if (ImGui::Button("Stop"))
			Stop();
	}
	else
	{
		if (ImGui::Button("Unlock Everything (gradual)##run", ImVec2(220, 0)))
			Run();
	}

	if (!LastResult.empty())
		ImGui::TextWrapped("%s", LastResult.c_str());
}
