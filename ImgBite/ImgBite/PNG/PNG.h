#pragma once

#include "../ByteArray.hpp"
#include "../Image.h"

#include <vector>

class PNG
{
public:
	bool Load( const char* filePath );

	unsigned int GetWidth( ) const noexcept { return m_width; }
	unsigned int GetHeight( ) const  noexcept { return m_height; }
	BYTE GetChannelDepth( ) const noexcept { return m_channelDepth; }
	BYTE GetBytePerChannel( ) const noexcept { return m_bytePerChannel; }
	BYTE GetBytePerPixel( ) const noexcept { return m_bytePerPixel; }

	const std::vector<BYTE>& GetByteStream( ) const noexcept { return m_colors; }

private:
	struct ChunkBegin
	{
		ByteArray<4> m_length;
		ByteArray<4> m_type;
	};

	struct ChunkEnd
	{
		ByteArray<4> m_crc;
	};

	struct IHDRChunk
	{
		ByteArray<4> m_width;
		ByteArray<4> m_height;
		ByteArray<1> m_bitDepth;
		ByteArray<1> m_colorType;
		ByteArray<1> m_compression;
		ByteArray<1> m_filter;
		ByteArray<1> m_interlace;
	};

	enum FILTER_TYPE
	{
		NONE = 0,
		SUB,
		UP,
		AVERAGE,
		PAETH
	};

	void Initialize( ) noexcept;

	void HandleChunk( std::ifstream& file, const ChunkBegin& header, std::vector<BYTE>& src );

	void ReadIHDRChunk( std::ifstream& file );
	void ReadIDATChunk( std::ifstream& file, int chunkLength, std::vector<BYTE>& src );
	
	std::vector<BYTE> InflatePixelData( std::vector<BYTE>& src );

	bool ApplyFilter( std::vector<BYTE>& data );

	void ApplyNoneFilter( const BYTE* data, size_t end );
	void ApplySubFilter( const BYTE* data, size_t end );
	void ApplyUpFilter( const BYTE* data, size_t end );
	void ApplyAverageFilter( const BYTE* data, size_t end );
	void ApplyPaethFilter( const BYTE* data, size_t end );

	using diff_type = std::ptrdiff_t;
	diff_type GetLeftPixelIndex( ) const noexcept { return static_cast<diff_type>( m_colors.size() - GetBytePerPixel( ) ); }
	diff_type GetAbovePixelIndex( ) const noexcept { return static_cast<diff_type>( m_colors.size( ) - GetBytePerPixel( ) * GetWidth() ); }
	diff_type GetUpperLeftPiexlIndex( ) const noexcept { return static_cast<diff_type>( GetAbovePixelIndex( ) - GetBytePerPixel( ) ); }

	unsigned int m_width = 0;
	unsigned int m_height = 0;
	BYTE m_channelDepth = 0;
	BYTE m_bytePerChannel = 0;
	BYTE m_bytePerPixel = 0;

	std::vector<BYTE> m_colors;
};

