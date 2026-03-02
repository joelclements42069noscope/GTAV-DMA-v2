#include "pch.h"

#include "offsets/Offsets.h"
#include "offsets/Pointers.h"

#include "Features.h"
#include "Heightmap.h"

extern bool bAlive;

bool DMA::Initialize()
{
	LPCSTR args[] = { "","-device","FPGA" };

	vmh = VMMDLL_Initialize(3, args);

	if (!vmh)
	{
		std::println("VMMDLL_Initialize failed.");
		return 0;
	}

	if (!VMMDLL_PidGetFromName(vmh, "GTA5_Enhanced.exe", &PID))
	{
		std::println("VMMDLL_PidGetFromName failed.");
		return 0;
	}

	if (!PID)
	{
		std::println("GTA5_Enhanced.exe PID is null.");
		return 0;
	}

	BaseAddress = VMMDLL_ProcessGetModuleBaseU(vmh, PID, "GTA5_Enhanced.exe");

	if (!BaseAddress)
	{
		std::println("GTA5_Enhanced.exe BaseAddress is null.");
		return 0;
	}

	std::println("GTA5_Enhanced.exe found @ {0:x}\n", BaseAddress);

	// Auto-resolve offsets via pattern scanning
	ScanPatterns();

	// Load tunable cache (for RP multiplier, no idle kick, etc.)
	TunableService::LoadFromCache("tunables.bin");

	// Load heightmap for waypoint teleport Z lookup
	Heightmap::Load("heightmap.bin");

	// Initialize stats writer (may fail if game not fully loaded; retries lazily on first use)
	StatsWriter::EnsureInitialized();

	return 1;
}

bool DMA::ScanPatterns()
{
	std::println("Starting auto offset resolution...\n");
	bool result = Pointers::Init(vmh, PID);

	if (result)
		std::println("Auto offset resolution complete!\n");
	else
		std::println("Auto offset resolution partially failed, fallbacks applied.\n");

	// Print diagnostic summary of all resolved offsets
	std::println("=== Pattern Scan Summary ===");
	std::println("  PedFactory:       {} ({:X})", Offsets::PedFactory ? "OK" : "MISSING", Offsets::PedFactory);
	std::println("  ScriptGlobals:    {} ({:X})", Offsets::ScriptGlobals ? "OK" : "MISSING", Offsets::ScriptGlobals);
	std::println("  ScriptThreads:    {} ({:X})", Offsets::ScriptThreads ? "OK" : "MISSING", Offsets::ScriptThreads);
	std::println("  ScriptPrograms:   {} ({:X})", Offsets::ScriptPrograms ? "OK" : "MISSING", Offsets::ScriptPrograms);
	std::println("  NetworkPlayerMgr: {} ({:X})", Offsets::NetworkPlayerMgr ? "OK" : "MISSING", Offsets::NetworkPlayerMgr);
	std::println("  NetworkObjectMgr: {} ({:X})", Offsets::NetworkObjectMgr ? "OK" : "MISSING", Offsets::NetworkObjectMgr);
	std::println("  IsSessionStarted: {} ({:X})", Offsets::IsSessionStarted ? "OK" : "MISSING", Offsets::IsSessionStarted);
	std::println("  NetworkTime:      {} ({:X})", Offsets::NetworkTime ? "OK" : "MISSING", Offsets::NetworkTime);
	std::println("  GameVersion:      {} ({:X})", Offsets::GameVersion ? "OK" : "MISSING", Offsets::GameVersion);
	std::println("  GameTimer:        {} ({:X})", Offsets::GameTimer ? "OK" : "MISSING", Offsets::GameTimer);
	std::println("  StatsMgr:         {} ({:X})", Offsets::StatsMgr ? "OK" : "MISSING", Offsets::StatsMgr);
	std::println("  HasGTAPlus:       {} ({:X})", Offsets::HasGTAPlus ? "OK" : "MISSING", Offsets::HasGTAPlus);
	std::println("  PedPool:          {} ({:X})", Offsets::PedPool ? "OK" : "MISSING", Offsets::PedPool);
	std::println("  VehiclePool:      {} ({:X})", Offsets::VehiclePool ? "OK" : "MISSING", Offsets::VehiclePool);
	std::println("  ObjectPool:       {} ({:X})", Offsets::ObjectPool ? "OK" : "MISSING", Offsets::ObjectPool);
	std::println("  BlipPtr:          {} ({:X})", Offsets::BlipPtr ? "OK" : "MISSING", Offsets::BlipPtr);
	std::println("============================");

	// Feature dependency warnings
	if (!Offsets::NetworkPlayerMgr)
		std::println("[WARNING] NetworkPlayerMgr failed -- OffTheRadar, Invisibility, PlayerList will NOT work");
	if (!Offsets::ScriptThreads)
		std::println("[WARNING] ScriptThreads failed -- CasinoSlotRigger will NOT work");
	if (!Offsets::ScriptPrograms)
		std::println("[WARNING] ScriptPrograms failed -- Script local features may NOT work");
	if (!Offsets::HasGTAPlus)
		std::println("[WARNING] HasGTAPlus failed -- GTA+ Unlock will NOT work");

	return result;
}

