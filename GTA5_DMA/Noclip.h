#pragma once

// DMA Noclip -- moves player by writing position each frame.
// Writes to BOTH fwEntity::m_Transform (entity+0x90) and CNavigation::Position (nav+0x50).
// Movement is relative to entity heading (forward/right vectors from transform matrix).
// Sets collision-proof GodFlags during noclip to reduce physics fighting.

class Noclip
{
public:
	static inline bool bEnable = false;
	static inline float Speed = 1.0f; // speed multiplier

	static bool OnDMAFrame();
	static void Render();

	// Movement state -- set by Render() (ImGui thread), read by OnDMAFrame() (DMA thread)
	static inline bool MoveForward = false;
	static inline bool MoveBack = false;
	static inline bool MoveLeft = false;
	static inline bool MoveRight = false;
	static inline bool MoveUp = false;
	static inline bool MoveDown = false;

private:
	static constexpr ptrdiff_t ENTITY_RIGHT_OFFSET = 0x60;   // fwEntity::m_Transform.rows[0] (right vector)
	static constexpr ptrdiff_t ENTITY_FORWARD_OFFSET = 0x70; // fwEntity::m_Transform.rows[1] (forward vector)
	static constexpr ptrdiff_t ENTITY_POS_OFFSET = 0x90;     // fwEntity::m_Transform.rows[3] (position)

	static constexpr uint32_t GODFLAGS_COLLISION_PROOF = 0x08; // Bit 3 in GodFlags

	static inline Vec3 NoclipPosition = { 0, 0, 0 };
	static inline Vec3 CachedForward = { 0, 1, 0 };  // Last valid forward heading
	static inline Vec3 CachedRight = { 1, 0, 0 };    // Last valid right heading
	static inline bool PositionInitialized = false;
	static inline uint32_t SavedGodFlags = 0;
	static inline bool GodFlagsSaved = false;
	static inline ULONGLONG LastTickMs = 0;
	static inline int FrameCount = 0;
};
