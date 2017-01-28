#pragma once

#include "ByteArray.hpp"

#include <vector>

using BYTE = unsigned char;

class IMAGE
{
public:
	unsigned int GetWidth( ) const noexcept { return m_width; }
	unsigned int GetHeight( ) const  noexcept { return m_height; }
	BYTE GetChannelDepth( ) const noexcept { return m_channelDepth; }
	BYTE GetBytePerChannel( ) const noexcept { return m_bytePerChannel; }
	BYTE GetBytePerPixel( ) const noexcept { return m_bytePerPixel; }

	const std::vector<BYTE>& GetByteStream( ) const noexcept { return m_colors; }

	void Initialize( ) noexcept
	{
		m_width = 0;
		m_height = 0;
		m_channelDepth = 0;
		m_bytePerChannel = 0;
		m_bytePerPixel = 0;

		m_colors.clear( );
	}

	virtual ~IMAGE( ) = default;
protected:
	unsigned int m_width = 0;
	unsigned int m_height = 0;
	BYTE m_channelDepth = 0;
	BYTE m_bytePerChannel = 0;
	BYTE m_bytePerPixel = 0;

	std::vector<BYTE> m_colors;
};