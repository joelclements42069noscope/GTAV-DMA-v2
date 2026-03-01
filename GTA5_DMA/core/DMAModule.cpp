#include "pch.h"
#include "core/DMAModule.h"

bool DMAModule::Load(VMM_HANDLE vmh, DWORD pid, const char* moduleName)
{
	// Get module base address
	m_Base = VMMDLL_ProcessGetModuleBaseU(vmh, pid, moduleName);
	if (!m_Base)
	{
		std::println("[DMAModule] Failed to get base address for {}", moduleName);
		return false;
	}

	// Get module map to find image size
	PVMMDLL_MAP_MODULE pModuleMap = nullptr;
	if (!VMMDLL_Map_GetModuleU(vmh, pid, &pModuleMap, VMMDLL_MODULE_FLAG_NORMAL))
	{
		std::println("[DMAModule] Failed to get module map for {}", moduleName);
		return false;
	}

	// Find our module in the map
	for (DWORD i = 0; i < pModuleMap->cMap; i++)
	{
		if (_stricmp(pModuleMap->pMap[i].uszText, moduleName) == 0)
		{
			m_ImageSize = pModuleMap->pMap[i].cbImageSize;
			break;
		}
	}

	VMMDLL_MemFree(pModuleMap);

	if (!m_ImageSize)
	{
		std::println("[DMAModule] Could not determine image size for {}", moduleName);
		return false;
	}

	std::println("[DMAModule] {} base: {:X}, size: {:X}", moduleName, m_Base, m_ImageSize);

	// Allocate local buffer and read the entire module image
	m_Buffer.resize(m_ImageSize, 0);

	// Read in chunks to handle potential page boundary issues
	constexpr size_t CHUNK_SIZE = 0x100000; // 1MB chunks
	size_t totalRead = 0;

	for (size_t offset = 0; offset < m_ImageSize; offset += CHUNK_SIZE)
	{
		size_t readSize = std::min(CHUNK_SIZE, m_ImageSize - offset);
		DWORD bytesRead = 0;

		VMMDLL_MemReadEx(vmh, pid, m_Base + offset, m_Buffer.data() + offset, (DWORD)readSize, &bytesRead,
			VMMDLL_FLAG_ZEROPAD_ON_FAIL);

		totalRead += bytesRead;
	}

	std::println("[DMAModule] Read {:X}/{:X} bytes from {}", totalRead, m_ImageSize, moduleName);

	if (totalRead < m_ImageSize / 2)
	{
		std::println("[DMAModule] Too few bytes read, module dump may be incomplete");
		return false;
	}

	return true;
}
