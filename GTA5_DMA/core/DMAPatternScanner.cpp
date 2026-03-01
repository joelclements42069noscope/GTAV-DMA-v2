#include "pch.h"
#include "core/DMAPatternScanner.h"
#include "core/DMAPatternCache.h"

#include <future>

namespace DMAOffsets
{
	DMAPatternScanner::DMAPatternScanner(const DMAModule* module)
		: m_Module(module), m_Patterns()
	{
	}

	bool DMAPatternScanner::Scan()
	{
		if (!m_Module || !m_Module->Valid())
			return false;

		bool scanSuccess = true;

		// Multi-threaded scanning via std::async
		std::vector<std::future<bool>> jobs;
		for (const auto& [pattern, func] : m_Patterns)
		{
			jobs.emplace_back(std::async(std::launch::async, &DMAPatternScanner::ScanInternal, this, pattern, func));
		}

		for (auto& job : jobs)
		{
			job.wait();
			if (scanSuccess)
				scanSuccess = job.get();
		}

		return scanSuccess;
	}

	bool DMAPatternScanner::ScanInternal(const IPattern* pattern, PatternCallback func) const
	{
		const auto signature = pattern->Signature();
		const auto& buffer = m_Module->Buffer();
		const auto moduleBase = m_Module->Base();
		const auto moduleSize = m_Module->Size();

		// Check cache first
		if (DMAPatternCache::IsInitialized())
		{
			auto offset = DMAPatternCache::GetCachedOffset(pattern->Hash().Update(moduleSize));
			if (offset.has_value())
			{
				uintptr_t resolvedAddr = moduleBase + offset.value();
				std::println("[PatternScanner] Cached [{}] : [{:X}]", pattern->Name(), resolvedAddr);
				std::invoke(func, ResolvedPtr(resolvedAddr, m_Module));
				return true;
			}
		}

		// Scan the local buffer
		for (size_t i = 0; i < moduleSize; ++i)
		{
			if (signature.size() + i > moduleSize)
				break;

			bool found = true;
			for (size_t sigIdx = 0; sigIdx < signature.size(); ++sigIdx)
			{
				if (signature[sigIdx] && signature[sigIdx].value() != buffer[i + sigIdx])
				{
					found = false;
					break;
				}
			}

			if (found)
			{
				uintptr_t remoteAddr = moduleBase + i;
				std::println("[PatternScanner] Found [{}] : [{:X}]", pattern->Name(), remoteAddr);

				std::invoke(func, ResolvedPtr(remoteAddr, m_Module));

				// Cache the result
				if (DMAPatternCache::IsInitialized())
				{
					DMAPatternCache::UpdateCachedOffset(
						pattern->Hash().Update(moduleSize),
						static_cast<int>(i));
				}

				return true;
			}
		}

		std::println("[PatternScanner] FAILED to find [{}]", pattern->Name());
		return false;
	}
}
