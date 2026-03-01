#pragma once

class DMA
{
public: /* Interface variables */
	static inline VMM_HANDLE vmh = 0;
	static inline DWORD PID = 0;

	static inline uintptr_t BaseAddress = 0;
	static inline uintptr_t LocalPlayerAddress = 0;
	static inline uintptr_t NavigationAddress = 0;
	static inline uintptr_t PlayerInfoAddress = 0;
	static inline uintptr_t VehicleAddress = 0;
	static inline uintptr_t VehicleNavigationAddress = 0;
	static inline uintptr_t WeaponInventoryAddress = 0;
	static inline uintptr_t WeaponManagerAddress = 0;
	static inline uintptr_t WeaponInfoAddress = 0;

	static inline Vec3 LocalPlayerLocation = { 0,0,0 };

public: /* DMA Interface function */
	static bool Initialize();
	static bool ScanPatterns(); // Auto offset resolution
	static bool DMAThreadEntry();
	static bool UpdatePlayerCurrentLocation();
	static bool UpdateVehicleInformation();

public: /* Generic memory helpers */
	template <typename T>
	static bool Read(uintptr_t address, T& out)
	{
		DWORD bytesRead = 0;
		VMMDLL_MemReadEx(vmh, PID, address, (BYTE*)&out, sizeof(T), &bytesRead, VMMDLL_FLAG_NOCACHE);
		return bytesRead == sizeof(T);
	}

	template <typename T>
	static bool Write(uintptr_t address, const T& value)
	{
		return VMMDLL_MemWrite(vmh, PID, address, (BYTE*)&value, sizeof(T));
	}

	// Follow a chain of pointer offsets from a base address
	static uintptr_t FollowPointerChain(uintptr_t base, std::initializer_list<ptrdiff_t> offsets)
	{
		uintptr_t current = base;
		for (auto offset : offsets)
		{
			if (!Read(current + offset, current) || !current)
				return 0;
		}
		return current;
	}

public: /* Globals */
	static uintptr_t GetGlobalAddress(DWORD Index);
	static DWORD GetGlobalInt(DWORD Index);
	static float GetGlobalFloat(DWORD Index);
	static bool SetGlobalByte(DWORD Index, BYTE NewValue);
	static bool SetGlobalInt(DWORD Index, DWORD NewValue);
	static bool SetGlobalLongInt(DWORD Index, uintptr_t NewValue);
	static bool SetGlobalFloat(DWORD Index, float NewValue);
	template <typename T>
	static bool GetGlobalValue(DWORD Index, T& Out)
	{
		uintptr_t GlobalAddress = GetGlobalAddress(Index);
		if (!GlobalAddress) return 0;

		T ReturnValue = 0;

		DWORD BytesRead = 0x0;

		VMMDLL_MemReadEx(vmh, PID, GlobalAddress, (BYTE*)&ReturnValue, sizeof(T), &BytesRead, VMMDLL_FLAG_NOCACHE);

		if (BytesRead != sizeof(T))
			return 0;

		Out = ReturnValue;

		return 1;
	}

	template <typename T>
	static bool SetGlobalValue(DWORD Index, T In)
	{
		uintptr_t GlobalAddress = GetGlobalAddress(Index);
		if (!GlobalAddress) return 0;

		VMMDLL_MemWrite(vmh, PID, GlobalAddress, (BYTE*)&In, sizeof(T));

		return 1;
	}

private: /* Private functions */
	static bool UpdateEssentials();
	static bool Close();
};


