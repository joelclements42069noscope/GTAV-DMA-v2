	{ PackedOp::BoolRange, 54773, 54786, 1 },
	{ PackedOp::Bool, 54653, 54653, 1 },
	{ PackedOp::BoolRange, 28313, 28318, 1 },
	{ PackedOp::Int, 24928, 24928, 10 },
	{ PackedOp::BoolRange, 51278, 51280, 1 },
	{ PackedOp::BoolRange, 51285, 51297, 1 },
	{ PackedOp::BoolRange, 28272, 28287, 1 },
	{ PackedOp::BoolRange, 41507, 42022, 1 },
	{ PackedOp::Bool, 9539, 9539, 1 },
	{ PackedOp::Int, 24903, 24903, 25 },
	{ PackedOp::Int, 24904, 24904, 2 },
	{ PackedOp::Bool, 9549, 9549, 1 },
	{ PackedOp::BoolRange, 9542, 9546, 1 },
	{ PackedOp::Bool, 9537, 9537, 1 },
	{ PackedOp::Bool, 7639, 7639, 1 },
	{ PackedOp::Int, 7669, 7669, 5 },
	{ PackedOp::Int, 7672, 7672, 2 },
	{ PackedOp::Int, 7670, 7670, 10 },
	{ PackedOp::Int, 7674, 7674, 3 },
	{ PackedOp::Int, 7671, 7671, 25 },
	{ PackedOp::BoolRange, 42037, 42038, 1 },
	{ PackedOp::BoolRange, 42041, 42042, 1 },
	{ PackedOp::BoolRange, 42044, 42051, 1 },
	{ PackedOp::Int, 41241, 41241, 5 },
	{ PackedOp::BoolRange, 41660, 41670, 1 },
	{ PackedOp::Bool, 28257, 28257, 1 },
	{ PackedOp::Bool, 22067, 22067, 1 },
	{ PackedOp::Bool, 15533, 15533, 1 },
	{ PackedOp::BoolRange, 22082, 22083, 1 },
	{ PackedOp::Bool, 36944, 36944, 1 },
	{ PackedOp::Bool, 41989, 41989, 1 },
	{ PackedOp::Bool, 15966, 15966, 1 },
	{ PackedOp::Bool, 41676, 41676, 1 },
	{ PackedOp::Bool, 41987, 41987, 1 },
	{ PackedOp::BoolRange, 36924, 36938, 1 },
	{ PackedOp::Int, 9359, 9359, 25 },
	{ PackedOp::BoolRange, 36870, 36917, 1 },
	{ PackedOp::BoolRange, 42002, 42013, 1 },
	{ PackedOp::BoolRange, 36831, 36865, 1 },
	{ PackedOp::BoolRange, 42029, 42036, 1 },
	{ PackedOp::BoolRange, 41874, 41883, 1 },
	{ PackedOp::Bool, 41988, 41988, 1 },
	{ PackedOp::BoolRange, 7553, 7592, 1 },
	{ PackedOp::Bool, 31737, 31737, 1 },
	{ PackedOp::Bool, 31753, 31753, 1 },
	{ PackedOp::BoolRange, 32397, 32399, 1 },
	{ PackedOp::BoolRange, 27089, 27090, 1 },
	{ PackedOp::Bool, 30309, 30309, 1 },
	{ PackedOp::Bool, 30522, 30522, 1 },
	{ PackedOp::Bool, 28270, 28270, 1 },
	{ PackedOp::Bool, 36842, 36842, 1 },
	{ PackedOp::Bool, 42025, 42025, 1 },
	{ PackedOp::Bool, 18139, 18139, 1 },
	{ PackedOp::Bool, 36861, 36861, 1 },
	{ PackedOp::BoolRange, 41712, 41714, 1 },
	{ PackedOp::BoolRange, 36867, 36868, 1 },
	{ PackedOp::BoolRange, 41715, 41719, 1 },
	{ PackedOp::Int, 42100, 42100, 5 },
	{ PackedOp::Bool, 25009, 25009, 1 },
	{ PackedOp::Int, 22063, 22063, 20 },
	{ PackedOp::Int, 42000, 42000, 20 },
	{ PackedOp::Bool, 51339, 51339, 1 },
	{ PackedOp::Bool, 36840, 36840, 1 },
	{ PackedOp::BoolRange, 41331, 41506, 1 },
	{ PackedOp::Bool, 42023, 42023, 1 },
	{ PackedOp::Int, 41246, 41246, 20 },
	{ PackedOp::Bool, 36922, 36922, 1 },
	{ PackedOp::Int, 42092, 42092, 5 },
	{ PackedOp::BoolRange, 36920, 36922, 1 },
	{ PackedOp::Int, 41242, 41242, 255 },
	{ PackedOp::BoolRange, 36941, 36942, 1 },
	{ PackedOp::BoolRange, 15456, 15460, 1 },

	// --- The Kortz Center Heist (packed career-progress checkmarks) ---
	// NOTE: these indices (60000+) are the 2026 DLC packed family, which is NOT
	// yet in the PackedStats range table (it ends at 59907). Until a verified
	// DLC12026 range is added, PackedStats::Resolve() returns invalid for these
	// and the ops are safely SKIPPED (no write, no corruption). The named
	// MPX_AWD_* awards in CareerProgress_NamedStats.inc already deliver the
	// Kortz unlocks; these are the progress-hub checkmarks. See PackedStats.cpp.
	{ PackedOp::Bool,      60011, 60011, 1 },  // Meet Raf De Angelis
	{ PackedOp::Bool,      60021, 60021, 1 },  // Scope out the Kortz Center
	{ PackedOp::Bool,      60022, 60022, 1 },  // Scope all secondary targets
	{ PackedOp::Bool,      60023, 60023, 1 },  // Complete on Hard, unspotted
	{ PackedOp::BoolRange, 60024, 60027, 1 },  // Use all 4 entry points
	{ PackedOp::Bool,      60028, 60028, 1 },  // Complete the Elite Challenge
	{ PackedOp::Int,       60049, 60049, 5 },  // Keep any 5 stolen paintings
	{ PackedOp::Int,       60050, 60050, 15 }, // Unlock 15 Platinum Awards
	{ PackedOp::Bool,      60105, 60105, 1 },  // Mansion trophy: World-class Collection
