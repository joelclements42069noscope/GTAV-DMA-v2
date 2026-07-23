#pragma once

// Stat / Packed-stat debugging + offset-verification UI.
// Lets you resolve a packed index to its backing stat, read the current value,
// write test values, and dump raw sStatData -- the tool for verifying the packed
// range table on the live Enhanced build before trusting bulk writes.

class StatDebug
{
public:
	static void Render();
};
