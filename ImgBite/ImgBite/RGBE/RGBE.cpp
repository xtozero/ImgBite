#include "stdafx.h"
#include "RGBE.h"

#include "../Util.h"

#include <algorithm>
#include <fstream>

bool RGBE::Load( const char * filePath )
{
	Initialize( );
	
	auto file = std::ifstream( filePath, std::ifstream::binary );
	
	if ( !file )
	{
		return false;
	}

	if ( !ReadHeader( file ) )
	{
		return false;
	}

	if ( !ReadPixelRunLength( file ) )
	{
		return false;
	}

	return true;
}

void RGBE::ReinhardToneMapping( float key )
{
	m_colors.clear( );
	m_colors.reserve( m_width * m_height * m_bytePerPixel );

	float avgLum = 0.f;
	for ( int i = 0, end = m_data.size( ); i < end; i += 3 )
	{
		avgLum += logf( 0.00001f + ConvertsRGB2Luminance( &m_data[i] ) );
	}
	avgLum = exp( avgLum / ( m_width * m_height ) );

	float XYZ[3] = { 0.f, 0.f, 0.f };
	float Yxy[3] = { 0.f, 0.f, 0.f };
	float sRGB[3] = { 0.f, 0.f, 0.f };
	float scale = key / avgLum;
	float Lp = 0.f;
	for ( int i = 0, end = m_data.size( ); i < end; i += 3 )
	{
		sRGB2XYZ( &m_data[i], XYZ );
		XYZ2Yxy( XYZ, Yxy );

		// http://www.cmap.polytechnique.fr/~peyre/cours/x2005signal/hdr_photographic.pdf
		// Equation 3
		Lp = Yxy[0] * scale;
		Yxy[0] = Lp / ( 1.f + Lp );

		Yxy2XYZ( Yxy, XYZ );
		XYZ2sRGB( XYZ, sRGB );

		m_colors.push_back( static_cast<BYTE>( std::min( sRGB[0] * 255.f, 255.f ) ) );
		m_colors.push_back( static_cast<BYTE>( std::min( sRGB[1] * 255.f, 255.f ) ) );
		m_colors.push_back( static_cast<BYTE>( std::min( sRGB[2] * 255.f, 255.f ) ) );
	}
}

void RGBE::ReinhardToneMappingVer2( float key )
{
	m_colors.clear( );
	m_colors.reserve( m_width * m_height * m_bytePerPixel );

	float avgLum = 0.f;
	float lum = 0.f;
	float lumWhite = -FLT_MAX;
	for ( int i = 0, end = m_data.size( ); i < end; i += 3 )
	{
		lum = ConvertsRGB2Luminance( &m_data[i] );
		lumWhite = std::max( lumWhite, lum );
		avgLum += logf( 0.00001f + lum );
	}
	avgLum = exp( avgLum / ( m_width * m_height ) );
	lumWhite = lumWhite != 0.f ? 1 / ( lumWhite * lumWhite ) : 0.f;

	float XYZ[3] = { 0.f, 0.f, 0.f };
	float Yxy[3] = { 0.f, 0.f, 0.f };
	float sRGB[3] = { 0.f, 0.f, 0.f };
	float scale = key / avgLum;
	float Lp = 0.f;
	for ( int i = 0, end = m_data.size( ); i < end; i += 3 )
	{
		sRGB2XYZ( &m_data[i], XYZ );
		XYZ2Yxy( XYZ, Yxy );

		// http://www.cmap.polytechnique.fr/~peyre/cours/x2005signal/hdr_photographic.pdf
		// Equation 4
		Lp = Yxy[0] * scale;
		Yxy[0] = ( Lp * ( 1 + ( Lp * lumWhite ) ) ) / ( 1.f + Lp );

		Yxy2XYZ( Yxy, XYZ );
		XYZ2sRGB( XYZ, sRGB );

		m_colors.push_back( static_cast<BYTE>( std::min( sRGB[0] * 255.f, 255.f ) ) );
		m_colors.push_back( static_cast<BYTE>( std::min( sRGB[1] * 255.f, 255.f ) ) );
		m_colors.push_back( static_cast<BYTE>( std::min( sRGB[2] * 255.f, 255.f ) ) );
	}
}

