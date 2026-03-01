#pragma once
#include <cstdint>
#include <vector>
#include <intrin.h>

// DMA-compatible pool iteration for GTA V Enhanced Edition.
// Decrypts PoolEncryption pointers, reads fwBasePool/fwVehiclePool structures,
// and iterates valid entity pointers.

namespace PoolIterator
{
	// PoolEncryption structure (encrypted pointer to fwBasePool)
	struct PoolEncryption
	{
		bool m_IsSet;       // 0x00
		char pad[7];        // 0x01
		uint64_t m_First;   // 0x08
		uint64_t m_Second;  // 0x10
	};
	static_assert(sizeof(PoolEncryption) == 0x18);

	// fwBasePool structure (for Peds, Objects)
	struct fwBasePool
	{
		char pad_0000[8];       // 0x00 - vtable
		uintptr_t m_Entries;    // 0x08 - base pointer for entity data
		uintptr_t m_Flags;      // 0x10 - pointer to flag byte array
		uint32_t m_Size;        // 0x18 - total pool capacity
		uint32_t m_ItemSize;    // 0x1C - size of each entry in bytes
	};
	static_assert(offsetof(fwBasePool, m_Entries) == 0x08);
	static_assert(offsetof(fwBasePool, m_Size) == 0x18);

	// fwVehiclePool structure (different layout, not encrypted)
	struct fwVehiclePool
	{
		uintptr_t m_PoolAddress;  // 0x00 - array of pointers
		uint32_t m_Size;          // 0x08 - pool capacity
		uint32_t pad;             // 0x0C
		uintptr_t m_BitArray;     // 0x10 - validity bitfield pointer
		uint32_t m_ItemCount;     // 0x18 - current count
	};

	// Decrypt PoolEncryption to get fwBasePool pointer
	// rotateExtra: +2 for PedPool, +3 for ObjectPool
	inline uintptr_t DecryptPool(const PoolEncryption& enc, int rotateExtra)
	{
		uint64_t x = _rotl64(enc.m_Second, 30);
		return ~_rotl64(_rotl64(x ^ enc.m_First, 32), (int)((uint8_t)x & 0x1F) + rotateExtra);
	}

	// Get all valid entity pointers from an encrypted pool (Ped/Object).
	// Reads the pool structure from game memory, then iterates checking flag bytes.
	inline std::vector<uintptr_t> GetEntitiesFromEncryptedPool(uintptr_t poolEncryptionAddr, int rotateExtra)
	{
		std::vector<uintptr_t> result;

		// Read PoolEncryption struct
		PoolEncryption enc{};
		if (!DMA::Read(poolEncryptionAddr, enc) || !enc.m_IsSet)
			return result;

		// Decrypt to get fwBasePool pointer
		uintptr_t poolPtr = DecryptPool(enc, rotateExtra);
		if (!poolPtr || poolPtr < 0x10000)
			return result;

		// Read fwBasePool header
		fwBasePool pool{};
		if (!DMA::Read(poolPtr, pool) || pool.m_Size == 0 || pool.m_Size > 50000)
			return result;

		if (!pool.m_Entries || !pool.m_Flags)
			return result;

		// Read all flag bytes at once
		std::vector<uint8_t> flags(pool.m_Size);
		DWORD bytesRead = 0;
		VMMDLL_MemReadEx(DMA::vmh, DMA::PID, pool.m_Flags,
			flags.data(), pool.m_Size, &bytesRead, VMMDLL_FLAG_NOCACHE);
		if (bytesRead != pool.m_Size)
			return result;

		// Iterate pool and collect valid entity pointers
		result.reserve(256);
		for (uint32_t i = 0; i < pool.m_Size; i++)
		{
			// Valid if high bit is NOT set
			if (flags[i] & 0x80)
				continue;

			// Entity address = m_Entries + index * m_ItemSize
			uintptr_t entityAddr = pool.m_Entries + (uintptr_t)i * pool.m_ItemSize;

			// Quick sanity: read first 8 bytes (vtable) to verify pointer is valid
			uintptr_t vtable = 0;
			if (DMA::Read(entityAddr, vtable) && vtable > 0x10000)
				result.push_back(entityAddr);
		}

		return result;
	}

	// Get all valid vehicle pointers from the vehicle pool (not encrypted, double-dereferenced)
	inline std::vector<uintptr_t> GetVehicles(uintptr_t vehiclePoolPtrAddr)
	{
		std::vector<uintptr_t> result;

		// VehiclePool is fwVehiclePool*** -- needs double dereference
		uintptr_t ptr1 = 0;
		if (!DMA::Read(vehiclePoolPtrAddr, ptr1) || !ptr1)
			return result;

		uintptr_t poolPtr = 0;
		if (!DMA::Read(ptr1, poolPtr) || !poolPtr)
			return result;

		fwVehiclePool pool{};
		if (!DMA::Read(poolPtr, pool) || pool.m_Size == 0 || pool.m_Size > 10000)
			return result;

		if (!pool.m_PoolAddress || !pool.m_BitArray)
			return result;

		// Read pointer array
		uint32_t ptrArraySize = pool.m_Size * sizeof(uintptr_t);
		std::vector<uintptr_t> ptrs(pool.m_Size);
		DWORD bytesRead = 0;
		VMMDLL_MemReadEx(DMA::vmh, DMA::PID, pool.m_PoolAddress,
			(BYTE*)ptrs.data(), ptrArraySize, &bytesRead, VMMDLL_FLAG_NOCACHE);
		if (bytesRead != ptrArraySize)
			return result;

		// Read bit array
		uint32_t bitArraySize = ((pool.m_Size + 31) / 32) * sizeof(uint32_t);
		std::vector<uint32_t> bits(bitArraySize / sizeof(uint32_t));
		VMMDLL_MemReadEx(DMA::vmh, DMA::PID, pool.m_BitArray,
			(BYTE*)bits.data(), bitArraySize, &bytesRead, VMMDLL_FLAG_NOCACHE);
		if (bytesRead != bitArraySize)
			return result;

		result.reserve(128);
		for (uint32_t i = 0; i < pool.m_Size; i++)
		{
			if ((bits[i >> 5] >> (i & 0x1F)) & 1)
			{
				if (ptrs[i] > 0x10000)
					result.push_back(ptrs[i]);
			}
		}

		return result;
	}

	// Helper: Read model hash from an entity pointer
	// fwEntity->m_ModelInfo at 0x20, CBaseModelInfo->m_Hash at 0x18
	inline uint32_t GetEntityModelHash(uintptr_t entityAddr)
	{
		uintptr_t modelInfoPtr = 0;
		if (!DMA::Read(entityAddr + 0x20, modelInfoPtr) || !modelInfoPtr)
			return 0;

		uint32_t hash = 0;
		DMA::Read(modelInfoPtr + 0x18, hash);
		return hash;
	}

	// Helper: Check if entity is a player ped (has non-null PlayerInfo pointer)
	// PED->pPlayerInfo at 0x10A8
	inline bool IsPlayerPed(uintptr_t pedAddr)
	{
		uintptr_t playerInfo = 0;
		if (!DMA::Read(pedAddr + offsetof(PED, pPlayerInfo), playerInfo))
			return false;
		return playerInfo != 0;
	}
}
