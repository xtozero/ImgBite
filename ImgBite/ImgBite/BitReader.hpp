#pragma once

#include <cassert>
#include <algorithm>

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

	int ShowBit( int bits )
	{
		assert( bits <= ( sizeof( m_cache ) * BIT_PER_BYTE ) );
		CacheBit( bits );

		assert( m_cached >= bits );
		return (m_cache >> ( m_cached - bits )) & ((0x1 << bits) - 1);
	}

	void SkipBit( int bits )
	{
		assert( bits <= (sizeof( m_cache ) * BIT_PER_BYTE) );
		CacheBit( bits );

		assert( m_cached >= bits );
		m_cached -= bits;
	}

	int GetBit( int bits )
	{
		if ( bits <= 0 )
		{
			return 0;
		}

		int ret = ShowBit( bits );
		SkipBit( bits );
		return ret;
	}

private:
	int CalcRequireCached( int bits ) const noexcept
	{
		return std::max( 0, bits - m_cached );
	}

	void CacheBit( int bits ) noexcept
	{
		int required = CalcRequireCached( bits );

		while ( required > 0 )
		{
			m_cache = (m_cache << BIT_PER_BYTE) | m_buffer[m_curOffset];

			m_cached += BIT_PER_BYTE;
			required -= BIT_PER_BYTE;
			++m_curOffset;
		}

		while ( m_cached < bits )
		{
			m_cached += 8;
			m_cache = (m_cache << BIT_PER_BYTE) | 0xFF;
		}
	}

	const BYTE* m_buffer = nullptr;
	size_t m_length = 0;
	size_t m_curOffset = 0;
	int m_cached = 0;

	size_t m_cache = 0;
};