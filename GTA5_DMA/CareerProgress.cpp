#include "pch.h"
#include "CareerProgress.h"
#include "StatsWriter.h"
#include "PackedStats.h"
#include "offsets/Offsets.h"

// ---------------------------------------------------------------------------
// Block 1 data: script globals that the Lua loop continuously zeroes.
//   ScriptGlobal(4516902):set_int(0)  ... 4516904
//   ScriptGlobal(262146):set_float(0)
// ---------------------------------------------------------------------------
static constexpr DWORD CAREER_RESET_GLOBALS_INT[] = { 4516902, 4516903, 4516904 };
static constexpr DWORD CAREER_RESET_GLOBAL_FLOAT = 262146;

// ---------------------------------------------------------------------------
// Block 2 data: named (string) stat writes. set_bool entries are folded in as
// 0/1 int writes -- STAT_SET_BOOL and STAT_SET_INT both write the same XOR-encoded
// m_Value field, so an int write of 1/0 is byte-identical to a bool write.
// Order preserved from the script (duplicate names => last write wins, as in Lua).
// ---------------------------------------------------------------------------
struct NamedStat { const char* name; int value; };
static const NamedStat NAMED_STATS[] = {
#include "CareerProgress_NamedStats.inc"
};

// ---------------------------------------------------------------------------
// Block 2 data: packed-stat ops, split into the pre-yield batch and the
// post-yield batch (the script does script.yield(5000) between them). These are
// NOT YET APPLIED -- packed stats are data-table-driven and being researched.
// The tables are kept ready so the implementation drops straight in.
// ---------------------------------------------------------------------------
enum class PackedOp { Bool, BoolRange, Int };
struct PackedStatOp { PackedOp op; int a; int b; int value; };

static const PackedStatOp PACKED_OPS_PRE[] = {
#include "CareerProgress_PackedPre.inc"
};
static const PackedStatOp PACKED_OPS_POST[] = {
#include "CareerProgress_PackedPost.inc"
};

static int CountPackedIndices(const PackedStatOp* ops, size_t n)
{
	int total = 0;
	for (size_t i = 0; i < n; i++)
		total += (ops[i].op == PackedOp::BoolRange) ? (ops[i].b - ops[i].a + 1) : 1;
	return total;
}

// Apply a packed-op table via PackedStats. Accumulates written / attempted counts.
static void ApplyPackedTable(const PackedStatOp* ops, size_t n, int& okOut, int& totalOut)
{
	for (size_t i = 0; i < n; i++)
	{
		const auto& o = ops[i];
		switch (o.op)
		{
		case PackedOp::Bool:
			totalOut += 1;
			if (PackedStats::SetPackedBool(o.a, o.value != 0)) okOut++;
			break;
		case PackedOp::BoolRange:
			totalOut += (o.b - o.a + 1);
			okOut += PackedStats::SetPackedBoolRange(o.a, o.b, o.value != 0);
			break;
		case PackedOp::Int:
			totalOut += 1;
			if (PackedStats::SetPackedInt(o.a, o.value)) okOut++;
			break;
		}
	}
}

// ---------------------------------------------------------------------------

void CareerProgress::ApplyNamedStats()
{
	NamedOk = 0;
	NamedFail = 0;

	for (const auto& s : NAMED_STATS)
	{
		if (StatsWriter::SetStatInt(s.name, s.value))
			NamedOk++;
		else
			NamedFail++;
	}

	std::println("[CareerProgress] Named stats: {} ok, {} failed (of {})",
		NamedOk, NamedFail, (int)std::size(NAMED_STATS));
}

void CareerProgress::LogPackedSkips()
{
	int pre = CountPackedIndices(PACKED_OPS_PRE, std::size(PACKED_OPS_PRE));
	int post = CountPackedIndices(PACKED_OPS_POST, std::size(PACKED_OPS_POST));
	PackedSkipped = pre + post;
	std::println("[CareerProgress] Packed stats SKIPPED (not yet supported): {} indices "
		"({} pre-yield + {} post-yield ops)", PackedSkipped,
		(int)std::size(PACKED_OPS_PRE), (int)std::size(PACKED_OPS_POST));
}

void CareerProgress::Run()
{
	if (RunState != Phase::Idle)
	{
		std::println("[CareerProgress] Run already in progress.");
		return;
	}
	RunState = Phase::ApplyMain;
	LastResult = "Career Progress: running...";
}

