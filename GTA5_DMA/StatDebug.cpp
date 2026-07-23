#include "pch.h"
#include "StatDebug.h"
#include "StatsWriter.h"
#include "PackedStats.h"
#include "TunableService.h"
#include "core/DMAScriptHelper.h"
#include <fstream>
#include <cstring>

// Scan a global index range and append every non-zero int (+ float if it looks like one).
static void ScanNonZeroGlobals(int start, int count, std::string& out, int maxHits = 200)
{
	int hits = 0;
	for (int i = 0; i < count && hits < maxHits; i++)
	{
		int idx = start + i;
		int v = (int)DMA::GetGlobalInt(idx);
		if (v != 0)
		{
			float f = DMA::GetGlobalFloat(idx);
			out += std::format("  [{}] = {}{}\n", idx, v,
				(f > 0.0001f && f < 1e9f) ? std::format("   (float {:.2f})", f) : std::string());
			hits++;
		}
	}
	if (hits >= maxHits) out += "  ... (truncated)\n";
}

// Builds the full offset/diagnostic report that the one-click export button writes out.
// Every section is guarded so it produces useful data even if stats aren't fully ready.
static std::string BuildOffsetReport()
{
	std::string r;
	r += "================= GTAV DMA OFFSET EXPORT =================\n";
	r += "Paste this whole file back to Claude.\n\n";

	r += std::format("Module base : {:X}\n", DMA::BaseAddress);
	r += std::format("CStatsMgr   : {} | stats={} | charIndex(MP slot)={}\n",
		StatsWriter::IsReady() ? "READY" : "NOT READY",
		StatsWriter::GetStatCount(), StatsWriter::GetCharIndex());
	r += std::format("Tunables    : {} loaded\n\n", TunableService::TunableCount);

	// ---- Packed table check ----
	if (StatsWriter::IsReady())
		r += PackedStats::TableReport();
	else
		r += "--- PACKED TABLE CHECK skipped (stats not ready) ---\n";
	r += "\n";

	// ---- Key named stats (resolve MPX_ -> MP<char>_ like the real write path) ----
	r += "--- KEY NAMED STATS (found + current value) ---\n";
	if (StatsWriter::IsReady())
	{
		auto resolveMpx = [](std::string n) -> std::string {
			if (n.size() > 3 && (n[0] == 'M' || n[0] == 'm') && (n[1] == 'P' || n[1] == 'p')
				&& (n[2] == 'X' || n[2] == 'x'))
				return "MP" + std::to_string(StatsWriter::GetCharIndex()) + n.substr(3);
			return n;
		};
		const char* names[] = {
			"MPPLY_LAST_MP_CHAR", "MPX_CLUB_POPULARITY", "MPX_CLUB_PAY_TIME_LEFT",
			"MPX_SALV23_INST_PROG", "MPX_HUB_EARNINGS", "MPX_FIXER_COUNT",
			"MP0_PSTAT_BOOL0", "MP0_TUPSTAT_BOOL0", "MP0_CASINOPSTAT_BOOL0",
		};
		for (const char* n : names)
		{
			std::string resolved = resolveMpx(n);
			uintptr_t p = StatsWriter::FindStatDataPtrByName(resolved);
			uint32_t lo = 0; bool ok = p && StatsWriter::ReadStatDword(p, 0, lo);
			r += std::format("  {:<24} (-> {:<24}) found={} value={}\n", n, resolved,
				p ? "Y" : "N", ok ? std::to_string(lo) : "-");
		}
	}
	else r += "  (stats not ready)\n";
	r += "\n";

	// ---- Business / guard / gender script globals (Legacy 1.x -- need verifying) ----
	r += "--- SCRIPT GLOBALS (Legacy 1.x business/guard/gender) ---\n";
	r += "  format: [index] int / float ; owning the business should make ownership != 0\n";
	struct G { const char* label; int idx; };
	const G globals[] = {
		{ "UE guard 2655293",      2655293 },
		{ "restock guard 2655288", 2655288 },
		{ "gender 1574927(0=M)",   1574927 },
		{ "BIZ_BASE 1845560",      1845560 },
		{ "warehouse WH_PROP",     1845560 + 128 },
		{ "mc MC_PROP",            1845560 + 205 },
		{ "hangar HANGAR_PROP",    1845560 + 304 },
		{ "hangar+3 (stock)",      1845560 + 304 + 3 },
		{ "nightclub NC_PROP",     1845560 + 364 },
		{ "nightclub+4 (pop f)",   1845560 + 364 + 4 },
		{ "salvage SY_PROP",       1845560 + 504 },
		{ "carwash CW_PROP",       1882717 + 1 + 158 + 27 },
		{ "hangar fill 1882714",   1882707 + 7 },
		{ "warehouse fill 1882695",1882682 + 13 },
	};
	for (const auto& g : globals)
		r += std::format("  [{:>7}] {:<22} int={:<12} float={:.3f}\n",
			g.idx, g.label, (int)DMA::GetGlobalInt(g.idx), DMA::GetGlobalFloat(g.idx));
	// MC business slot array samples: at(MC_PROP, i, 13) = MC_PROP+1+i*13
	r += "  MC slots at(MC_PROP,i,13):";
	for (int i = 0; i < 7; i++)
		r += std::format(" [{}]={}", i, (int)DMA::GetGlobalInt(1845560 + 205 + 1 + i * 13));
	r += "\n";

	// Non-zero globals across the business regions -- when you OWN businesses, ownership/stock
	// shows up here; lets Claude map the real Enhanced offsets.
	r += "  -- non-zero globals 1845560..1846360 (business BD) --\n";
	ScanNonZeroGlobals(1845560, 800, r);
	r += "  -- non-zero globals 1882640..1882960 (fill/goods/carwash) --\n";
	ScanNonZeroGlobals(1882640, 320, r);
	r += "  -- non-zero globals 1673810..1673840 (MC supplies) --\n";
	ScanNonZeroGlobals(1673810, 30, r);
	r += "\n";

	// ---- Restocker tunables ----
	r += "--- RESTOCKER TUNABLES (resolve check) ---\n";
	if (TunableService::IsLoaded())
	{
		struct T { const char* label; uint32_t hash; };
		std::vector<T> ts = {
			{ "nightclubincomeuptopop100", DMAScript::Joaat("nightclubincomeuptopop100") },
			{ "meth_prod 1370024930", 1370024930u },
			{ "weed_prod (-635596193)", (uint32_t)-635596193 },
			{ "coke_prod 702413484", 702413484u },
			{ "bunker_prod 215868155", 215868155u },
			{ "acid (-672998848)", (uint32_t)-672998848 },
		};
		for (auto& t : ts)
		{
			DWORD gi = TunableService::GetTunableGlobalIndex(t.hash);
			int v = 0; bool ok = gi && TunableService::GetTunableInt(t.hash, v);
			r += std::format("  {:<26} hash={:08X} globalIdx={} value={}\n",
				t.label, t.hash, gi, ok ? std::to_string(v) : "-");
		}
	}
	else r += "  (tunables.bin not loaded)\n";

	r += "\n================= END EXPORT =================\n";
	return r;
}