bool DMA::DMAThreadEntry()
{

	while (bAlive)
	{
		try
		{
			UpdateEssentials();
		}
		catch (std::runtime_error& e)
		{
			std::println("UpdateEssentials threw exception!\n   {}\n",e.what());
			continue;
		}
		catch (...)
		{
			std::println("Uncaught exception in UpdateEssentials()");
			continue;
		}

		RefreshHealth::OnDMAFrame();
		NoWanted::OnDMAFrame();
		WeaponInspector::OnDMAFrame();
		Teleport::OnDMAFrame();
		GodMode::OnDMAFrame();
		CarGodMode::OnDMAFrame();
		SwimSpeed::OnDMAFrame();
		Seatbelt::OnDMAFrame();
		RunSpeed::OnDMAFrame();
		HashChanger::OnDMAFrame();
		CarControl::OnDMAFrame();
		CarInspector::OnDMAFrame();
		CasinoSlotRigger::OnDMAFrame();
		HeistCuts::OnDMAFrame();
		OffTheRadar::OnDMAFrame();
		Invisibility::OnDMAFrame();
		PlayerList::OnDMAFrame();
		Noclip::OnDMAFrame();
		RPMultiplier::OnDMAFrame();
		NoIdleKick::OnDMAFrame();
		WorldActions::OnDMAFrame();
	}

	DMA::Close();

	return 1;
}

bool DMA::UpdatePlayerCurrentLocation()
{
	uintptr_t LocationAddress = NavigationAddress + offsetof(CNavigation, Position);

	DWORD BytesRead = 0;

	VMMDLL_MemReadEx(vmh, PID, LocationAddress, (BYTE*)&LocalPlayerLocation, sizeof(Vec3), &BytesRead, VMMDLL_FLAG_NOCACHE);

	if (BytesRead != sizeof(Vec3))
	{
		ZeroMemory(&LocalPlayerLocation, sizeof(Vec3));
		throw std::runtime_error("Incomplete LocalPlayer location read.");
	}

	return 1;
}

uintptr_t DMA::GetGlobalAddress(DWORD Index)
{
	if (!Offsets::ScriptGlobals)
	{
		static bool warned = false;
		if (!warned) { std::println("[Globals] ScriptGlobals offset is 0 (pattern scan failed, no fallback)"); warned = true; }
		return 0;
	}

	int ChunkIndex = Index >> 0x12 & 0x3F;
	int ElementIndex = Index & 0x3FFFF;

	DWORD BytesRead = 0x0;

	uintptr_t GlobalAddress = BaseAddress + Offsets::ScriptGlobals;

	uintptr_t ChunkPtr = GlobalAddress + (ChunkIndex * 0x8);

	uintptr_t ChunkAddress = 0x0;
	VMMDLL_MemReadEx(vmh, PID, ChunkPtr, (BYTE*)&ChunkAddress, sizeof(uintptr_t), &BytesRead, VMMDLL_FLAG_NOCACHE);

	if (BytesRead != sizeof(uintptr_t))
	{
		std::println("Incomplete ChunkPtr read.");
		return 0;
	}

	uintptr_t ElementAddress = ChunkAddress + (ElementIndex * 0x8);

	return ElementAddress;
}

