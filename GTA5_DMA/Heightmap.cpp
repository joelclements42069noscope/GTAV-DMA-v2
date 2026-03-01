#include "pch.h"
#include "Heightmap.h"

bool Heightmap::Load(const std::string& filePath)
{
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open())
	{
		std::println("[Heightmap] Could not open {} (place next to exe for waypoint Z lookup)", filePath);
		return false;
	}

	// Read header
	file.read(reinterpret_cast<char*>(&MinX), sizeof(float));
	file.read(reinterpret_cast<char*>(&MinY), sizeof(float));
	file.read(reinterpret_cast<char*>(&CellSize), sizeof(float));
	file.read(reinterpret_cast<char*>(&GridW), sizeof(uint32_t));
	file.read(reinterpret_cast<char*>(&GridH), sizeof(uint32_t));

	if (!file.good() || CellSize <= 0 || GridW == 0 || GridH == 0)
	{
		std::println("[Heightmap] Invalid header");
		return false;
	}

	size_t totalCells = static_cast<size_t>(GridW) * GridH;
	Data.resize(totalCells);
	file.read(reinterpret_cast<char*>(Data.data()), totalCells * sizeof(float));

	if (!file.good())
	{
		std::println("[Heightmap] Failed to read data ({} cells expected)", totalCells);
		Data.clear();
		return false;
	}

	bLoaded = true;
	std::println("[Heightmap] Loaded {}x{} grid (cell={:.0f}m, bounds=[{:.0f},{:.0f}], {:.1f}MB)",
		GridW, GridH, CellSize, MinX, MinY,
		(totalCells * sizeof(float)) / (1024.0f * 1024.0f));
	return true;
}

float Heightmap::GetZ(float worldX, float worldY, float fallback)
{
	if (!bLoaded)
		return fallback;

	// Map world coords to grid indices
	float gx = (worldX - MinX) / CellSize;
	float gy = (worldY - MinY) / CellSize;

	int ix = static_cast<int>(gx);
	int iy = static_cast<int>(gy);

	if (ix < 0 || ix >= static_cast<int>(GridW) || iy < 0 || iy >= static_cast<int>(GridH))
		return fallback;

	float z = Data[static_cast<size_t>(iy) * GridW + ix];

	// 0 means no data (ocean/unsampled) -- use fallback
	if (z == 0.0f)
		return fallback;

	return z;
}
