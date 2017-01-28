#pragma once

using BYTE = unsigned char;

class ByteBufferReader
{
public:
	ByteBufferReader( const BYTE* buffer, size_t length ) noexcept :
		m_buffer( buffer ),
		m_length( length )
	{
	}

	template <typename T>
	T Get( )
	{
		assert( m_curOffset + sizeof( T ) <= m_length );
		const T* ret = reinterpret_cast<const T*>( m_buffer + m_curOffset );
		m_curOffset += sizeof( T );

		return *ret;
	}

	explicit operator bool() noexcept
	{
		return m_curOffset < m_length;
	}

private:
	const BYTE* m_buffer = nullptr;
	size_t m_length = 0;
	size_t m_curOffset = 0;
};