DWORD DMA::GetGlobalInt(DWORD Index)
{
	DWORD OutValue = 0x0;
	DWORD BytesRead = 0x0;

	uintptr_t GlobalAddress = DMA::GetGlobalAddress(Index);

	if (!GlobalAddress)
		return 0;

	VMMDLL_MemReadEx(vmh, PID, GlobalAddress, (BYTE*)&OutValue, sizeof(DWORD), &BytesRead, VMMDLL_FLAG_NOCACHE);

	if (BytesRead != sizeof(DWORD))
	{
		std::println("Incomplete GlobalAddress read.");
		return 0;
	}

	return OutValue;
}

float DMA::GetGlobalFloat(DWORD Index)
{
	float OutValue = 0.0f;

	DWORD BytesRead = 0x0;

	uintptr_t FloatAddress = DMA::GetGlobalAddress(Index);

	if (!FloatAddress)
		return 0;

	VMMDLL_MemReadEx(vmh, PID, FloatAddress, (BYTE*)&OutValue, sizeof(float), &BytesRead, VMMDLL_FLAG_NOCACHE);

	if (BytesRead != sizeof(float))
		return 0.0f;

	return OutValue;
}

bool DMA::SetGlobalByte(DWORD Index, BYTE NewValue)
{
	uintptr_t WriteAddress = DMA::GetGlobalAddress(Index);

	if (!WriteAddress)
		return 0;

	VMMDLL_MemWrite(vmh, PID, WriteAddress, (BYTE*)&NewValue, sizeof(BYTE));

	return 1;
}

bool DMA::SetGlobalInt(DWORD Index, DWORD NewValue)
{
	uintptr_t WriteAddress = DMA::GetGlobalAddress(Index);

	if (!WriteAddress)
		return 0;

	VMMDLL_MemWrite(vmh, PID, WriteAddress, (BYTE*)&NewValue, sizeof(DWORD));

	return 1;
}

bool DMA::SetGlobalLongInt(DWORD Index, uintptr_t NewValue)
{
	uintptr_t WriteAddress = DMA::GetGlobalAddress(Index);

	if (!WriteAddress)
		return 0;

	VMMDLL_MemWrite(vmh, PID, WriteAddress, (BYTE*)&NewValue, sizeof(uintptr_t));

	return 1;
}

bool DMA::SetGlobalFloat(DWORD Index, float NewValue)
{
	uintptr_t FloatAddress = DMA::GetGlobalAddress(Index);

	if (!FloatAddress)
		return 0;

	VMMDLL_MemWrite(vmh, PID, FloatAddress, (BYTE*)&NewValue, sizeof(float));

	return 1;
}

