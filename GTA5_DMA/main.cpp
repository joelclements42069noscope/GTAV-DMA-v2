#include "pch.h"
#include <print>
#include <thread>

#include "MyImGui.h"

bool bAlive = true;

int main(int, char**)
{
	MyImGui::Initialize();

	if (!DMA::Initialize())
	{
		MyImGui::Close();
		system("pause");
		return 0;
	}

	std::thread DMAThread(DMA::DMAThreadEntry);

	// Keep the display and system awake while the tool is open (no sleep / screensaver).
	// ES_CONTINUOUS makes this persist until we clear it on exit.
	SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);

	while (bAlive)
	{
		if (GetAsyncKeyState(VK_END) & 1)
			bAlive = false;

		MyImGui::OnFrame();
	}

	// Restore normal power/idle behavior.
	SetThreadExecutionState(ES_CONTINUOUS);

	DMAThread.join();

	MyImGui::Close();

	return 0;
}
