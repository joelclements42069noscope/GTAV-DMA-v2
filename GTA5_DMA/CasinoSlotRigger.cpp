#include "pch.h"
#include "CasinoSlotRigger.h"
#include "core/DMAScriptHelper.h"
#include "offsets/Offsets.h"
#include <cstdlib>

// JOAAT hash for "casino_slots" script
static constexpr uint32_t CASINO_SLOTS_HASH = DMAScript::Joaat("casino_slots");

bool CasinoSlotRigger::IsBlacklisted(int index)
{
	return index == 9 || index == 21 || index == 22 || index == 87 || index == 152;
}

bool CasinoSlotRigger::IsSafeSpinState(int state)
{
	return state == 8 || state == 14;
}

int CasinoSlotRigger::RandomLosingValue()
{
	// Values 3-9 excluding 6 (the winning value)
	// Possible: 3, 4, 5, 7, 8, 9  (6 values)
	int val = 3 + (std::rand() % 7); // [3,9]
	if (val == WINNING_VALUE)
		val = 9; // replace 6 with 9
	return val;
}

void CasinoSlotRigger::WriteSlotValues(uintptr_t stack, bool win)
{
	for (int i = 3; i <= 196; i++)
	{
		if (IsBlacklisted(i))
			continue;

		int value = win ? WINNING_VALUE : RandomLosingValue();
		DMAScript::WriteScriptLocal(stack, SLOTS_RESULTS_TABLE + i, value);
	}
}

bool CasinoSlotRigger::OnDMAFrame()
{
	bool isActive = bRigWin || bAutoMode;

	// When turned off: write losing values once to prevent continued wins
	if (!isActive)
	{
		if (WasRigging)
		{
			if (Offsets::ScriptThreads)
			{
				uintptr_t threadAddr = DMAScript::FindScriptThread(CASINO_SLOTS_HASH);
				if (threadAddr)
				{
					uintptr_t stack = DMAScript::GetScriptStack(threadAddr);
					if (stack)
					{
						std::println("[Slots] Disabled -- writing losing values to prevent continued wins");
						WriteSlotValues(stack, false);
					}
				}
			}
			WasRigging = false;
			LastSpinState = -1;
			AutoWinCount = 0;
			AutoLoseCount = 0;
		}
		return true;
	}

	if (!Offsets::ScriptThreads)
	{
		static bool warned = false;
		if (!warned) { std::println("[Slots] ScriptThreads offset is 0 (pattern scan failed)"); warned = true; }
		return true;
	}

	uintptr_t threadAddr = DMAScript::FindScriptThread(CASINO_SLOTS_HASH);
	if (!threadAddr)
	{
		static bool warned = false;
		if (!warned) { std::println("[Slots] casino_slots script not found (are you at a slot machine?)"); warned = true; }
		return true;
	}

	uintptr_t stack = DMAScript::GetScriptStack(threadAddr);
	if (!stack)
		return true;

	int spinState = 0;
	DMAScript::ReadScriptLocal(stack, SPIN_STATE_VAR, spinState);

	if (!IsSafeSpinState(spinState))
	{
		LastSpinState = spinState;
		return true;
	}

	WasRigging = true;

	if (bRigWin)
	{
		// Always-win mode: check if values already set
		bool needsWrite = false;
		for (int i = 3; i <= 196; i++)
		{
			if (IsBlacklisted(i))
				continue;

			int currentValue = 0;
			DMAScript::ReadScriptLocal(stack, SLOTS_RESULTS_TABLE + i, currentValue);
			if (currentValue != WINNING_VALUE)
			{
				needsWrite = true;
				break;
			}
		}

		if (needsWrite)
		{
			std::println("[Slots] Writing winning values (spin state: {})", spinState);
			WriteSlotValues(stack, true);
		}
	}
	else if (bAutoMode)
	{
		// Auto mode: win based on WinRate percentage
		// Detect new spin by transition from non-safe to safe state
		bool newSpin = (LastSpinState != -1 && !IsSafeSpinState(LastSpinState));

		if (newSpin || LastSpinState == -1)
		{
			bool win = (std::rand() % 100) < WinRate;
			std::println("[Slots] Auto mode: {} (rate={}%, spin state: {}, W:{} L:{})",
				win ? "WIN" : "LOSE", WinRate, spinState, AutoWinCount, AutoLoseCount);

			WriteSlotValues(stack, win);

			if (win) AutoWinCount++;
			else AutoLoseCount++;
		}
	}

	LastSpinState = spinState;
	return true;
}

void CasinoSlotRigger::Render()
{
	ImGui::Checkbox("Rig Slots: Always Win", &bRigWin);
	if (bRigWin && bAutoMode)
		bAutoMode = false; // mutually exclusive

	ImGui::Checkbox("Rig Slots: Auto", &bAutoMode);
	if (bAutoMode && bRigWin)
		bRigWin = false; // mutually exclusive

	if (bAutoMode)
	{
		ImGui::SliderInt("Win Rate %", &WinRate, 0, 100);
		ImGui::Text("W:%d  L:%d", AutoWinCount, AutoLoseCount);
	}
}
