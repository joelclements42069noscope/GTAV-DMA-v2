#pragma once
#include <cstdint>
#include <string>

// DMA-compatible script thread access
// Allows reading/writing script local variables and finding script threads by hash

namespace DMAScript
{
	// JOAAT hash function (Jenkins One At A Time) for script name hashing
	constexpr uint32_t Joaat(const char* str)
	{
		uint32_t hash = 0;
		while (*str)
		{
			hash += static_cast<uint32_t>(*str >= 'A' && *str <= 'Z' ? *str + 32 : *str);
			hash += (hash << 10);
			hash ^= (hash >> 6);
			str++;
		}
		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);
		return hash;
	}

	// DMA-compatible atArray layout (matches rage::atArray<T>)
	struct DMA_atArrayHeader
	{
		uintptr_t m_Data;     // 0x00 - pointer to data array
		uint16_t m_Size;      // 0x08 - number of elements
		uint16_t m_Capacity;  // 0x0A - allocated capacity
	};
	static_assert(sizeof(DMA_atArrayHeader) == 0x10);

	// DMA-compatible scrThread layout (offsets only, no virtual functions)
	// Based on rage::scrThread from YimMenu V2
	namespace ScrThread
	{
		constexpr size_t VTABLE = 0x00;
		constexpr size_t CONTEXT = 0x08;          // scrThread::Context
		constexpr size_t CONTEXT_THREAD_ID = 0x08; // Context.m_ThreadId
		constexpr size_t CONTEXT_SCRIPT_HASH = 0x10; // Context.m_ScriptHash (uint64)
		constexpr size_t CONTEXT_STATE = 0x18;     // Context.m_State
		constexpr size_t STACK_PTR = 0xB8;         // void* m_Stack
		constexpr size_t SCRIPT_HASH = 0x150;      // uint32_t m_ScriptHash
		constexpr size_t SCRIPT_NAME = 0x154;       // char[64] m_ScriptName
		constexpr size_t STRUCT_SIZE = 0x198;
	}

	// Find a script thread by its JOAAT hash
	// Returns the remote address of the scrThread, or 0 if not found
	uintptr_t FindScriptThread(uint32_t scriptHash);

	// Get the stack pointer for a script thread
	// Returns the remote address of the thread's stack, or 0 if not found
	uintptr_t GetScriptStack(uintptr_t threadAddr);

	// Read a script local variable (int-sized, 8 bytes per slot)
	template<typename T>
	bool ReadScriptLocal(uintptr_t stackAddr, size_t localIndex, T& out);

	// Write a script local variable
	template<typename T>
	bool WriteScriptLocal(uintptr_t stackAddr, size_t localIndex, const T& value);

	// Convenience: find thread + read local in one call
	template<typename T>
	bool ReadScriptLocal(uint32_t scriptHash, size_t localIndex, T& out);

	// Convenience: find thread + write local in one call
	template<typename T>
	bool WriteScriptLocal(uint32_t scriptHash, size_t localIndex, const T& value);
}

// User-defined literal for JOAAT hashing (e.g., "casino_slots"_J)
constexpr uint32_t operator""_J(const char* str, size_t)
{
	return DMAScript::Joaat(str);
}