bool RGBE::ReadHeader( std::ifstream& file )
{
	m_header.valid = 0;
	memset( m_header.programtype, 0, sizeof( m_header.programtype ) );
	m_header.gamma = m_header.exposure = 1.f;

	char buf[128];
	
	file.getline( buf, sizeof( buf ) );

	if ( buf[0] != '#' || buf[1] != '?' )
	{

	}
	else
	{
		m_header.valid |= RGBE_VALID_PROGRAMTYPE;
		for ( int i = 0, end = sizeof( m_header.programtype ) - 1; i < end; ++i )
		{
			if ( buf[i + 2] == 0 || isspace( buf[i + 2] ) )
			{
				break;
			}
			m_header.programtype[i] = buf[i + 2];
		}

		file.getline( buf, sizeof( buf ) );
	}

	float tempf = 0.f;
	while ( true )
	{
		if ( ( buf[0] == 0 ) || ( buf[0] == '\n' ) )
		{
			break;
		}
		else if ( sscanf_s( buf, "GAMMA=%g", &tempf ) == 1 )
		{
			m_header.gamma = tempf;
			m_header.valid |= RGBE_VALID_GAMMA;
		}
		else if ( sscanf_s( buf, "EXPOSURE=%g", &tempf ) == 1 )
		{
			m_header.exposure = tempf;
			m_header.valid |= RGBE_VALID_EXPOSURE;
		}

		file.getline( buf, sizeof( buf ) );
	}

	file.getline( buf, sizeof( buf ) );
	if ( sscanf_s( buf, "-Y %d +X %d", &m_height, &m_width ) < 2 )
	{
		return false;
	}

	m_channelDepth = 8;
	m_bytePerChannel = 1;
	m_bytePerPixel = 3;

	return true;
}

bool RGBE::ReadPixel( std::ifstream& file )
{
	m_data.resize( m_width * m_height * 3 );

	BYTE rgbe[4];
	float* sRGB = m_data.data( );
	for ( int i = 0, end = m_width * m_height; i < end; ++i )
	{
		file.read( reinterpret_cast<char*>( rgbe ), sizeof( rgbe ) );
		Rgbe2float( rgbe, sRGB );
		sRGB += 3;
	}

	return true;
}

bool RGBE::ReadPixelRunLength( std::ifstream& file )
{
	if ( m_width < 8 || m_width > 0x7FFF )
	{
		return ReadPixel( file );
	}

	int numScanlines = static_cast<int>( m_height );
	BYTE rgbe[4];
	BYTE* scanlineBuf = nullptr;
	BYTE* ptr = nullptr;
	BYTE* ptrEnd = nullptr;
	BYTE buf[2];
	int count;

	m_data.resize( m_width * m_height * 3 );
	float* sRGB = m_data.data( );

	while ( numScanlines > 0 )
	{
		file.read( reinterpret_cast<char*>( rgbe ), sizeof( rgbe ) );
		if ( rgbe[0] != 2 || rgbe[1] != 2 || rgbe[2] & 0x80 )
		{
			file.seekg( 0 );
			return ReadPixel( file );
		}

		if ( ( ( static_cast<unsigned int>( rgbe[2] ) << 8 ) | rgbe[3] ) != m_width )
		{
			return false;
		}

		if ( scanlineBuf == nullptr )
		{
			scanlineBuf = new BYTE[4 * m_width];
		}

		if ( scanlineBuf == nullptr )
		{
			return false;
		}

		ptr = &scanlineBuf[0];

		for ( int i = 0; i < 4; ++i )
		{
			ptrEnd = &scanlineBuf[( i + 1 ) * m_width];
			while ( ptr < ptrEnd )
			{
				file.read( reinterpret_cast<char*>( buf ), 2 );
				if ( buf[0] > 128 )
				{
					count = buf[0] - 128;
					if ( ( count == 0 ) || ( count > ptrEnd - ptr ) )
					{
						delete[] scanlineBuf;
						return false;
					}
					while ( count-- > 0 )
					{
						*ptr++ = buf[1];
					}
				}
				else
				{
					count = buf[0];

					if ( ( count == 0 ) || ( count > ptrEnd - ptr ) )
					{
						delete[] scanlineBuf;
						return false;
					}
					*ptr++ = buf[1];
					if ( --count > 0 )
					{
						file.read( reinterpret_cast<char*>( ptr ), count );
						ptr += count;
					}
				}
			}
		}

		for ( unsigned int i = 0; i < m_width; ++i )
		{
			rgbe[0] = scanlineBuf[i];
			rgbe[1] = scanlineBuf[i + m_width];
			rgbe[2] = scanlineBuf[i + 2 * m_width];
			rgbe[3] = scanlineBuf[i + 3 * m_width];
			Rgbe2float( rgbe, sRGB );
			sRGB += 3;
		}

		--numScanlines;
	}

	delete[] scanlineBuf;
	return true;
}
