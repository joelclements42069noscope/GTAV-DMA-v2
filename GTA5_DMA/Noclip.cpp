#include "pch.h"
#include "Noclip.h"

bool Noclip::OnDMAFrame()
{
	if (!bEnable)
	{
		// Restore GodFlags when noclip is disabled
		if (GodFlagsSaved && DMA::LocalPlayerAddress)
		{
			DMA::Write(DMA::LocalPlayerAddress + offsetof(PED, GodFlags), SavedGodFlags);
			GodFlagsSaved = false;
			std::println("[Noclip] Restored GodFlags to {:X}", SavedGodFlags);
		}
		LastTickMs = 0;
		FrameCount = 0;
		PositionInitialized = false;
		return true;
	}

	if (!DMA::LocalPlayerAddress)
		return true;

	// Save and set collision-proof GodFlags on first frame
	if (!GodFlagsSaved)
	{
		DMA::Read(DMA::LocalPlayerAddress + offsetof(PED, GodFlags), SavedGodFlags);
		GodFlagsSaved = true;
		std::println("[Noclip] Saved GodFlags: {:X}", SavedGodFlags);
	}

	// Check vehicle state (needed for heading read)
	BYTE inVehicleBits = 0;
	DMA::Read(DMA::LocalPlayerAddress + offsetof(PED, InVehicleBits), inVehicleBits);
	bool inVehicle = (inVehicleBits & 0x1) && DMA::VehicleAddress;

	// Snapshot position ONCE when noclip first enables
	if (!PositionInitialized)
	{
		uintptr_t entityPosAddr = DMA::LocalPlayerAddress + ENTITY_POS_OFFSET;
		DWORD bytesRead = 0;
		VMMDLL_MemReadEx(DMA::vmh, DMA::PID, entityPosAddr, (BYTE*)&NoclipPosition, sizeof(Vec3), &bytesRead, VMMDLL_FLAG_NOCACHE);
		if (bytesRead != sizeof(Vec3))
			return true;

		PositionInitialized = true;
		LastTickMs = GetTickCount64();
		std::println("[Noclip] Initialized at ({:.1f}, {:.1f}, {:.1f})", NoclipPosition.x, NoclipPosition.y, NoclipPosition.z);
		return true;
	}

	// Delta time
	ULONGLONG now = GetTickCount64();
	float deltaTime = (float)(now - LastTickMs) / 1000.0f;
	LastTickMs = now;

	if (deltaTime <= 0.0f || deltaTime > 0.5f)
		deltaTime = 0.016f;

	// Read entity heading (forward + right vectors from transform matrix)
	// Use vehicle heading when in vehicle, player heading when on foot
	uintptr_t headingEntity = inVehicle ? DMA::VehicleAddress : DMA::LocalPlayerAddress;
	float matrixData[8] = {};
	DWORD matRead = 0;
	VMMDLL_MemReadEx(DMA::vmh, DMA::PID, headingEntity + ENTITY_RIGHT_OFFSET,
		(BYTE*)matrixData, sizeof(matrixData), &matRead, VMMDLL_FLAG_NOCACHE);

	Vec3 readRight, readFwd;
	readRight.x = matrixData[0]; readRight.y = matrixData[1]; readRight.z = matrixData[2];
	readFwd.x = matrixData[4]; readFwd.y = matrixData[5]; readFwd.z = matrixData[6];

	// Flatten to horizontal plane
	readFwd.z = 0;
	readRight.z = 0;
	float fwdLen = sqrtf(readFwd.x * readFwd.x + readFwd.y * readFwd.y);
	float rightLen = sqrtf(readRight.x * readRight.x + readRight.y * readRight.y);

	// Only update cached heading if we got valid vectors (prevents ragdoll from corrupting heading)
	if (fwdLen > 0.1f)
	{
		CachedForward.x = readFwd.x / fwdLen;
		CachedForward.y = readFwd.y / fwdLen;
		CachedForward.z = 0;
	}
	if (rightLen > 0.1f)
	{
		CachedRight.x = readRight.x / rightLen;
		CachedRight.y = readRight.y / rightLen;
		CachedRight.z = 0;
	}

	// Accumulate movement relative to cached heading
	Vec3 moveDir;
	moveDir.x = 0; moveDir.y = 0; moveDir.z = 0;

	if (MoveForward) { moveDir.x += CachedForward.x; moveDir.y += CachedForward.y; }
	if (MoveBack)    { moveDir.x -= CachedForward.x; moveDir.y -= CachedForward.y; }
	if (MoveRight)   { moveDir.x += CachedRight.x;   moveDir.y += CachedRight.y; }
	if (MoveLeft)    { moveDir.x -= CachedRight.x;   moveDir.y -= CachedRight.y; }
	if (MoveUp)      moveDir.z += 1.0f;
	if (MoveDown)    moveDir.z -= 1.0f;

	// Normalize and apply
	float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y + moveDir.z * moveDir.z);
	if (len > 0.0f)
	{
		moveDir.x /= len;
		moveDir.y /= len;
		moveDir.z /= len;

		float moveAmount = Speed * 20.0f * deltaTime;
		NoclipPosition.x += moveDir.x * moveAmount;
		NoclipPosition.y += moveDir.y * moveAmount;
		NoclipPosition.z += moveDir.z * moveAmount;
	}

	// Periodic diagnostics
	FrameCount++;
	if (FrameCount % 300 == 1)
	{
		std::println("[Noclip] frame={} pos=({:.1f},{:.1f},{:.1f}) heading=({:.2f},{:.2f}) spd={:.1f}",
			FrameCount, NoclipPosition.x, NoclipPosition.y, NoclipPosition.z,
			CachedForward.x, CachedForward.y, Speed);
	}

	// Build scatter write batch
	VMMDLL_SCATTER_HANDLE vmsh = VMMDLL_Scatter_Initialize(DMA::vmh, DMA::PID, VMMDLL_FLAG_NOCACHE);
	if (!vmsh)
		return true;

	// 1. Entity position (entity+0x90)
	uintptr_t entityPosAddr = DMA::LocalPlayerAddress + ENTITY_POS_OFFSET;
	VMMDLL_Scatter_PrepareWrite(vmsh, entityPosAddr, (BYTE*)&NoclipPosition, sizeof(Vec3));

	// 2. CNavigation position (nav+0x50)
	if (DMA::NavigationAddress)
	{
		uintptr_t navPosAddr = DMA::NavigationAddress + offsetof(CNavigation, Position);
		VMMDLL_Scatter_PrepareWrite(vmsh, navPosAddr, (BYTE*)&NoclipPosition, sizeof(Vec3));
	}

	// 3. Set collision-proof in GodFlags to reduce physics fighting
	uint32_t godFlags = SavedGodFlags | GODFLAGS_COLLISION_PROOF;
	VMMDLL_Scatter_PrepareWrite(vmsh, DMA::LocalPlayerAddress + offsetof(PED, GodFlags),
		(BYTE*)&godFlags, sizeof(godFlags));

	// 4. If in vehicle, also move the vehicle
	if (inVehicle)
	{
		uintptr_t vehEntityPosAddr = DMA::VehicleAddress + ENTITY_POS_OFFSET;
		VMMDLL_Scatter_PrepareWrite(vmsh, vehEntityPosAddr, (BYTE*)&NoclipPosition, sizeof(Vec3));

		if (DMA::VehicleNavigationAddress)
		{
			uintptr_t vehNavPosAddr = DMA::VehicleNavigationAddress + offsetof(CNavigation, Position);
			VMMDLL_Scatter_PrepareWrite(vmsh, vehNavPosAddr, (BYTE*)&NoclipPosition, sizeof(Vec3));
		}

		// Also set collision-proof on vehicle
		VMMDLL_Scatter_PrepareWrite(vmsh, DMA::VehicleAddress + offsetof(CVehicle, GodBits),
			(BYTE*)&godFlags, sizeof(godFlags));
	}

	VMMDLL_Scatter_Execute(vmsh);
	VMMDLL_Scatter_CloseHandle(vmsh);

	return true;
}

