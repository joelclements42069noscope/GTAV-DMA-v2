#include "pch.h"
#include "PlayerList.h"
#include "offsets/Offsets.h"

bool PlayerList::OnDMAFrame()
{
	if (!bEnable)
		return true;

	if (!Offsets::NetworkPlayerMgr)
		return false;

	// Read CNetworkPlayerMgr*
	uintptr_t pMgr = 0;
	if (!DMA::Read(DMA::BaseAddress + Offsets::NetworkPlayerMgr, pMgr) || !pMgr)
		return false;

	// Read all 32 player pointers at once (scatter read for efficiency)
	uintptr_t playerPtrs[32] = {};
	DWORD bytesRead = 0;
	VMMDLL_MemReadEx(DMA::vmh, DMA::PID,
		pMgr + PLAYERS_ARRAY_OFFSET,
		(BYTE*)playerPtrs, sizeof(playerPtrs),
		&bytesRead, VMMDLL_FLAG_NOCACHE);

	if (bytesRead != sizeof(playerPtrs))
		return false;

	ConnectedCount = 0;

	for (int i = 0; i < 32; i++)
	{
		Players[i].Connected = false;
		Players[i].PlayerIndex = static_cast<uint8_t>(i);
		Players[i].RockstarId = 0;

		if (!playerPtrs[i])
			continue;

		// Read player index
		uint8_t idx = 0;
		DMA::Read(playerPtrs[i] + PLAYER_INDEX_OFFSET, idx);
		Players[i].PlayerIndex = idx;

		// Read Rockstar ID
		int64_t rid = 0;
		DMA::Read(playerPtrs[i] + ROCKSTAR_ID_OFFSET, rid);
		Players[i].RockstarId = rid;

		Players[i].Connected = true;
		ConnectedCount++;
	}

	return true;
}

void PlayerList::Render()
{
	if (!bEnable)
		return;

	if (ImGui::CollapsingHeader("Player List"))
	{
		ImGui::Text("Connected Players: %d", ConnectedCount);
		ImGui::Separator();

		if (ImGui::BeginTable("##players", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 40.0f);
			ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableSetupColumn("Rockstar ID", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

			for (int i = 0; i < 32; i++)
			{
				if (!Players[i].Connected)
					continue;

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%d", i);
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%d", Players[i].PlayerIndex);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%lld", Players[i].RockstarId);
			}

			ImGui::EndTable();
		}
	}
}
