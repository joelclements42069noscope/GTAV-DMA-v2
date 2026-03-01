#include "pch.h"
#include "core/DMAPatternCache.h"

namespace DMAOffsets
{
	void DMAPatternCache::InitImpl(const std::string& cacheFilePath)
	{
		std::lock_guard lock(m_Mutex);
		m_FilePath = cacheFilePath;

		std::ifstream stream(m_FilePath, std::ios_base::binary);
		if (stream.is_open())
		{
			while (!stream.eof())
			{
				std::uint64_t hash = 0;
				int offset = 0;

				stream.read(reinterpret_cast<char*>(&hash), sizeof(hash));
				stream.read(reinterpret_cast<char*>(&offset), sizeof(offset));

				if (stream.gcount() > 0)
					m_Data.emplace(hash, offset);
			}

			std::println("[PatternCache] Loaded {} cached offsets from {}", m_Data.size(), m_FilePath);
		}
		else
		{
			std::println("[PatternCache] No cache file found, will create after scan");
		}

		m_Initialized = true;
	}

	void DMAPatternCache::SaveImpl()
	{
		std::lock_guard lock(m_Mutex);

		std::ofstream stream(m_FilePath, std::ios_base::binary);
		if (!stream.is_open())
		{
			std::println("[PatternCache] Failed to write cache file: {}", m_FilePath);
			return;
		}

		for (auto& [hash, offset] : m_Data)
		{
			auto h = hash;
			stream.write(reinterpret_cast<char*>(&h), sizeof(h));
			stream.write(reinterpret_cast<char*>(&offset), sizeof(offset));
		}

		std::println("[PatternCache] Saved {} offsets to {}", m_Data.size(), m_FilePath);
	}

	std::optional<int> DMAPatternCache::GetCachedOffsetImpl(PatternHash hash)
	{
		std::lock_guard lock(m_Mutex);
		if (auto it = m_Data.find(hash.GetHash()); it != m_Data.end())
			return it->second;
		return std::nullopt;
	}

	void DMAPatternCache::UpdateCachedOffsetImpl(PatternHash hash, int offset)
	{
		std::lock_guard lock(m_Mutex);
		m_Data[hash.GetHash()] = offset;
	}
}