void Noclip::Render()
{
	ImGui::Checkbox("Noclip", &bEnable);
	if (!bEnable)
	{
		MoveForward = MoveBack = MoveLeft = MoveRight = MoveUp = MoveDown = false;
		return;
	}

	ImGui::SliderFloat("Noclip Speed", &Speed, 0.1f, 10.0f, "%.1f");

	// Capture keyboard state
	bool kW = ImGui::IsKeyDown(ImGuiKey_W);
	bool kS = ImGui::IsKeyDown(ImGuiKey_S);
	bool kA = ImGui::IsKeyDown(ImGuiKey_A);
	bool kD = ImGui::IsKeyDown(ImGuiKey_D);
	bool kSpace = ImGui::IsKeyDown(ImGuiKey_Space);
	bool kCtrl = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);

	// Movement buttons (hold to move)
	ImGui::Text("WASD = Fwd/Back/Left/Right, Space/Ctrl = Up/Down");
	ImGui::Dummy(ImVec2(56, 0)); ImGui::SameLine();
	ImGui::Button("Fwd##nc", ImVec2(56, 28)); bool bW = ImGui::IsItemActive();

	ImGui::Button("Left##nc", ImVec2(56, 28)); bool bA = ImGui::IsItemActive();
	ImGui::SameLine();
	ImGui::Button("Back##nc", ImVec2(56, 28)); bool bS = ImGui::IsItemActive();
	ImGui::SameLine();
	ImGui::Button("Right##nc", ImVec2(56, 28)); bool bD = ImGui::IsItemActive();

	ImGui::Button("Up##nc", ImVec2(72, 24)); bool bUp = ImGui::IsItemActive();
	ImGui::SameLine();
	ImGui::Button("Down##nc", ImVec2(72, 24)); bool bDown = ImGui::IsItemActive();

	MoveForward = kW || bW;
	MoveBack = kS || bS;
	MoveLeft = kA || bA;
	MoveRight = kD || bD;
	MoveUp = kSpace || bUp;
	MoveDown = kCtrl || bDown;

	bool anyInput = MoveForward || MoveBack || MoveLeft || MoveRight || MoveUp || MoveDown;
	if (anyInput)
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Moving...");
	else
		ImGui::TextDisabled("Press WASD or hold buttons to move");
}
