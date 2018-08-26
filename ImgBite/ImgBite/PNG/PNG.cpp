#include "stdafx.h"
#include "PNG.h"

#include "../../zlib/include/zlib.h"

#include <array>
#include <fstream>

#pragma comment( lib, "zdll.lib" )

namespace
{
	BYTE GetPngPixelSize( BYTE colortype )
	{
		constexpr BYTE pixelSizes[] = { 1, 0, 3, 1, 2, 0, 4 };

		return pixelSizes[colortype];
	}

	BYTE PaethPredictor( BYTE a, BYTE b, BYTE c )
	{
		int p = a + b - c;
		int pa = std::abs( p - a );
		int pb = std::abs( p - b );
		int pc = std::abs( p - c );
		if ( pa <= pb && pa <= pc )
		{
			return a;
		}
		else if ( pb <= pc )
		{
			return b;
		}
		else
		{
			return c;
		}
	}
}

bool PNG::Load( const char* filePath )
{
	Initialize( );

	auto file = std::ifstream( filePath, std::ifstream::binary );

	if ( !file )
	{
		return false;
	}

	// Skip Signature
	file.seekg( 8, std::ios::beg );

	std::vector<BYTE> src;
	std::vector<BYTE> data;

	ChunkBegin header;
	ChunkEnd tail;
	while ( file )
	{
		file.read( reinterpret_cast<char*>(&header), sizeof( header ) );

		HandleChunk( file, header, src );

		file.read( reinterpret_cast<char*>(&tail), sizeof( tail ) );
	}

	//The compressed datastream is then the concatenation of the contents of all the IDAT chunks
	data = InflatePixelData( src );

	if ( data.size() == 0 )
	{
		return false;
	}

	if ( !ApplyFilter( data ) )
	{
		return false;
	}

	return true;
}

void PNG::HandleChunk( std::ifstream& file, const ChunkBegin& header, std::vector<BYTE>& src )
{
	const char* typeStr = reinterpret_cast<const char*>(&header.m_type);

	if ( strncmp( typeStr, "IHDR", 4 ) == 0 )
	{
		ReadIHDRChunk( file );
	}
	else if ( strncmp( typeStr, "IDAT", 4 ) == 0 )
	{
		ReadIDATChunk( file, header.m_length.GetHostOrder<int>( ), src );
	}
	else
	{
		file.seekg( header.m_length.GetHostOrder<int>( ), std::ios::cur );
	}
}

void PNG::ReadIHDRChunk( std::ifstream& file )
{
	IHDRChunk ihdr;
	file.read( reinterpret_cast<char*>(&ihdr), sizeof( ihdr ) );

	m_width = ihdr.m_width.GetHostOrder<unsigned int>( );
	m_height = ihdr.m_height.GetHostOrder<unsigned int>( );
	m_channelDepth = ihdr.m_bitDepth.GetHostOrder<BYTE>( );
	m_bytePerChannel = static_cast<BYTE>(std::ceil( static_cast<float>(m_channelDepth) / 8 )); // byte per channel
	m_bytePerPixel = m_bytePerChannel * GetPngPixelSize( ihdr.m_colorType.GetHostOrder<BYTE>( ) ); // byte per pixel
}

void PNG::ReadIDATChunk( std::ifstream& file, int chunkLength, std::vector<BYTE>& src )
{
	// There can be multiple IDAT chunks.
	size_t offset = src.size( );
	src.resize( src.size( ) + chunkLength );

	file.read( reinterpret_cast<char*>( src.data() + offset ), chunkLength );
}

std::vector<BYTE> PNG::InflatePixelData( std::vector<BYTE>& src )
{
	std::vector<BYTE> data;
	data.resize( m_width * m_height * m_bytePerPixel + m_height );

	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	if ( inflateInit( &stream ) != Z_OK )
	{
		return data;
	}

	stream.avail_in = static_cast<unsigned int>( src.size() );
	stream.next_in = src.data( );

	// Inflate data at once
	stream.avail_out = static_cast<unsigned int>( data.size( ) );
	stream.next_out = data.data( );
	inflate( &stream, Z_FINISH );

 	return data;
}

