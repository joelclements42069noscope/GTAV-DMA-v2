#include "pch.h"
#include "RPMultiplier.h"
#include "TunableService.h"

bool RPMultiplier::OnDMAFrame()
{
	if (!bEnable)
		return true;

	if (!TunableService::IsLoaded())
		return true;

	// Try hash lookup first
	DWORD globalIdx = TunableService::GetTunableGlobalIndex(XP_MULTIPLIER_HASH);

	if (globalIdx != 0)
	{
		// Hash found in tunables.bin -- use it directly
		TunableService::SetTunableFloat(XP_MULTIPLIER_HASH, Multiplier);
		return true;
	}

	// Hash not found -- try to find the tunable by scanning for its known cloud value (75.0)
	if (!ScanAttempted)
	{
		ScanAttempted = true;
		std::println("[RPMult] XP_MULTIPLIER hash not in tunables.bin, scanning for cloud value {:.1f}...", XP_CLOUD_VALUE);

		auto matches = TunableService::ScanForFloat(XP_CLOUD_VALUE);
		if (matches.size() == 1)
		{
			ResolvedGlobalIdx = matches[0];
			std::println("[RPMult] Found unique match! Using globalIdx={}", ResolvedGlobalIdx);
		}
		else if (matches.size() > 1)
		{
			// Multiple matches -- log them all, use the first
			ResolvedGlobalIdx = matches[0];
			std::println("[RPMult] Found {} matches, using first: globalIdx={}", matches.size(), ResolvedGlobalIdx);
		}
		else
		{
			std::println("[RPMult] No tunable with value {:.1f} found. RP multiplier will not work.", XP_CLOUD_VALUE);
		}
	}

	if (ResolvedGlobalIdx != 0)
	{
		TunableService::SetGlobalFloat(ResolvedGlobalIdx, Multiplier);
	}

	return true;
}
