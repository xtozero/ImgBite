#pragma once

#include <cassert>

using BYTE = unsigned char;

class BitReader final
{
	static constexpr int BIT_PER_BYTE = 8;

public:
	BitReader( const BYTE* buffer, size_t length ) noexcept :
		m_buffer( buffer ),
		m_length( length )
	{
	}

	int ShowBit( size_t bits )
	{
		assert( bits <= ( sizeof( m_cache ) * BIT_PER_BYTE ) );
		CacheBit( bits );

		assert( m_cached >= bits );
		return (m_cache >> ( m_cached - bits )) & ((0x1 << bits) - 1);
	}

	size_t SkipBit( size_t bits )
	{
		assert( bits <= (sizeof( m_cache ) * BIT_PER_BYTE) );
		CacheBit( bits );

		assert( m_cached >= bits );
		return m_cached -= bits;
	}

	int GetBit( int bits )
	{
		if ( bits == 0 )
		{
			return 0;
		}

		int ret = ShowBit( bits );
		SkipBit( bits );
		return ret;
	}

private:
	bool IsBufferRemain( int bits ) const noexcept
	{
		return ( m_curOffset + std::ceil( bits / 8 ) ) != m_length;
	}

	bool IsCacheRemain( int bits ) const noexcept
	{
		return ( m_cached + bits ) <= (sizeof( m_cache ) * BIT_PER_BYTE);
	}

	int CalcRequireCached( size_t bits ) const noexcept
	{
		int required = static_cast<int>( bits - m_cached );

		if ( !IsBufferRemain( required ) )
		{
			return 0;
		}

		if ( !IsCacheRemain( required ) )
		{
			return 0;
		}

		return required;
	}

	void CacheBit( size_t bits ) noexcept
	{
		int required = CalcRequireCached( bits );

		while ( required > 0 )
		{
			m_cache = (m_cache << BIT_PER_BYTE) | m_buffer[m_curOffset];

			m_cached += BIT_PER_BYTE;
			required -= BIT_PER_BYTE;
			++m_curOffset;
		}

		if ( m_cached < bits )
		{
			m_cached += 8;
			m_cache = (m_cache << BIT_PER_BYTE) | 0xFF;
			return;
		}
	}

	const BYTE* m_buffer = nullptr;
	size_t m_length = 0;
	size_t m_curOffset = 0;
	size_t m_cached = 0;

	size_t m_cache = 0;
};