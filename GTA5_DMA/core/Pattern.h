#pragma once
#include <cstdint>
#include <array>
#include <optional>
#include <span>
#include <string_view>
#include <vector>
#include <algorithm>

// Ported from YimMenu V2 - adapted for DMA context
// Original: src/core/util/StrToHex.hpp, src/core/memory/PatternHash.hpp, src/core/memory/Pattern.hpp

namespace DMAOffsets
{
	inline constexpr std::uint8_t StrToHex(const char& ch) noexcept
	{
		if (ch >= '0' && ch <= '9')
			return ch - '0';
		if (ch >= 'A' && ch <= 'F')
			return ch - 'A' + 10;
		return ch - 'a' + 10;
	}

	class PatternHash
	{
	public:
		std::uint64_t m_Hash;

		constexpr PatternHash() :
			m_Hash(0xD84A917A64D4D016ULL)
		{
		}

		constexpr PatternHash(std::uint64_t hash) :
			m_Hash(hash)
		{
		}

		constexpr PatternHash(const PatternHash& other) :
			m_Hash(other.m_Hash)
		{
		}

		constexpr PatternHash(PatternHash&& other) :
			m_Hash(other.m_Hash)
		{
		}

		constexpr void operator=(const PatternHash& other)
		{
			m_Hash = other.m_Hash;
		}

		constexpr PatternHash Update(char data) const
		{
			auto hash = m_Hash;
			hash += (static_cast<std::uint64_t>(data));
			hash += hash << 10;
			hash ^= (hash >> 6);
			return PatternHash(hash);
		}

		constexpr PatternHash Update(int data) const
		{
			auto hash = m_Hash;
			hash ^= (data >> 8);
			hash += hash << 10;
			hash ^= (hash >> 7);
			return PatternHash(hash);
		}

		constexpr PatternHash Update(std::uint64_t data) const
		{
			auto hash = m_Hash;
			hash ^= (data * 0xA5C38736C426FCB8ULL);
			hash += hash << 15;
			hash ^= (hash >> 5);
			return PatternHash(hash);
		}

		constexpr std::uint64_t GetHash() const
		{
			return m_Hash;
		}
	};

	template<std::size_t N>
	struct Signature
	{
		char m_Signature[N]{};
		std::size_t m_SignatureByteLength;
		PatternHash m_Hash;

		constexpr Signature(char const (&signature)[N]) :
			m_SignatureByteLength(0)
		{
			std::ranges::copy(signature, m_Signature);

			for (std::size_t i = 0; i < N; i++)
			{
				m_Hash = m_Hash.Update(m_Signature[i]);
				if (m_Signature[i] == ' ' || m_Signature[i] == '\0')
					m_SignatureByteLength++;
			}

			m_Hash = m_Hash.Update(m_SignatureByteLength);
		}

		inline constexpr const decltype(m_Signature)& Get() const
		{
			return m_Signature;
		}

		constexpr std::size_t Length() const
		{
			return N;
		}

		constexpr std::size_t ByteLength() const
		{
			return m_SignatureByteLength;
		}

		constexpr PatternHash Hash() const
		{
			return m_Hash;
		}
	};

	class IPattern
	{
	public:
		constexpr IPattern() = default;
		virtual ~IPattern() = default;

		virtual const std::string_view Name() const = 0;
		virtual constexpr std::span<const std::optional<std::uint8_t>> Signature() const = 0;
		virtual const PatternHash Hash() const = 0;
	};

	template<Signature S>
	class Pattern final : public IPattern
	{
	private:
		const std::string_view m_Name;
		std::array<std::optional<std::uint8_t>, S.ByteLength()> m_Signature;
		PatternHash m_Hash;

	public:
		constexpr Pattern(const std::string_view name);

		inline virtual const std::string_view Name() const override
		{
			return m_Name;
		}
		inline virtual constexpr std::span<const std::optional<std::uint8_t>> Signature() const override
		{
			return m_Signature;
		}
		inline virtual const PatternHash Hash() const override
		{
			return m_Hash;
		}
	};

	template<Signature S>
	inline constexpr Pattern<S>::Pattern(const std::string_view name) :
		IPattern(),
		m_Name(name)
	{
		m_Hash = S.Hash();

		for (size_t i = 0, pos = 0; i < S.Length(); i++)
		{
			const auto c = S.Get()[i];
			if (c == ' ')
				continue;
			if (c == '\0')
				break;

			if (S.Get()[i] == '?')
			{
				if (S.Get()[i + 1] == '?')
					i++;

				m_Signature[pos++] = std::nullopt;
				continue;
			}

			int next = i + 1;
			m_Signature[pos++] = static_cast<std::uint8_t>(StrToHex(S.Get()[i]) * 0x10 + StrToHex(S.Get()[next]));
			i++;
		}
	}
}
