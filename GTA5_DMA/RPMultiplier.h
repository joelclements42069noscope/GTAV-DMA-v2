#pragma once
#include <cstdint>

class RPMultiplier
{
public:
	static inline bool bEnable = false;
	static inline float Multiplier = 1.0f;

	static bool OnDMAFrame();

private:
	static constexpr uint32_t XP_MULTIPLIER_HASH = 0xB6E46AB9; // JOAAT("XP_MULTIPLIER")
	static constexpr float XP_CLOUD_VALUE = 75.0f;              // R* cloud default for XP_MULTIPLIER
	static inline DWORD ResolvedGlobalIdx = 0;                  // Found via scan if hash lookup fails
	static inline bool ScanAttempted = false;
};
