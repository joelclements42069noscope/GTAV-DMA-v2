#include "pch.h"
#include "DailyObjectives.h"
#include "StatsWriter.h"

void DailyObjectives::CompleteTimeTrials()
{
	int ok = 0;
	// Standard Time Trial
	if (StatsWriter::SetStatInt("MPPLY_TIMETRIAL_COMPLETED_WEEK", 1)) ok++;
	// RC Bandito Time Trial
	if (StatsWriter::SetStatInt("MPPLY_RCTTCOMPLETEDWEEK", 1)) ok++;
	// Junk Energy Bike Time Trial
	if (StatsWriter::SetStatInt("MPPLY_BTTCOMPLETED", 1)) ok++;

	std::println("[DailyObjectives] Time Trials: {}/3 written", ok);
	StatusText = (ok == 3) ? "Time Trials completed!" : "Time Trials: some failed";
}

void DailyObjectives::CompleteExoticExports()
{
	int ok = 0;
	// All 10 vehicles delivered (bitmask 1023 = 0x3FF = all 10 bits set)
	if (StatsWriter::SetStatInt("MPX_CBV_DELIVERED_BS", 1023)) ok++;
	// Set state to indicate completion
	if (StatsWriter::SetStatInt("MPX_CBV_STATE", 2)) ok++;

	std::println("[DailyObjectives] Exotic Exports: {}/2 written", ok);
	StatusText = (ok == 2) ? "Exotic Exports completed!" : "Exotic Exports: some failed";
}

void DailyObjectives::CompleteSkydives()
{
	int ok = 0;
	for (int i = 0; i < 10; i++)
	{
		std::string statName = "MPX_DAILYCOLLECT_SKYDIVES" + std::to_string(i);
		if (StatsWriter::SetStatInt(statName, 1)) ok++;
	}

	std::println("[DailyObjectives] Skydives: {}/10 written", ok);
	StatusText = (ok == 10) ? "Skydives completed!" : "Skydives: some failed";
}

void DailyObjectives::Render()
{
	if (!StatsWriter::IsReady())
	{
		ImGui::TextDisabled("Stats not available - waiting for CStatsMgr (check console)");
		return;
	}
	ImGui::Text("Stats loaded: %d entries", StatsWriter::GetStatCount());

	if (ImGui::Button("Complete Time Trials"))
		CompleteTimeTrials();
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Standard, RC Bandito, and Junk Energy Bike time trials");

	if (ImGui::Button("Complete Exotic Exports"))
		CompleteExoticExports();
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Marks all 10 exotic export vehicles as delivered");

	if (ImGui::Button("Complete Skydive Collections"))
		CompleteSkydives();
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("All 10 skydive collection locations");

	if (ImGui::Button("Complete All##daily"))
	{
		CompleteTimeTrials();
		CompleteExoticExports();
		CompleteSkydives();
		StatusText = "All daily activities applied!";
	}

	if (StatusText && StatusText[0])
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%s", StatusText);
}
