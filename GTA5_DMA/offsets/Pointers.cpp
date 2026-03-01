#include "pch.h"
#include "offsets/Pointers.h"
#include "offsets/Offsets.h"
#include "core/DMAModule.h"
#include "core/DMAPatternScanner.h"
#include "core/DMAPatternCache.h"

using namespace DMAOffsets;

// Static module instance that lives for the duration of the program
static DMAModule g_GameModule;

bool Pointers::Init(VMM_HANDLE vmh, DWORD pid)
{
	std::println("[Pointers] Starting pattern scan...");

	// Initialize cache
	DMAPatternCache::Init("pattern_cache.bin");

	// Load the game module into local buffer
	if (!g_GameModule.Load(vmh, pid, "GTA5_Enhanced.exe"))
	{
		std::println("[Pointers] Failed to load game module, using fallback offsets");
		Offsets::ApplyFallbacks();
		Offsets::Initialized = true;
		return false;
	}

	auto scanner = DMAPatternScanner(&g_GameModule);

	// -------------------------------------------------------------------
	// PedFactory (equivalent to WorldPtr)
	// YimMenu: ptr.Add(7).Add(3).Rip().As<CPedFactory**>()
	// We store the offset from base, which when dereferenced gives CPedFactory*
	// -------------------------------------------------------------------
	constexpr auto pedFactoryPtrn = Pattern<"C7 40 30 03 00 00 00 48 8B 0D">("PedFactory");
	scanner.Add(pedFactoryPtrn, [](ResolvedPtr ptr) {
		// Skip 7 bytes (C7 40 30 03 00 00 00), then at "48 8B 0D XX XX XX XX"
		// the RIP-relative operand is at +3 from the 48 8B 0D instruction
		auto resolved = ptr.Add(7).Add(3).Rip();
		Offsets::PedFactory = resolved.AsOffset();
		std::println("[Pointers] PedFactory offset: {:X}", Offsets::PedFactory);
	});

	// -------------------------------------------------------------------
	// ScriptGlobals (equivalent to GlobalPtr)
	// YimMenu: ptr.Add(7).Add(3).Rip().As<int64_t**>()
	// -------------------------------------------------------------------
	constexpr auto scriptGlobalsPtrn = Pattern<"48 8B 8E B8 00 00 00 48 8D 15 ? ? ? ? 49 89 D8">("ScriptGlobals");
	scanner.Add(scriptGlobalsPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(7).Add(3).Rip();
		Offsets::ScriptGlobals = resolved.AsOffset();
		std::println("[Pointers] ScriptGlobals offset: {:X}", Offsets::ScriptGlobals);
	});

	// -------------------------------------------------------------------
	// ScriptThreads (needed for ScriptLocal access -- slot rigger, heist skip, etc.)
	// YimMenu: ptr.Add(3).Rip().As<rage::atArray<rage::scrThread*>*>()
	// -------------------------------------------------------------------
	constexpr auto scriptThreadsPtrn = Pattern<"48 8B 05 ? ? ? ? 48 89 34 F8 48 FF C7 48 39 FB 75 97">("ScriptThreads");
	scanner.Add(scriptThreadsPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(3).Rip();
		Offsets::ScriptThreads = resolved.AsOffset();
		std::println("[Pointers] ScriptThreads offset: {:X}", Offsets::ScriptThreads);
	});

	// -------------------------------------------------------------------
	// ScriptPrograms
	// YimMenu: ptr.Add(0x13).Add(3).Rip().Add(0xD8).As<rage::scrProgram**>()
	// -------------------------------------------------------------------
	constexpr auto scriptProgramsPtrn = Pattern<"48 C7 84 C8 D8 00 00 00 00 00 00 00">("ScriptPrograms");
	scanner.Add(scriptProgramsPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(0x13).Add(3).Rip();
		Offsets::ScriptPrograms = resolved.AsOffset();
		std::println("[Pointers] ScriptPrograms offset: {:X}", Offsets::ScriptPrograms);
	});

	// -------------------------------------------------------------------
	// IsSessionStarted
	// YimMenu: addr.Add(3).Rip().As<bool*>()
	// -------------------------------------------------------------------
	constexpr auto isSessionStartedPtrn = Pattern<"0F B6 05 ? ? ? ? 0A 05 ? ? ? ? 75 2A">("IsSessionStarted");
	scanner.Add(isSessionStartedPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(3).Rip();
		Offsets::IsSessionStarted = resolved.AsOffset();
		std::println("[Pointers] IsSessionStarted offset: {:X}", Offsets::IsSessionStarted);
	});

	// -------------------------------------------------------------------
	// NetworkPlayerMgr
	// YimMenu: ptr.Add(2).Add(3).Rip().As<CNetworkPlayerMgr**>()
	// -------------------------------------------------------------------
	constexpr auto networkPlayerMgrPtrn = Pattern<"75 0E 48 8B 05 ? ? ? ? 48 8B 88 F0 00 00 00">("NetworkPlayerMgr");
	scanner.Add(networkPlayerMgrPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(2).Add(3).Rip();
		Offsets::NetworkPlayerMgr = resolved.AsOffset();
		std::println("[Pointers] NetworkPlayerMgr offset: {:X}", Offsets::NetworkPlayerMgr);
	});

	// -------------------------------------------------------------------
	// NetworkObjectMgr
	// YimMenu: ptr.Add(0xC).Add(3).Rip().As<CNetworkObjectMgr**>()
	// -------------------------------------------------------------------
	constexpr auto networkObjectMgrPtrn = Pattern<"41 83 7E FA 02 40 0F 9C C5 C1 E5 02">("NetworkObjectMgr");
	scanner.Add(networkObjectMgrPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(0xC).Add(3).Rip();
		Offsets::NetworkObjectMgr = resolved.AsOffset();
		std::println("[Pointers] NetworkObjectMgr offset: {:X}", Offsets::NetworkObjectMgr);
	});

	// -------------------------------------------------------------------
	// PedPool
	// YimMenu: ptr.Add(0x18).Add(3).Rip().As<PoolEncryption*>()
	// -------------------------------------------------------------------
	constexpr auto pedPoolPtrn = Pattern<"80 79 4B 00 0F 84 F5 00 00 00 48 89 F1">("PedPool");
	scanner.Add(pedPoolPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(0x18).Add(3).Rip();
		Offsets::PedPool = resolved.AsOffset();
		std::println("[Pointers] PedPool offset: {:X}", Offsets::PedPool);
	});

	// -------------------------------------------------------------------
	// VehiclePool
	// YimMenu: ptr.Sub(0xA).Add(3).Rip().As<rage::fwVehiclePool***>()
	// -------------------------------------------------------------------
	constexpr auto vehiclePoolPtrn = Pattern<"48 83 78 18 0D">("VehiclePool");
	scanner.Add(vehiclePoolPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Sub(0xA).Add(3).Rip();
		Offsets::VehiclePool = resolved.AsOffset();
		std::println("[Pointers] VehiclePool offset: {:X}", Offsets::VehiclePool);
	});

	// -------------------------------------------------------------------
	// ObjectPool
	// YimMenu: ptr.Add(5).Add(3).Rip().As<PoolEncryption*>()
	// -------------------------------------------------------------------
	constexpr auto objectPoolPtrn = Pattern<"48 8B 04 0A C3 0F B6 05">("ObjectPool");
	scanner.Add(objectPoolPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(5).Add(3).Rip();
		Offsets::ObjectPool = resolved.AsOffset();
		std::println("[Pointers] ObjectPool offset: {:X}", Offsets::ObjectPool);
	});

	// -------------------------------------------------------------------
	// NetworkTime
	// YimMenu: ptr.Add(2).Rip().As<uint32_t*>()
	// -------------------------------------------------------------------
	constexpr auto networkTimePtrn = Pattern<"89 05 ? ? ? ? 80 3D ? ? ? ? ? 0F 84 ? ? ? ? E9">("NetworkTime");
	scanner.Add(networkTimePtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(2).Rip();
		Offsets::NetworkTime = resolved.AsOffset();
		std::println("[Pointers] NetworkTime offset: {:X}", Offsets::NetworkTime);
	});

	// -------------------------------------------------------------------
	// GameTimer
	// YimMenu: ptr.Add(2).Rip().As<uint32_t*>()
	// -------------------------------------------------------------------
	constexpr auto gameTimerPtrn = Pattern<"3B 2D ? ? ? ? 76 ? 89 D9">("GameTimer");
	scanner.Add(gameTimerPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(2).Rip();
		Offsets::GameTimer = resolved.AsOffset();
		std::println("[Pointers] GameTimer offset: {:X}", Offsets::GameTimer);
	});

	// -------------------------------------------------------------------
	// GameVersion
	// YimMenu: ptr.Add(3).Rip().As<const char*>()
	// -------------------------------------------------------------------
	constexpr auto versionPtrn = Pattern<"4C 8D 0D ? ? ? ? 48 8D 5C 24 ? 48 89 D9 48 89 FA">("GameVersion");
	scanner.Add(versionPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(3).Rip();
		Offsets::GameVersion = resolved.AsOffset();
		std::println("[Pointers] GameVersion offset: {:X}", Offsets::GameVersion);
	});

	// -------------------------------------------------------------------
	// StatsMgr
	// YimMenu: ptr.Add(4).Add(3).Rip().As<CStatsMgr*>()
	// -------------------------------------------------------------------
	constexpr auto statsMgrPtrn = Pattern<"89 6C 24 28 48 8D 0D ? ? ? ? 48 8D">("CStatsMgr");
	scanner.Add(statsMgrPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(4).Add(3).Rip();
		Offsets::StatsMgr = resolved.AsOffset();
		std::println("[Pointers] StatsMgr offset: {:X}", Offsets::StatsMgr);
	});

	// -------------------------------------------------------------------
	// HasGTAPlus (SC membership)
	// YimMenu: addr.Add(3).Rip().As<int*>()
	// -------------------------------------------------------------------
	constexpr auto hasGTAPlusPtrn = Pattern<"48 8D 15 ? ? ? ? 41 B8 18 02 00 00 E8">("HasGTAPlus");
	scanner.Add(hasGTAPlusPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(3).Rip();
		Offsets::HasGTAPlus = resolved.AsOffset();
		std::println("[Pointers] HasGTAPlus offset: {:X}", Offsets::HasGTAPlus);
	});

	// -------------------------------------------------------------------
	// BlipPtr (BlipArray* -- array of Blip pointers for waypoint teleport)
	// lea r15, [rip+disp32] ; mov rsi, [r15+rax*8]
	// Resolution: .Add(3).Rip() -> absolute address of BlipArray
	// -------------------------------------------------------------------
	constexpr auto blipPtrPtrn = Pattern<"4C 8D 3D ? ? ? ? 49 8B 34 C7">("BlipPtr");
	scanner.Add(blipPtrPtrn, [](ResolvedPtr ptr) {
		auto resolved = ptr.Add(3).Rip();
		Offsets::BlipPtr = resolved.AsOffset();
		std::println("[Pointers] BlipPtr offset: {:X}", Offsets::BlipPtr);
	});

	// Run all patterns
	bool success = scanner.Scan();

	// Save cache (even partial results are useful)
	DMAPatternCache::Save();

	// Apply fallbacks for any critical patterns that failed
	Offsets::ApplyFallbacks();

	Offsets::Initialized = true;

	if (success)
		std::println("[Pointers] All patterns found successfully!");
	else
		std::println("[Pointers] Some patterns failed, fallback offsets applied where available");

	return success;
}
