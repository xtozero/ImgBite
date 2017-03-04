#include "stdafx.h"
#include "NETPBM.h"

#include "../BitReader.hpp"

bool NETPBM::Load( const char* filePath )
{
	auto file = std::ifstream( filePath, std::ios::binary );

	if ( !file )
	{
		return false;
	}

	char magicNum[3];

	file >> magicNum >> m_width >> m_height;

	// skip '\n' character
	char skip;
	file.read( &skip, 1 );

	m_channelDepth = 8;

	if ( magicNum[0] != 'P' )
	{
		return false;
	}

	switch ( magicNum[1] )
	{
	case '1':
		HandlePortableBitMap( file );
		break;
	case '2':
		HandlePortableGrayMap( file );
		break;
	case '3':
		HandlePortablePixMap( file );
		break;
	case '4':
		HandlePortableBinaryBitMap( file );
		break;
	case '5':
		HandlePortableBinaryGrayMap( file );
		break;
	case '6':
		HandlePortableBinaryPixMap( file );
		break;
	default:
		return false;
	}

	return true;
}

void NETPBM::HandlePortableBitMap( std::ifstream& file )
{
	m_bytePerChannel = 1;
	m_bytePerPixel = 1;

	BYTE buffer;

	while ( file )
	{
		file >> buffer;
		m_colors.push_back( buffer == 1 ? 255 : 0 );
	}
}

void NETPBM::HandlePortableGrayMap( std::ifstream& file )
{
}

void NETPBM::HandlePortablePixMap( std::ifstream& file )
{
}

void NETPBM::HandlePortableBinaryBitMap( std::ifstream& file )
{
	m_bytePerChannel = 1;
	m_bytePerPixel = 1;

	std::vector<BYTE> buffer;
	buffer.resize( m_width * m_height / 8 + 1 );

	file.read( reinterpret_cast<char*>( buffer.data( ) ), buffer.size() );

	BitReader br( buffer.data(), buffer.size( ) );

	for ( int i = 0, end = m_width * m_height; i < end; ++i )
	{
		m_colors.push_back( br.GetBit( 1 ) );
	}

	/*std::ofstream dump( "dump.txt" );

	for ( int y = 0; y < m_height; ++y )
	{
		for ( int x = 0; x < m_width; ++x )
		{
			dump << static_cast<int>( m_colors[y * m_height + x] );
		}
		dump << std::endl;
	}*/
}

void NETPBM::HandlePortableBinaryGrayMap( std::ifstream& file )
{
}

void NETPBM::HandlePortableBinaryPixMap( std::ifstream& file )
{
}
