#pragma once
#include <cstdint>
#include <vector>
#include <string>

// Represents a remote process module read via DMA.
// Loads the module's code into a local buffer for pattern scanning.
class DMAModule
{
public:
	bool Load(VMM_HANDLE vmh, DWORD pid, const char* moduleName);

	// Remote base address of the module in the target process
	uintptr_t Base() const { return m_Base; }

	// Total image size
	size_t Size() const { return m_ImageSize; }

	// End address (Base + Size)
	uintptr_t End() const { return m_Base + m_ImageSize; }

	// Local buffer containing the module's memory
	const std::vector<uint8_t>& Buffer() const { return m_Buffer; }

	// Read a value from the local buffer at a given remote address
	template<typename T>
	T ReadLocal(uintptr_t remoteAddr) const
	{
		size_t offset = remoteAddr - m_Base;
		if (offset + sizeof(T) > m_Buffer.size())
			return T{};
		return *reinterpret_cast<const T*>(&m_Buffer[offset]);
	}

	// Resolve a RIP-relative address from the local buffer
	// Reads int32_t displacement at remoteAddr and returns remoteAddr + disp + 4
	uintptr_t ResolveRip(uintptr_t remoteAddr) const
	{
		int32_t displacement = ReadLocal<int32_t>(remoteAddr);
		return remoteAddr + displacement + 4;
	}

	bool Valid() const { return m_Base != 0 && !m_Buffer.empty(); }

private:
	uintptr_t m_Base = 0;
	size_t m_ImageSize = 0;
	std::vector<uint8_t> m_Buffer;
};
