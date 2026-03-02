#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Pre-computed heightmap for GTA V ground Z lookup.
// Used by waypoint teleport since we can't call GET_GROUND_Z over DMA.
// Data sourced from: https://github.com/Andreas1331/ragemp-gtav-heightmap
//
// File format (compact):
//   Header (20 bytes):
//     float minX, minY     -- map origin
//     float cellSize       -- meters per cell
//     uint32_t gridW, gridH -- grid dimensions
//   Data:
//     float[gridW * gridH] -- row-major, Y-first (row = Y index, col = X index)

class Heightmap
{
public:
	// Load heightmap from file. Returns true on success.
	static bool Load(const std::string& filePath = "heightmap.bin");

	// Get ground Z at world X/Y. Returns fallback if not loaded or out of bounds.
	static float GetZ(float worldX, float worldY, float fallback = 300.0f);

	static bool IsLoaded() { return bLoaded; }

private:
	static inline bool bLoaded = false;

	// Map bounds (from RageMP heightmap)
	static inline float MinX = 0, MinY = 0;
	static inline float CellSize = 0;
	static inline uint32_t GridW = 0, GridH = 0;

	static inline std::vector<float> Data;
};