void StatDebug::Render()
{
	// ---- One-click export (works even before stats are ready) ----
	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "One-click: export everything Claude needs.");
	static std::string exportStatus;
	if (ImGui::Button("EXPORT ALL OFFSETS -> dma_offsets.txt", ImVec2(320, 0)))
	{
		std::string report = BuildOffsetReport();
		std::println("{}", report); // also to console
		std::ofstream f("dma_offsets.txt", std::ios::trunc);
		if (f.is_open())
		{
			f << report;
			f.close();
			char cwd[MAX_PATH] = {};
			GetCurrentDirectoryA(MAX_PATH, cwd);
			exportStatus = std::format("Saved to {}\\dma_offsets.txt  -- open it and paste the contents back.", cwd);
		}
		else
			exportStatus = "Failed to open dma_offsets.txt for writing (also dumped to console).";
	}
	if (!exportStatus.empty())
		ImGui::TextWrapped("%s", exportStatus.c_str());
	ImGui::Separator();

	if (!StatsWriter::IsReady())
	{
		ImGui::TextDisabled("Stats not available -- waiting for CStatsMgr (join a session, check console).");
		ImGui::TextDisabled("(The export button above still works for globals/tunables.)");
		return;
	}

	ImGui::Text("Stats loaded: %d | Character index (MP slot): %d",
		StatsWriter::GetStatCount(), StatsWriter::GetCharIndex());
	ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
		"Advanced manual tools below -- the EXPORT button above is all Claude needs.");

	// -------- Packed index resolver --------
	if (ImGui::CollapsingHeader("Packed Stat Resolver", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static int index = 110;
		static int intWriteValue = 1;

		ImGui::SetNextItemWidth(160.f);
		ImGui::InputInt("Packed Index", &index);

		PackedStats::Resolved r = PackedStats::Resolve(index, true);

		if (!r.valid)
		{
			ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1),
				"No range matches index %d (out of known table -- needs newer table for this build).", index);
		}
		else
		{
			ImGui::Text("Type:        %s", r.isBool ? "BOOL (64 bits/stat)" : "INT (8-bit field)");
			ImGui::Text("Range start: %d  tag: %s", r.rangeStart, r.tag);
			ImGui::Text("Backing stat: %s   (sub %d)", r.statName.c_str(), r.sub);
			if (r.isBool)
				ImGui::Text("Bit: %d  (dword +0x%X, shift %d)", r.bit, 0x10 + r.dwordOffset, r.shift);
			else
				ImGui::Text("Field: %d  (dword +0x%X, shift %d, width 8)", r.bit, 0x10 + r.dwordOffset, r.shift);

			if (!r.dataPtr)
				ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1),
					"Stat NOT found in CStatsMgr -- name likely wrong for this build.");
			else
			{
				ImGui::Text("sStatData @ %llX", (unsigned long long)r.dataPtr);
				if (r.valueRead)
					ImGui::TextColored(ImVec4(0.4f, 1, 0.6f, 1), "Current value: %u", r.curValue);
				else
					ImGui::TextDisabled("Current value: <read failed>");

				if (r.isBool)
				{
					if (ImGui::Button("Set TRUE"))
						PackedStats::SetPackedBool(index, true);
					ImGui::SameLine();
					if (ImGui::Button("Set FALSE"))
						PackedStats::SetPackedBool(index, false);
				}
				else
				{
					ImGui::SetNextItemWidth(120.f);
					ImGui::InputInt("##intval", &intWriteValue);
					ImGui::SameLine();
					if (ImGui::Button("Write Int (0-255)"))
						PackedStats::SetPackedInt(index, intWriteValue);
				}
			}
		}
	}

	// -------- Raw stat inspector --------
	if (ImGui::CollapsingHeader("Raw Stat Inspector"))
	{
		static char nameBuf[64] = "MP0_PSTAT_BOOL0";
		ImGui::SetNextItemWidth(300.f);
		ImGui::InputText("Stat Name", nameBuf, sizeof(nameBuf));

		if (ImGui::Button("Resolve + Read"))
			StatsWriter::DiagnoseStat(nameBuf); // detailed dump to console

		uintptr_t dataPtr = StatsWriter::FindStatDataPtrByName(nameBuf);
		if (!dataPtr)
			ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Not found in CStatsMgr");
		else
		{
			uint32_t lo = 0, hi = 0;
			bool okLo = StatsWriter::ReadStatDword(dataPtr, 0, lo);
			bool okHi = StatsWriter::ReadStatDword(dataPtr, 4, hi);
			ImGui::Text("@ %llX", (unsigned long long)dataPtr);
			if (okLo) ImGui::Text("value+0x10 (decoded): %u  (0x%08X)", lo, lo);
			if (okHi) ImGui::Text("value+0x14 (decoded): %u  (0x%08X)", hi, hi);
			if (okLo && okHi)
			{
				unsigned long long v64 = ((unsigned long long)hi << 32) | lo;
				ImGui::Text("as 64-bit: %llu", v64);
			}
			ImGui::TextDisabled("(full byte dump printed to console)");
		}
	}

	// -------- Script Global inspector (for business restocker / guard / gender globals) --------
	if (ImGui::CollapsingHeader("Script Global Inspector"))
	{
		static int globalIndex = 2655288;   // restocker guard global by default
		static bool useAt = false;
		static int atIndex = 0, atSize = 13;
		static int writeValue = 0;

		ImGui::SetNextItemWidth(180.f);
		ImGui::InputInt("Global Index", &globalIndex);
		ImGui::Checkbox("Array .at(index, size)", &useAt);
		if (useAt)
		{
			ImGui::SetNextItemWidth(120.f);
			ImGui::InputInt("at index", &atIndex);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(120.f);
			ImGui::InputInt("elem size", &atSize);
		}

		// .at(i, sz) resolves to base + 1 + i*sz (matches ScriptGlobal:at semantics)
		int effective = useAt ? (globalIndex + 1 + atIndex * atSize) : globalIndex;
		ImGui::Text("Effective index: %d", effective);

		DWORD asInt = DMA::GetGlobalInt(effective);
		float asFloat = DMA::GetGlobalFloat(effective);
		ImGui::TextColored(ImVec4(0.4f, 1, 0.6f, 1), "int: %d (0x%X)   |   float: %.3f",
			(int)asInt, asInt, asFloat);

		ImGui::SetNextItemWidth(140.f);
		ImGui::InputInt("##gwrite", &writeValue);
		ImGui::SameLine();
		if (ImGui::Button("Write Int##global"))
			DMA::SetGlobalInt(effective, (DWORD)writeValue);
		ImGui::SameLine();
		if (ImGui::Button("Write 0.0f##global"))
			DMA::SetGlobalFloat(effective, 0.0f);
	}

	// -------- Script Local Inspector + Finder (for heist hacking / detection locals) --------
	// Use this to discover script locals we don't have hard-coded offsets for --
	// e.g. the Kortz Center hacking-minigame progress local, or a heist detection
	// counter. Workflow to FIND a local: type the running script's name, Snapshot,
	// perform ONE step in-game (advance the hack / get spotted), then Diff -- the
	// changed local index is the one you want. Reading is always safe; only the
	// explicit Write buttons modify game memory.
	if (ImGui::CollapsingHeader("Script Local Inspector + Finder"))
	{
		static char scriptName[64] = "fm_mission_controller_v3"; // Kortz finale controller
		static int localIndex = 0;
		static int writeValue = 0;

		ImGui::SetNextItemWidth(280.f);
		ImGui::InputText("Script name", scriptName, sizeof(scriptName));

		// Quick presets for the heist controllers.
		if (ImGui::SmallButton("fm_mission_controller_v3")) strcpy_s(scriptName, "fm_mission_controller_v3");
		ImGui::SameLine();
		if (ImGui::SmallButton("fm_mission_controller")) strcpy_s(scriptName, "fm_mission_controller");
		ImGui::SameLine();
		if (ImGui::SmallButton("kortz_planning")) strcpy_s(scriptName, "kortz_planning");

		uint32_t scriptHash = DMAScript::Joaat(scriptName);
		uintptr_t thread = DMAScript::FindScriptThread(scriptHash);
		uintptr_t stack = thread ? DMAScript::GetScriptStack(thread) : 0;

		if (!thread)
			ImGui::TextColored(ImVec4(1, 0.5f, 0.3f, 1), "'%s' not running", scriptName);
		else if (!stack)
			ImGui::TextColored(ImVec4(1, 0.5f, 0.3f, 1), "'%s' running but no stack", scriptName);
		else
			ImGui::TextColored(ImVec4(0.4f, 1, 0.6f, 1), "'%s' running (stack ok)", scriptName);

		ImGui::SetNextItemWidth(180.f);
		ImGui::InputInt("Local index", &localIndex);
		if (localIndex < 0) localIndex = 0;

		if (stack)
		{
			int asInt = 0; float asFloat = 0.0f;
			DMAScript::ReadScriptLocal<int>(stack, (size_t)localIndex, asInt);
			DMAScript::ReadScriptLocal<float>(stack, (size_t)localIndex, asFloat);
			ImGui::Text("[%d] int: %d (0x%X)  |  float: %.3f", localIndex, asInt, (uint32_t)asInt, asFloat);

			ImGui::SetNextItemWidth(140.f);
			ImGui::InputInt("##lwrite", &writeValue);
			ImGui::SameLine();
			if (ImGui::Button("Write Int##local"))
				DMAScript::WriteScriptLocal<int>(stack, (size_t)localIndex, writeValue);
		}

		// Snapshot/diff finder over a local range.
		static int scanStart = 0, scanCount = 4000;
		static std::vector<int> snapshot;
		static int snapStart = 0, snapCount = 0;
		static std::string diffStatus;
		static char label[64] = "kortz: advanced hack one step";

		ImGui::Separator();
		ImGui::TextWrapped("Finder: Snapshot, do ONE step in-game, Diff. Changed locals -> your target.");
		ImGui::SetNextItemWidth(120.f);
		ImGui::InputInt("Scan start##local", &scanStart);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.f);
		ImGui::InputInt("Count##local", &scanCount);
		if (scanCount < 1) scanCount = 1;
		if (scanCount > 20000) scanCount = 20000;
		ImGui::SetNextItemWidth(300.f);
		ImGui::InputText("Label##local", label, sizeof(label));

		if (ImGui::Button("1) Snapshot##local", ImVec2(140, 0)))
		{
			if (!stack) { diffStatus = "Script not running -- can't snapshot."; }
			else
			{
				snapshot.resize(scanCount);
				for (int i = 0; i < scanCount; i++)
				{
					int v = 0;
					DMAScript::ReadScriptLocal<int>(stack, (size_t)(scanStart + i), v);
					snapshot[i] = v;
				}
				snapStart = scanStart; snapCount = scanCount;
				diffStatus = std::format("Snapshot: {} locals from {}. Do '{}' in-game, then Diff.",
					scanCount, scanStart, label);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("2) Diff -> file##local", ImVec2(160, 0)))
		{
			if (!stack) diffStatus = "Script not running.";
			else if (snapshot.size() != (size_t)snapCount || snapCount == 0)
				diffStatus = "Snapshot first.";
			else
			{
				std::string rep = std::format("=== LOCAL DIFF [{}] {} (start={} count={}) ===\n",
					label, scriptName, snapStart, snapCount);
				int changes = 0;
				for (int i = 0; i < snapCount; i++)
				{
					int now = 0;
					DMAScript::ReadScriptLocal<int>(stack, (size_t)(snapStart + i), now);
					if (now != snapshot[i])
					{
						float f = 0.0f;
						DMAScript::ReadScriptLocal<float>(stack, (size_t)(snapStart + i), f);
						rep += std::format("  [{}]: {} -> {}{}\n", snapStart + i, snapshot[i], now,
							(f > 0.0001f && f < 1e9f) ? std::format("  (float {:.2f})", f) : std::string());
						changes++;
					}
				}
				rep += std::format("=== {} changed ===\n\n", changes);
				std::println("{}", rep);
				std::ofstream f("dma_local_diff.txt", std::ios::app);
				if (f.is_open()) { f << rep; f.close(); }
				diffStatus = std::format("[{}] {} locals changed -> appended to dma_local_diff.txt.", label, changes);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset##localfile"))
		{
			std::ofstream f("dma_local_diff.txt", std::ios::trunc); f.close();
			diffStatus = "Cleared dma_local_diff.txt.";
		}
		if (!diffStatus.empty())
			ImGui::TextWrapped("%s", diffStatus.c_str());
	}

	// -------- Business Offset Finder (snapshot / diff) --------
	// Workflow: set range -> Snapshot -> (buy/sell/restock a business in-game) -> Diff.
	// The changed indices ARE the business's ownership/stock globals on this build.
	if (ImGui::CollapsingHeader("Business Offset Finder (snapshot/diff)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static int scanStart = 1845000;
		static int scanCount = 3000;
		static std::vector<int> snapshot;
		static int snapStart = 0, snapCount = 0; // range the snapshot was taken at
		static std::string diffStatus;
		static char label[64] = "warehouse: sold 1 crate";

		ImGui::TextWrapped("Workflow: pick a preset, type what you'll do, Snapshot, do it ONCE in-game, "
			"then Diff. Captures append to dma_global_diff.txt -- do warehouse, hangar, nightclub in "
			"turn, then paste the one file back.");

		// Presets -- business BD ~1845560; hangar/warehouse fill-trigger globals ~1882700.
		if (ImGui::SmallButton("Preset: Business BD (1845000 +3000)")) { scanStart = 1845000; scanCount = 3000; }
		ImGui::SameLine();
		if (ImGui::SmallButton("Preset: Fill region (1882500 +700)")) { scanStart = 1882500; scanCount = 700; }
		if (ImGui::SmallButton("Preset: Wide BD (1844000 +6000)")) { scanStart = 1844000; scanCount = 6000; }
		ImGui::SameLine();
		if (ImGui::SmallButton("Preset: Hangar/WH fill+BD wide (1845000 +38000)")) { scanStart = 1845000; scanCount = 38000; }

		ImGui::SetNextItemWidth(140.f);
		ImGui::InputInt("Scan start", &scanStart);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(140.f);
		ImGui::InputInt("Count", &scanCount);
		if (scanCount < 1) scanCount = 1;
		if (scanCount > 20000) scanCount = 20000;

		ImGui::SetNextItemWidth(300.f);
		ImGui::InputText("Label (what you changed)", label, sizeof(label));

		if (ImGui::Button("1) Snapshot", ImVec2(130, 0)))
		{
			snapshot.resize(scanCount);
			for (int i = 0; i < scanCount; i++)
				snapshot[i] = (int)DMA::GetGlobalInt(scanStart + i);
			snapStart = scanStart;
			snapCount = scanCount;
			diffStatus = std::format("Snapshot: {} globals from {}. Now do '{}' in-game, then Diff.",
				scanCount, scanStart, label);
		}
		ImGui::SameLine();
		if (ImGui::Button("2) Diff -> append file", ImVec2(170, 0)))
		{
			if (snapshot.size() != (size_t)snapCount || snapCount == 0)
				diffStatus = "Snapshot first.";
			else
			{
				std::string rep = std::format("=== DIFF [{}] (start={} count={}) ===\n", label, snapStart, snapCount);
				int changes = 0;
				for (int i = 0; i < snapCount; i++)
				{
					int now = (int)DMA::GetGlobalInt(snapStart + i);
					if (now != snapshot[i])
					{
						float f = DMA::GetGlobalFloat(snapStart + i);
						rep += std::format("  [{}] (BD+{}): {} -> {}{}\n",
							snapStart + i, (snapStart + i) - 1845560, snapshot[i], now,
							(f > 0.0001f && f < 1e9f) ? std::format("  (float {:.2f})", f) : std::string());
						changes++;
					}
				}
				rep += std::format("=== {} changed ===\n\n", changes);
				std::println("{}", rep);
				std::ofstream f("dma_global_diff.txt", std::ios::app); // APPEND so multiple captures accumulate
				if (f.is_open()) { f << rep; f.close(); }
				char cwd[MAX_PATH] = {};
				GetCurrentDirectoryA(MAX_PATH, cwd);
				diffStatus = std::format("[{}] {} changed -> appended to {}\\dma_global_diff.txt.",
					label, changes, cwd);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset file"))
		{
			std::ofstream f("dma_global_diff.txt", std::ios::trunc); // clear to start fresh
			f.close();
			diffStatus = "Cleared dma_global_diff.txt.";
		}
		if (!diffStatus.empty())
			ImGui::TextWrapped("%s", diffStatus.c_str());
	}

	// -------- Tunable inspector (for restocker tunable hashes / nightclub income) --------
	if (ImGui::CollapsingHeader("Tunable Inspector"))
	{
		if (!TunableService::IsLoaded())
		{
			ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1),
				"tunables.bin not loaded -- place it next to the exe (%d tunables when loaded).",
				TunableService::TunableCount);
		}
		else
		{
			ImGui::Text("Tunables loaded: %d", TunableService::TunableCount);
			static char tuneName[64] = "nightclubincomeuptopop100";
			static int rawHash = 0;
			static int setVal = 1;

			ImGui::SetNextItemWidth(280.f);
			ImGui::InputText("Tunable Name", tuneName, sizeof(tuneName));
			ImGui::SetNextItemWidth(180.f);
			ImGui::InputInt("or Raw Hash (decimal)", &rawHash);

			uint32_t hash = (tuneName[0] != '\0') ? DMAScript::Joaat(tuneName) : (uint32_t)rawHash;
			ImGui::Text("Using hash: 0x%08X (%u)", hash, hash);

			DWORD gi = TunableService::GetTunableGlobalIndex(hash);
			if (!gi)
				ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Not found in tunable map");
			else
			{
				int val = 0;
				bool ok = TunableService::GetTunableInt(hash, val);
				ImGui::Text("global index: %u | value: %s", gi, ok ? std::to_string(val).c_str() : "<read fail>");
				ImGui::SetNextItemWidth(140.f);
				ImGui::InputInt("##tval", &setVal);
				ImGui::SameLine();
				if (ImGui::Button("Set Tunable Int"))
					TunableService::SetTunableInt(hash, setVal);
			}
		}
	}
}