bool DMA::UpdateEssentials()
{
	DWORD BytesRead = 0x0;

	// Use runtime-resolved PedFactory offset (or fallback WorldPtr)
	uintptr_t PedFactoryPtr = BaseAddress + Offsets::PedFactory;
	uintptr_t PedFactoryAddress = 0x0;
	VMMDLL_MemReadEx(vmh, PID, PedFactoryPtr, (BYTE*)&PedFactoryAddress, sizeof(uintptr_t), &BytesRead, VMMDLL_FLAG_NOCACHE);
	if (BytesRead != sizeof(uintptr_t)) [[unlikely]]
	{
		std::println("PedFactory Dereference failed! Reinitializing VMH.");
		Close();
		Initialize();
		return 0;
	}
	if (!PedFactoryAddress) [[unlikely]]
	{
		std::println("*PedFactory is null! Reinitializing VMH.");
		Close();
		Initialize();
		return 0;
	}

	// CPedFactory->m_LocalPed at offset 0x08 (same as World->pPlayer at 0x08)
	uintptr_t LocalPlayerPtr = PedFactoryAddress + 0x08;
	VMMDLL_MemReadEx(vmh, PID, LocalPlayerPtr, (BYTE*)&LocalPlayerAddress, sizeof(uintptr_t), &BytesRead, VMMDLL_FLAG_NOCACHE);
	if (BytesRead != sizeof(uintptr_t)) [[unlikely]]
		throw std::runtime_error("Incomplete LocalPlayerPtr read.");

	if (!LocalPlayerAddress) [[unlikely]]
		throw std::runtime_error("*LocalPlayerPtr is null.");

	uintptr_t CNavigationPtr = LocalPlayerAddress + offsetof(PED, pCNavigation);
	VMMDLL_MemReadEx(vmh, PID, CNavigationPtr, (BYTE*)&NavigationAddress, sizeof(uintptr_t), &BytesRead, VMMDLL_FLAG_NOCACHE);
	if (BytesRead != sizeof(uintptr_t)) [[unlikely]]
		throw std::runtime_error("Incomplete CNavigationPtr read.");

	if (!NavigationAddress) [[unlikely]]
		throw std::runtime_error("*CNavigationPtr is null.");

	uintptr_t PlayerInfoPtr = LocalPlayerAddress + offsetof(PED, pPlayerInfo);
	VMMDLL_MemReadEx(vmh, PID, PlayerInfoPtr, (BYTE*)&PlayerInfoAddress, sizeof(uintptr_t), &BytesRead, VMMDLL_FLAG_NOCACHE);
	if (BytesRead != sizeof(uintptr_t)) [[unlikely]]
		throw std::runtime_error("Incomplete PlayerInfoPtr read.");

	if (!PlayerInfoAddress) [[unlikely]]
		throw std::runtime_error("*PlayerInfoPtr is null.");

	uintptr_t WeaponInventoryPtr = LocalPlayerAddress + offsetof(PED, pCWeaponInventory);
	VMMDLL_MemReadEx(vmh, PID, WeaponInventoryPtr, (BYTE*)&WeaponInventoryAddress, sizeof(uintptr_t), &BytesRead, VMMDLL_FLAG_NOCACHE);
	if (BytesRead != sizeof(uintptr_t)) [[unlikely]]
		throw std::runtime_error("Incomplete WeaponInventoryPtr read.");

	if (!WeaponInventoryAddress) [[unlikely]]
		throw std::runtime_error("*WeaponInventoryPtr is null.");

	uintptr_t WeaponManagerPtr = LocalPlayerAddress + offsetof(PED, pCPedWeaponManager);
	VMMDLL_MemReadEx(vmh, PID, WeaponManagerPtr, (BYTE*)&WeaponManagerAddress, sizeof(uintptr_t), &BytesRead, VMMDLL_FLAG_NOCACHE);
	if (BytesRead != sizeof(uintptr_t)) [[unlikely]]
		throw std::runtime_error("Incomplete WeaponManagerPtr read.");

	if (!WeaponManagerAddress) [[unlikely]]
		throw std::runtime_error("*WeaponManagerPtr is null.");

	uintptr_t WeaponInfoPtr = WeaponManagerAddress + offsetof(CPEdWeaponManager, pCWeaponInfo);
	VMMDLL_MemReadEx(DMA::vmh, DMA::PID, WeaponInfoPtr, (BYTE*)&WeaponInfoAddress, sizeof(uintptr_t), &BytesRead, VMMDLL_FLAG_NOCACHE);
	if (BytesRead != sizeof(uintptr_t)) [[unlikely]]
		throw std::runtime_error("Incomplete WeaponInfoPtr read.");

	UpdateVehicleInformation();

	UpdatePlayerCurrentLocation();

	return 1;
}

bool DMA::Close()
{
	VMMDLL_Close(vmh);
	return 1;
}

bool DMA::UpdateVehicleInformation()
{
	DWORD BytesRead = 0x0;

	uintptr_t VehiclePtr = LocalPlayerAddress + offsetof(PED, pCVehicle);
	VMMDLL_MemReadEx(vmh, PID, VehiclePtr, (BYTE*)&VehicleAddress, sizeof(uintptr_t), &BytesRead, VMMDLL_FLAG_NOCACHE);
	if (BytesRead != sizeof(uintptr_t)) [[unlikely]]
	{
		VehicleAddress = 0;
		return 0;
	}

	if (!VehicleAddress) [[unlikely]]
		return 0;

	uintptr_t VehicleNavigationPtr = VehicleAddress + offsetof(CVehicle, pCNavigation);
	VMMDLL_MemReadEx(vmh, PID, VehicleNavigationPtr, (BYTE*)&VehicleNavigationAddress, sizeof(uintptr_t), &BytesRead, VMMDLL_FLAG_NOCACHE);
	if (BytesRead != sizeof(uintptr_t)) [[unlikely]]
	{
		VehicleNavigationAddress = 0;
		return 0;
	}

	if (!VehicleNavigationAddress) [[unlikely]]
		return 0;

	return 1;
}