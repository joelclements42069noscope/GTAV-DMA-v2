#pragma once
#include <cstdint>

class NoIdleKick
{
public:
	static inline bool bEnable = false;

	static bool OnDMAFrame();

private:
	// Pre-computed JOAAT hashes for idle kick tunables
	static constexpr uint32_t HASHES[] = {
		0xB3A4D684, // IDLEKICK_WARNING1
		0xC16F7219, // IDLEKICK_WARNING2
		0xE009AF49, // IDLEKICK_WARNING3
		0x949C8AAD, // IDLEKICK_KICK
		0x1A768E30, // ConstrainedKick_Warning1
		0x28C02AC3, // ConstrainedKick_Warning2
		0xBE1A5575, // ConstrainedKick_Warning3
		0x1245EB8A, // ConstrainedKick_Kick
	};
	static constexpr int HASH_COUNT = 8;
};
