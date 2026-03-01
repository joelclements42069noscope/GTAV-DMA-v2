#pragma once
#include "core/Pattern.h"

#include <unordered_map>
#include <optional>
#include <mutex>
#include <fstream>
#include <string>

namespace DMAOffsets
{
	class DMAPatternCache
	{
	public:
		static void Init(const std::string& cacheFilePath = "pattern_cache.bin")
		{
			GetInstance().InitImpl(cacheFilePath);
		}

		static void Save()
		{
			GetInstance().SaveImpl();
		}

		static std::optional<int> GetCachedOffset(PatternHash hash)
		{
			return GetInstance().GetCachedOffsetImpl(hash);
		}

		static void UpdateCachedOffset(PatternHash hash, int offset)
		{
			GetInstance().UpdateCachedOffsetImpl(hash, offset);
		}

		static bool IsInitialized()
		{
			return GetInstance().m_Initialized;
		}

	private:
		static DMAPatternCache& GetInstance()
		{
			static DMAPatternCache Instance;
			return Instance;
		}

		void InitImpl(const std::string& cacheFilePath);
		void SaveImpl();
		std::optional<int> GetCachedOffsetImpl(PatternHash hash);
		void UpdateCachedOffsetImpl(PatternHash hash, int offset);

		bool m_Initialized = false;
		std::string m_FilePath;
		std::unordered_map<std::uint64_t, int> m_Data;
		std::mutex m_Mutex;
	};
}