bool PNG::ApplyFilter( std::vector<BYTE>& data )
{
	size_t length = m_bytePerPixel * m_width;
	size_t lineOffset = length + 1;

	BYTE filterType = data[0];

	m_colors.reserve( m_width * m_height * m_bytePerPixel );

	for ( size_t i = 0; i < data.size( ); i += lineOffset )
	{
		// When filter type is None necessary to insert a filter type byte before the data.
		if ( data[i] != NONE )
		{
			filterType = data[i];
		}

		BYTE* rowStart = &data[i + 1];
		switch ( filterType )
		{
		case NONE: ApplyNoneFilter( rowStart, length );
			break;
		case SUB: ApplySubFilter( rowStart, length );
			break;
		case UP: ApplyUpFilter( rowStart, length );
			break;
		case AVERAGE: ApplyAverageFilter( rowStart, length );
			break;
		case PAETH: ApplyPaethFilter( rowStart, length );
			break;
		default:
			// undefined filter type
			assert( false );
			break;
		}
	}

	return true;
}

void PNG::ApplyNoneFilter( const BYTE* data, size_t end )
{
	m_colors.insert( m_colors.end( ), data, data + end );
}

void PNG::ApplySubFilter( const BYTE* data, size_t end )
{
	std::ptrdiff_t leftIdx = GetLeftPixelIndex( );

	std::ptrdiff_t rowStartIdx = m_colors.size( );

	for ( size_t i = 0; i < end; ++i, ++leftIdx )
	{
		BYTE left = leftIdx < rowStartIdx ? 0 : m_colors[leftIdx];

		m_colors.push_back( ( *(data + i) + left) % 256 );
	}
}

void PNG::ApplyUpFilter( const BYTE * data, size_t end )
{
	std::ptrdiff_t aboveIdx = GetAbovePixelIndex( );

	for ( size_t i = 0; i < end; ++i, ++aboveIdx )
	{
		BYTE above = aboveIdx < 0 ? 0 : m_colors[aboveIdx];

		m_colors.push_back( (*(data + i) + above) % 256 );
	}
}

void PNG::ApplyAverageFilter( const BYTE * data, size_t end )
{
	std::ptrdiff_t aboveIdx = GetAbovePixelIndex( );
	std::ptrdiff_t leftIdx = GetLeftPixelIndex( );

	std::ptrdiff_t rowStartIdx = m_colors.size( );

	for ( size_t i = 0; i < end; ++i, ++aboveIdx, ++leftIdx )
	{
		BYTE above = aboveIdx < 0 ? 0 : m_colors[aboveIdx];
		BYTE left = leftIdx < rowStartIdx ? 0 : m_colors[leftIdx];

		BYTE average = static_cast<BYTE>(std::floor( (above + left) / 2.f ));
		m_colors.push_back( (*(data + i) + average) % 256 );
	}
}

void PNG::ApplyPaethFilter( const BYTE * data, size_t end )
{
	std::ptrdiff_t aboveIdx = GetAbovePixelIndex( );
	std::ptrdiff_t upperLeftIdx = GetUpperLeftPiexlIndex( );
	std::ptrdiff_t leftIdx = GetLeftPixelIndex( );

	std::ptrdiff_t rowStartIdx = m_colors.size( );

	for ( size_t i = 0; i < end; ++i, ++aboveIdx, ++upperLeftIdx, ++leftIdx )
	{
		BYTE upperLeft = (upperLeftIdx < 0 || leftIdx < rowStartIdx) ? 0 : m_colors[upperLeftIdx];
		BYTE above = aboveIdx < 0 ? 0 : m_colors[aboveIdx];
		BYTE left = leftIdx < rowStartIdx ? 0 : m_colors[leftIdx];

		m_colors.push_back( ( *(data + i) + PaethPredictor( left, above, upperLeft ) ) % 256 );
	}
}