bool CareerProgress::OnDMAFrame()
{
	// ---- Block 1: continuous global zeroing loop ----
	if (bGlobalLoop)
	{
		for (DWORD idx : CAREER_RESET_GLOBALS_INT)
			DMA::SetGlobalInt(idx, 0);
		DMA::SetGlobalFloat(CAREER_RESET_GLOBAL_FLOAT, 0.0f);
	}

	// ---- Block 2: one-shot run state machine ----
	switch (RunState)
	{
	case Phase::Idle:
		break;

	case Phase::ApplyMain:
	{
		if (!StatsWriter::IsReady())
		{
			LastResult = "Career Progress: waiting for stats (join a session)...";
			break; // retry next frame
		}

		// Session check -- the script only runs online.
		if (Offsets::IsSessionStarted)
		{
			bool online = false;
			DMA::Read(DMA::BaseAddress + Offsets::IsSessionStarted, online);
			if (!online)
			{
				LastResult = "Career Progress: join any freemode session, then run again.";
				RunState = Phase::Idle;
				break;
			}
		}

		ApplyNamedStats();

		PackedOk = 0;
		PackedTotal = 0;
		if (bIncludePacked)
		{
			ApplyPackedTable(PACKED_OPS_PRE, std::size(PACKED_OPS_PRE), PackedOk, PackedTotal);
			std::println("[CareerProgress] Pre-yield packed: {}/{} indices written", PackedOk, PackedTotal);
		}

		YieldUntilMs = GetTickCount64() + 5000; // script.yield(5000)
		RunState = Phase::WaitYield;
		LastResult = std::format("Named stats applied ({}/{}). Finishing in 5s...",
			NamedOk, (int)std::size(NAMED_STATS));
		break;
	}

	case Phase::WaitYield:
		if (GetTickCount64() >= YieldUntilMs)
			RunState = Phase::ApplyFinal;
		break;

	case Phase::ApplyFinal:
	{
		if (bIncludePacked)
		{
			ApplyPackedTable(PACKED_OPS_POST, std::size(PACKED_OPS_POST), PackedOk, PackedTotal);
			LastResult = std::format(
				"Career Progress done. Named: {} ok / {} failed. "
				"Packed: {}/{} indices written (table UNVERIFIED -- check results). Check your Career page.",
				NamedOk, NamedFail, PackedOk, PackedTotal);
		}
		else
		{
			LogPackedSkips();
			LastResult = std::format(
				"Career Progress done. Named: {} ok / {} failed. "
				"Packed: {} skipped (enable 'Include packed stats'). Check your Career page.",
				NamedOk, NamedFail, PackedSkipped);
		}
		std::println("[CareerProgress] {}", LastResult);
		RunState = Phase::Idle;
		break;
	}
	}

	return true;
}

void CareerProgress::Render()
{
	ImGui::Checkbox("Career Progress Loop", &bGlobalLoop);
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Continuously zeroes the career-reset script globals "
			"(4516902-4516904 + float 262146), matching the script's while-true loop.");

	ImGui::Checkbox("Include packed stats (verified table)##cp", &bIncludePacked);
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Applies the ~%d packed-stat indices. The range table is VERIFIED on this\n"
			"build (every family's stat names resolve, incl. the newer DLC families).",
			CountPackedIndices(PACKED_OPS_PRE, std::size(PACKED_OPS_PRE)) +
			CountPackedIndices(PACKED_OPS_POST, std::size(PACKED_OPS_POST)));

	if (!StatsWriter::IsReady())
	{
		ImGui::BeginDisabled();
		ImGui::Button("Apply Career Progress");
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::TextDisabled("(stats not ready -- join a session)");
	}
	else
	{
		const bool running = (RunState != Phase::Idle);
		ImGui::BeginDisabled(running);
		if (ImGui::Button(running ? "Applying..." : "Apply Career Progress"))
			Run();
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Applies all %d named career/award stats. The ~%d packed-stat "
				"indices are not yet supported (logged & skipped).",
				(int)std::size(NAMED_STATS),
				CountPackedIndices(PACKED_OPS_PRE, std::size(PACKED_OPS_PRE)) +
				CountPackedIndices(PACKED_OPS_POST, std::size(PACKED_OPS_POST)));
	}

	if (!LastResult.empty())
		ImGui::TextWrapped("%s", LastResult.c_str());
}
