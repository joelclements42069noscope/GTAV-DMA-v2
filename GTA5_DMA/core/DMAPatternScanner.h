#pragma once
#include "core/Pattern.h"
#include "core/DMAModule.h"

#include <functional>
#include <vector>

namespace DMAOffsets
{
	// Represents a resolved address from a pattern scan.
	// All operations work on the local buffer copy but track the remote address.
	class ResolvedPtr
	{
	public:
		ResolvedPtr(uintptr_t remoteAddr, const DMAModule* module)
			: m_RemoteAddr(remoteAddr), m_Module(module) {}

		// Add offset to current address
		ResolvedPtr Add(ptrdiff_t offset) const
		{
			return ResolvedPtr(m_RemoteAddr + offset, m_Module);
		}

		// Subtract offset from current address
		ResolvedPtr Sub(ptrdiff_t offset) const
		{
			return ResolvedPtr(m_RemoteAddr - offset, m_Module);
		}

		// Resolve RIP-relative address: reads int32 displacement from local buffer
		ResolvedPtr Rip() const
		{
			return ResolvedPtr(m_Module->ResolveRip(m_RemoteAddr), m_Module);
		}

		// Get as remote address (offset from module base, suitable for storing)
		uintptr_t AsRemote() const { return m_RemoteAddr; }

		// Get as offset from module base
		ptrdiff_t AsOffset() const { return static_cast<ptrdiff_t>(m_RemoteAddr - m_Module->Base()); }

	private:
		uintptr_t m_RemoteAddr;
		const DMAModule* m_Module;
	};

	using PatternCallback = std::function<void(ResolvedPtr)>;

	class DMAPatternScanner
	{
	public:
		DMAPatternScanner(const DMAModule* module);

		template<Signature S>
		void Add(const Pattern<S>& pattern, const PatternCallback& func);

		bool Scan();

	private:
		bool ScanInternal(const IPattern* pattern, PatternCallback func) const;

		const DMAModule* m_Module;
		std::vector<std::pair<const IPattern*, PatternCallback>> m_Patterns;
	};

	template<Signature S>
	inline void DMAPatternScanner::Add(const Pattern<S>& pattern, const PatternCallback& func)
	{
		m_Patterns.push_back(std::make_pair(&pattern, func));
	}
}
