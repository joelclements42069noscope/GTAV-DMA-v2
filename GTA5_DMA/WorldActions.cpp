#include "pch.h"
#include "WorldActions.h"
#include "offsets/Offsets.h"

// Camera model hashes (JOAAT) from YimMenu Object::IsCamera()
static const uint32_t CameraHashes[] = {
	DMAScript::Joaat("p_cctv_s"),
	DMAScript::Joaat("prop_cctv_cam_01a"),
	DMAScript::Joaat("prop_cctv_cam_01b"),
	DMAScript::Joaat("prop_cctv_cam_02a"),
	DMAScript::Joaat("prop_cctv_cam_03a"),
	DMAScript::Joaat("prop_cctv_cam_04a"),
	DMAScript::Joaat("prop_cctv_cam_04b"),
	DMAScript::Joaat("prop_cctv_cam_04c"),
	DMAScript::Joaat("prop_cctv_cam_05a"),
	DMAScript::Joaat("prop_cctv_cam_06a"),
	DMAScript::Joaat("prop_cctv_cam_07a"),
	DMAScript::Joaat("prop_cctv_pole_01a"),
	DMAScript::Joaat("prop_cctv_pole_02"),
	DMAScript::Joaat("prop_cctv_pole_03"),
	DMAScript::Joaat("prop_cctv_pole_04"),
	DMAScript::Joaat("prop_cs_cctv"),
	DMAScript::Joaat("hei_prop_bank_cctv_01"),
	DMAScript::Joaat("hei_prop_bank_cctv_02"),
	DMAScript::Joaat("ch_prop_ch_cctv_cam_02a"),
	DMAScript::Joaat("xm_prop_x17_server_farm_cctv_01"),
};

bool WorldActions::IsCameraHash(uint32_t hash)
{
	for (auto h : CameraHashes)
		if (h == hash)
			return true;
	return false;
}

bool WorldActions::IsEnemyPed(uintptr_t pedAddr)
{
	// DMA heuristic: check if ped has a weapon equipped (not unarmed)
	// Read CPedWeaponManager -> CWeaponInfo -> m_name hash
	uintptr_t weaponMgr = 0;
	if (!DMA::Read(pedAddr + offsetof(PED, pCPedWeaponManager), weaponMgr) || !weaponMgr)
		return false;

	uintptr_t weaponInfo = 0;
	if (!DMA::Read(weaponMgr + offsetof(CPEdWeaponManager, pCWeaponInfo), weaponInfo) || !weaponInfo)
		return false;

	uint32_t weaponHash = 0;
	if (!DMA::Read(weaponInfo + offsetof(WeaponInfo, m_name), weaponHash))
		return false;

	// WEAPON_UNARMED hash
	constexpr uint32_t WEAPON_UNARMED = 0xA2719263;
	return weaponHash != WEAPON_UNARMED && weaponHash != 0;
}

void WorldActions::KillAllPeds()
{
	if (!Offsets::PedPool)
	{
		LastResult = "PedPool offset not resolved";
		std::println("[WorldActions] {}", LastResult);
		return;
	}

	uintptr_t poolAddr = DMA::BaseAddress + Offsets::PedPool;
	auto peds = PoolIterator::GetEntitiesFromEncryptedPool(poolAddr, 2);

	if (peds.empty())
	{
		LastResult = "No peds found in pool (pool empty or decryption failed)";
		std::println("[WorldActions] {}", LastResult);
		return;
	}

	int killed = 0;
	int skipped = 0;
	for (uintptr_t pedAddr : peds)
	{
		// Skip player peds
		if (PoolIterator::IsPlayerPed(pedAddr))
		{
			skipped++;
			continue;
		}

		// Set health to 0
		float zero = 0.0f;
		if (DMA::Write(pedAddr + offsetof(PED, CurrentHealth), zero))
			killed++;
	}

	LastResult = std::format("Killed {} peds ({} players skipped, {} total in pool)", killed, skipped, peds.size());
	std::println("[WorldActions] {}", LastResult);
}

void WorldActions::KillAllEnemies()
{
	if (!Offsets::PedPool)
	{
		LastResult = "PedPool offset not resolved";
		std::println("[WorldActions] {}", LastResult);
		return;
	}

	uintptr_t poolAddr = DMA::BaseAddress + Offsets::PedPool;
	auto peds = PoolIterator::GetEntitiesFromEncryptedPool(poolAddr, 2);

	if (peds.empty())
	{
		LastResult = "No peds found in pool";
		std::println("[WorldActions] {}", LastResult);
		return;
	}

	int killed = 0;
	int skipped = 0;
	for (uintptr_t pedAddr : peds)
	{
		// Skip player peds
		if (PoolIterator::IsPlayerPed(pedAddr))
		{
			skipped++;
			continue;
		}

		// Only kill armed NPCs (enemy heuristic)
		if (!IsEnemyPed(pedAddr))
			continue;

		float zero = 0.0f;
		if (DMA::Write(pedAddr + offsetof(PED, CurrentHealth), zero))
			killed++;
	}

	LastResult = std::format("Killed {} armed NPCs ({} players skipped, {} total in pool)", killed, skipped, peds.size());
	std::println("[WorldActions] {}", LastResult);
}

void WorldActions::DestroyCameras()
{
	if (!Offsets::ObjectPool)
	{
		LastResult = "ObjectPool offset not resolved";
		std::println("[WorldActions] {}", LastResult);
		return;
	}

	uintptr_t poolAddr = DMA::BaseAddress + Offsets::ObjectPool;
	auto objects = PoolIterator::GetEntitiesFromEncryptedPool(poolAddr, 3);

	if (objects.empty())
	{
		LastResult = "No objects found in pool (pool empty or decryption failed)";
		std::println("[WorldActions] {}", LastResult);
		return;
	}

	int destroyed = 0;
	int checked = 0;
	for (uintptr_t objAddr : objects)
	{
		uint32_t modelHash = PoolIterator::GetEntityModelHash(objAddr);
		if (!modelHash)
			continue;

		checked++;
		if (IsCameraHash(modelHash))
		{
			// Set health to 0 to destroy the camera object
			float zero = 0.0f;
			DMA::Write(objAddr + 0x0280, zero);
			destroyed++;
		}
	}

	LastResult = std::format("Destroyed {} cameras ({} objects checked, {} total in pool)", destroyed, checked, objects.size());
	std::println("[WorldActions] {}", LastResult);
}

bool WorldActions::OnDMAFrame()
{
	if (!bLoopKillPeds && !bLoopKillEnemies && !bLoopDestroyCameras)
		return true;

	// Throttle: pool iteration is heavy (hundreds of DMA reads).
	// Run at most every 500ms to avoid starving other features (noclip etc).
	ULONGLONG now = GetTickCount64();
	if (now - LastLoopTickMs < 500)
		return true;
	LastLoopTickMs = now;

	if (bLoopKillPeds)
		KillAllPeds();
	if (bLoopKillEnemies)
		KillAllEnemies();
	if (bLoopDestroyCameras)
		DestroyCameras();
	return true;
}
