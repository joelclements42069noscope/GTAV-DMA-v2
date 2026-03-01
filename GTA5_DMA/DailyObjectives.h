#pragma once

// Daily Objectives / Weekly Activities via stat writes
// Completes daily/weekly activities that use regular stats (not packed stats).
//
// Supported (regular stat writes):
//   - Time Trials (standard, RC Bandito, Junk Energy Bike)
//   - Exotic Exports (all vehicles delivered)
//   - Skydive Collections (all 10 locations)
//
// NOT supported (require packed stats / natives):
//   - G's Cache, Stash House, Shipwreck, Hidden Caches, Treasure Chests
//   - Buried Stashes, LS Tags, Madrazo Hits, Animal Photography
//   - Smoke on the Water, Golden Clover, Junk Energy Skydives
//   - Street Dealers, Trick or Treat

class DailyObjectives
{
public:
	static void Render(); // ImGui UI

private:
	static void CompleteTimeTrials();
	static void CompleteExoticExports();
	static void CompleteSkydives();

	static inline const char* StatusText = "";
};
