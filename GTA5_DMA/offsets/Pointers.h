#pragma once

// Forward declarations
class DMAModule;

namespace Pointers
{
	// Initialize pattern scanner: reads module, scans patterns, populates Offsets
	bool Init(VMM_HANDLE vmh, DWORD pid);
}
