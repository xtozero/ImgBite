#pragma once

#include <istream>
#include <cassert>

template <int N>
class ByteArray final
{
public:
	unsigned char* operator&( ) noexcept
	{
		return m_bytes;
	}

	template <typename T>
	void operator=( const T& value ) noexcept
	{
		static_assert(sizeof( T ) == N, "Data type size is not matched");
		
		const unsigned char* pSrc = reinterpret_cast<const unsigned char*>( &value );
		
		int srcSize = sizeof( T );
		for ( int i = 0; i < srcSize; ++i )
		{
			m_bytes[i] = *pSrc;
			++pSrc;
		}
	}

	unsigned char operator[]( const size_t idx ) const noexcept
	{
		assert( idx < N );
		return m_bytes[idx];
	}

	template <typename T>
	T Get() const noexcept
	{
		static_assert(sizeof( T ) == N, "Data type size is not matched");

		T value = *(reinterpret_cast<const T*>(m_bytes));
		return value;
	}

	template <typename T>
	T GetNetworkOrder( ) const noexcept
	{
		return GetByteSwapOrder<T>( );
	}

	template <typename T>
	T GetHostOrder( ) const noexcept
	{
		return GetByteSwapOrder<T>( );
	}

	template <typename T>
	operator T( ) const noexcept
	{
		return GetHostOrder<T>( );
	}

private:
	template <typename T>
	T GetByteSwapOrder( ) const noexcept
	{
		static_assert(sizeof( T ) == N, "Data type size is not matched");

		unsigned char ntValue[N];

		for ( int i = 0, j = N - 1; i < N || j >= 0; ++i, --j )
		{
			ntValue[j] = m_bytes[i];
		}

		T value = *(reinterpret_cast<const T*>(ntValue));
		return value;
	}

	unsigned char m_bytes[N] = {0,};
};

template <int N>
std::istream& operator>> ( std::istream& is, ByteArray<N>& val )
{
	is.read( &val, N );
	return is;
}