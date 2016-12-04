#pragma once

using BYTE = unsigned char;

class IMAGE
{
protected:
	size_t m_width = 0;
	size_t m_height = 0;
	BYTE m_channelDepth = 0;
	BYTE m_bytePerChannel = 0;
	BYTE m_bytePerPixel = 0;
};