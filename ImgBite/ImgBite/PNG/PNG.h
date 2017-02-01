#pragma once

#include "../Image.h"

class PNG : public IMAGE
{
public:
	bool Load( const char* filePath );

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

	std::ptrdiff_t GetLeftPixelIndex( ) const noexcept { return static_cast<std::ptrdiff_t>( m_colors.size() - GetBytePerPixel( ) ); }
	std::ptrdiff_t GetAbovePixelIndex( ) const noexcept { return static_cast<std::ptrdiff_t>( m_colors.size( ) - GetBytePerPixel( ) * GetWidth() ); }
	std::ptrdiff_t GetUpperLeftPiexlIndex( ) const noexcept { return static_cast<std::ptrdiff_t>( GetAbovePixelIndex( ) - GetBytePerPixel( ) ); }
};

