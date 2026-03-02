#include "pch.h"
#include "core/DMAScriptHelper.h"
#include "offsets/Offsets.h"

namespace DMAScript
{
	uintptr_t FindScriptThread(uint32_t scriptHash)
	{
		if (!Offsets::ScriptThreads)
			return 0;

		// Read the atArray header from remote memory
		uintptr_t arrayAddr = DMA::BaseAddress + Offsets::ScriptThreads;
		DMA_atArrayHeader arrayHeader{};
		if (!DMA::Read(arrayAddr, arrayHeader))
			return 0;

		if (!arrayHeader.m_Data || arrayHeader.m_Size == 0)
			return 0;

		// Read all thread pointers at once (max ~200 threads typically)
		uint16_t count = std::min(arrayHeader.m_Size, (uint16_t)256);
		std::vector<uintptr_t> threadPtrs(count);

		DWORD bytesRead = 0;
		VMMDLL_MemReadEx(DMA::vmh, DMA::PID, arrayHeader.m_Data,
			(BYTE*)threadPtrs.data(), count * sizeof(uintptr_t),
			&bytesRead, VMMDLL_FLAG_NOCACHE);

		if (bytesRead < sizeof(uintptr_t))
			return 0;

		// Search for the matching script hash
		for (uint16_t i = 0; i < count; i++)
		{
			if (!threadPtrs[i])
				continue;

			// Read the script hash from this thread
			uint32_t hash = 0;
			if (!DMA::Read(threadPtrs[i] + ScrThread::SCRIPT_HASH, hash))
				continue;

			if (hash == scriptHash)
				return threadPtrs[i];
		}

		return 0;
	}

	uintptr_t GetScriptStack(uintptr_t threadAddr)
	{
		if (!threadAddr)
			return 0;

		uintptr_t stack = 0;
		if (!DMA::Read(threadAddr + ScrThread::STACK_PTR, stack))
			return 0;

		return stack;
	}

	// Template specializations need to be in the cpp to access DMA

	template<>
	bool ReadScriptLocal<int>(uintptr_t stackAddr, size_t localIndex, int& out)
	{
		if (!stackAddr)
			return false;
		uintptr_t addr = stackAddr + (localIndex * sizeof(uintptr_t));
		return DMA::Read(addr, out);
	}

	template<>
	bool ReadScriptLocal<float>(uintptr_t stackAddr, size_t localIndex, float& out)
	{
		if (!stackAddr)
			return false;
		uintptr_t addr = stackAddr + (localIndex * sizeof(uintptr_t));
		return DMA::Read(addr, out);
	}

	template<>
	bool ReadScriptLocal<int>(uint32_t scriptHash, size_t localIndex, int& out)
	{
		auto thread = FindScriptThread(scriptHash);
		if (!thread) return false;
		auto stack = GetScriptStack(thread);
		return ReadScriptLocal(stack, localIndex, out);
	}

	template<>
	bool ReadScriptLocal<float>(uint32_t scriptHash, size_t localIndex, float& out)
	{
		auto thread = FindScriptThread(scriptHash);
		if (!thread) return false;
		auto stack = GetScriptStack(thread);
		return ReadScriptLocal(stack, localIndex, out);
	}

	template<>
	bool WriteScriptLocal<int>(uintptr_t stackAddr, size_t localIndex, const int& value)
	{
		if (!stackAddr)
			return false;
		uintptr_t addr = stackAddr + (localIndex * sizeof(uintptr_t));
		return DMA::Write(addr, value);
	}

	template<>
	bool WriteScriptLocal<float>(uintptr_t stackAddr, size_t localIndex, const float& value)
	{
		if (!stackAddr)
			return false;
		uintptr_t addr = stackAddr + (localIndex * sizeof(uintptr_t));
		return DMA::Write(addr, value);
	}

	template<>
	bool WriteScriptLocal<int>(uint32_t scriptHash, size_t localIndex, const int& value)
	{
		auto thread = FindScriptThread(scriptHash);
		if (!thread) return false;
		auto stack = GetScriptStack(thread);
		return WriteScriptLocal(stack, localIndex, value);
	}

	template<>
	bool WriteScriptLocal<float>(uint32_t scriptHash, size_t localIndex, const float& value)
	{
		auto thread = FindScriptThread(scriptHash);
		if (!thread) return false;
		auto stack = GetScriptStack(thread);
		return WriteScriptLocal(stack, localIndex, value);
	}
}
