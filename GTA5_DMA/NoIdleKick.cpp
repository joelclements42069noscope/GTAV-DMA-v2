#include "pch.h"
#include "NoIdleKick.h"
#include "TunableService.h"

bool NoIdleKick::OnDMAFrame()
{
	if (!bEnable)
		return true;

	if (!TunableService::IsLoaded())
		return true;

	// Write INT_MAX to all idle kick tunables to prevent being kicked
	for (int i = 0; i < HASH_COUNT; i++)
	{
		TunableService::SetTunableInt(HASHES[i], INT_MAX);
	}

	return true;
}
