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
	m_channelDepth = 1;
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
	m_channelDepth = 8;
	m_bytePerChannel = 1;
	m_bytePerPixel = 1;

	int maxValue;
	file >> maxValue;

	BYTE buffer;
	while ( file )
	{
		file >> buffer;
		m_colors.push_back( buffer );
	}

	std::for_each( m_colors.begin( ), m_colors.end( ),
					[maxValue = maxValue]( BYTE& color )
					{
						color = static_cast<BYTE>( static_cast<float>( color ) / maxValue * 255 );
					} );
}

void NETPBM::HandlePortablePixMap( std::ifstream& file )
{
	m_channelDepth = 8;
	m_bytePerChannel = 1;
	m_bytePerPixel = 3;

	int maxValue;
	file >> maxValue;

	BYTE buffer;
	while ( file )
	{
		file >> buffer;
		m_colors.push_back( buffer );
	}

	std::for_each( m_colors.begin( ), m_colors.end( ),
					[maxValue = maxValue]( BYTE& color )
					{
						color = static_cast<BYTE>( static_cast<float>( color ) / maxValue * 255 );
					} );
}

void NETPBM::HandlePortableBinaryBitMap( std::ifstream& file )
{
	m_channelDepth = 1;
	m_bytePerChannel = 1;
	m_bytePerPixel = 1;

	std::vector<BYTE> buffer;
	buffer.resize( m_width * m_height / 8 + 1 );

	file.read( reinterpret_cast<char*>( buffer.data( ) ), buffer.size() );

	BitReader br( buffer.data(), buffer.size( ) );

	for ( int i = 0, end = m_width * m_height; i < end; ++i )
	{
		m_colors.push_back( br.GetBit( 1 ) == 1 ? 255 : 0 );
	}
}

void NETPBM::HandlePortableBinaryGrayMap( std::ifstream& file )
{
	m_channelDepth = 8;
	m_bytePerChannel = 1;
	m_bytePerPixel = 1;

	int maxValue;
	file >> maxValue;

	char skip[2];
	file.read( skip, sizeof( skip ) );

	std::vector<BYTE> buffer;
	m_colors.resize( m_width * m_height );

	file.read( reinterpret_cast<char*>( m_colors.data( ) ), m_colors.size( ) );

	std::for_each( m_colors.begin( ), m_colors.end( ), 
					[maxValue = maxValue]( BYTE& color )
					{
						color = static_cast<BYTE>( static_cast<float>( color ) / maxValue * 255 );
					} );
}

void NETPBM::HandlePortableBinaryPixMap( std::ifstream& file )
{
	m_channelDepth = 8;
	m_bytePerChannel = 1;
	m_bytePerPixel = 3;

	int maxValue;
	file >> maxValue;

	char skip;
	file.read( &skip, sizeof( skip ) );

	m_colors.resize( m_width * m_height * 3 );

	file.read( reinterpret_cast<char*>( m_colors.data( ) ), m_colors.size( ) );

	std::for_each( m_colors.begin( ), m_colors.end( ),
					[maxValue = maxValue]( BYTE& color )
					{
						color = static_cast<BYTE>( static_cast<float>( color ) / maxValue * 255 );
					} );	
}
