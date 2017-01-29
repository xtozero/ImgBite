#include "stdafx.h"
#include "JFIF.h"

#include "../DCT.hpp"
#include "../BitReader.hpp"
#include "../ByteBufferReader.hpp"

#include <bitset>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>

namespace
{
	bool IsAPP( const BYTE marker )
	{
		return marker >= JFIF::APP0 && marker <= JFIF::APPF;
	}

	bool IsSOF( const BYTE marker )
	{
		return marker >= JFIF::SOF0 && marker < JFIF::SOFF;
	}

	BYTE ClipColor( const int component )
	{
		return static_cast<BYTE>( std::max( 0, std::min( 0xFF, component ) ) );
	}

	using Color = std::tuple<int, int, int>;
	Color YCbCr2RGB( int y, int cb, int cr )
	{
		int r = static_cast<int>( round( 1.402 * ( cr - 128 ) + y ) );
		int g = static_cast<int>( round( -0.34414 * ( cb - 128 ) - 0.71414 * ( cr - 128 ) + y ) );
		int b = static_cast<int>( round( 1.772 * ( cb - 128 ) + y ) );
		return Color{ ClipColor( r ), ClipColor( g ), ClipColor( b ) };
	}
};

bool JFIF::Load( const char * filePath )
{
	Initialize( );

	auto file = std::ifstream( filePath, std::ifstream::binary );

	if ( !file )
	{
		return false;
	}

	BYTE marker = SOI;
	while ( marker )
	{
		marker = ReadNextMarker( file );
		HandleMarker( file, marker );
	}

	return true;
}

BYTE JFIF::ReadNextMarker( std::ifstream & file ) const
{
	BYTE marker[2] = { 0 };

	while ( file )
	{
		file.read( reinterpret_cast<char*>(marker), sizeof( marker ) );
		if ( marker[0] == 0xFF && marker[1] != 0 )
		{
			return marker[1];
		}
	}

	return 0;
}

std::streampos JFIF::FindNextMarkerPos( std::ifstream & file )
{
	std::streampos prev = file.tellg( );
	std::streampos cur = prev;
	BYTE buffer = 0;

	while ( file )
	{
		file.read( reinterpret_cast<char*>(&buffer), sizeof( buffer ) );
		
		if ( buffer == 0xFF )
		{
			file.read( reinterpret_cast<char*>(&buffer), sizeof( buffer ) );

			if ( buffer != 0x00 )
			{
				file.seekg( -2, std::ios::cur );
				cur = file.tellg( );
				file.seekg( prev );
				break;
			}
		}
	}

	return cur;
}

size_t JFIF::ReadFieldLength( std::ifstream & file )
{
	ByteArray<2> size;
	file.read( reinterpret_cast<char*>(&size), sizeof( size ) );

	// Length of segment include length field. so need to sub 2byte
	return size.GetHostOrder<unsigned short>( ) - 2;
}

void JFIF::HandleMarker( std::ifstream & file, const BYTE marker )
{
	// Don't have to handle these markers
	if ( marker == SOI || marker == EOI )
	{
		return;
	}

	size_t length = ReadFieldLength( file );
	//BYTE buffer[MAX_BUFFER];
	//file.read( reinterpret_cast<char*>( buffer ), length );

	std::vector<BYTE> buffer;
	buffer.resize( length );
	file.read( reinterpret_cast<char*>(buffer.data()), length );

	if ( marker == DQT )
	{
		HandleDQTMarker( buffer, length );
	}
	else if ( marker == DHT )
	{
		HandleDHTMarker( buffer, length );
	}
	else if ( marker == SOS )
	{
		HandleSOSMarker( buffer, length );
		
		std::streampos cur = file.tellg( );
		length = static_cast<size_t>( FindNextMarkerPos( file ) - cur );
		assert( MAX_BUFFER >= length );
		buffer.resize( length );
		file.read( reinterpret_cast<char*>(buffer.data()), length );
		//file.read( reinterpret_cast<char*>(buffer), length );
		
		HandleScan( buffer, length );
	}
	else if ( marker == DRI )
	{
		HandleDRIMarker( buffer, length );
	}
	else if ( marker == DNL )
	{

	}
	else if ( marker == COM )
	{
		//Comment marker. so just skip
	}
	else if ( IsAPP( marker ) )
	{
		if ( marker == APP0 )
		{
			HandleJFIFAppMarker( buffer, length );
		}
		else
		{
			HandleOtherAppMarker( buffer, length );
		}
	}
	else if ( IsSOF( marker ) )
	{
		HandleSOFMarker( buffer, length );
	}
}

void JFIF::HandleJFIFAppMarker( const std::vector<BYTE>& buffer, const size_t )
{
	// Don't need to handle JFIP app marker yet

	//ByteBufferReader br( &buffer[5], length - 5 );

	//char major = br.Get<ByteArray<1>>( );
	//char minor = br.Get<ByteArray<1>>( );
	//char units = br.Get<ByteArray<1>>( );

	//short xDensity = br.Get<ByteArray<2>>( );
	//short yDensity = br.Get<ByteArray<2>>( );
	//
	//char xThumbnail = br.Get<ByteArray<1>>( );
	//char yThumbnail = br.Get<ByteArray<1>>( );
}

void JFIF::HandleOtherAppMarker( const std::vector<BYTE>& buffer, const size_t )
{
	//Don't have to handle other app markers
}

void JFIF::HandleSOFMarker( const std::vector<BYTE>& buffer, const size_t length )
{
	ByteBufferReader br( buffer.data(), length );

	// skip samplingPrecision
	br.Get<BYTE>( );
	
	short height = br.Get<ByteArray<2>>( );
	short width = br.Get<ByteArray<2>>( );
	m_height = static_cast<unsigned int>( height );
	m_width = static_cast<unsigned int>( width );

	m_bytePerPixel = m_comCount = static_cast<int>( br.Get<BYTE>( ) );
	m_frameDesc.clear( );
	m_frameDesc.reserve( m_comCount );

	m_componentPixel.resize( m_comCount );
	for ( auto& pixel : m_componentPixel )
	{
		pixel.resize( m_width * m_height );
	}

	m_colors.resize( m_width * m_height * m_bytePerPixel );

	BYTE comID = 0;
	BYTE samplingFreq = 0;
	BYTE quantTableID = 0;

	for ( int i = 0; i < m_comCount; ++i )
	{
		comID = br.Get<BYTE>( );
		samplingFreq = br.Get<BYTE>( );
		quantTableID = br.Get<BYTE>( );

		m_frameDesc.emplace_back( comID, samplingFreq, quantTableID );
		m_maxSamplingFreqX = std::max( m_maxSamplingFreqX, m_frameDesc[i].m_xSamplingFreq );
		m_maxSamplingFreqY = std::max( m_maxSamplingFreqY, m_frameDesc[i].m_ySamplingFreq );
	}

	for ( int i = 0; i < m_comCount; ++i )
	{
		auto& frame = m_frameDesc[i];
		frame.m_width = m_width * frame.m_xSamplingFreq / m_maxSamplingFreqX;
		frame.m_height = m_height * frame.m_ySamplingFreq / m_maxSamplingFreqY;;
	}

	m_mcuSizeX = m_maxSamplingFreqX << 3;
	m_mcuSizeY = m_maxSamplingFreqY << 3;
	m_mcuWidth = static_cast<int>( std::ceil( static_cast<float>( m_width ) / m_mcuSizeX ) );
	m_mcuHeight = static_cast<int>( std::ceil( static_cast<float>( m_height ) / m_mcuSizeY ) );
}

void JFIF::HandleDQTMarker( const std::vector<BYTE>& buffer, const size_t length )
{
	ByteBufferReader br( buffer.data(), length );
	
	while ( br )
	{
		BYTE tableIdentifier = br.Get<BYTE>( );

		bool isWORD = ( ( tableIdentifier & 0xF0 ) >> 4 ) == 1;
		BYTE tableID = ( tableIdentifier & 0x0F );

		assert( tableID < MAX_TABLE );

		for ( int i = 0; i < QUANT_TALBE_SIZE; ++i )
		{
			m_quantTable[tableID][i] = isWORD ? br.Get<ByteArray<2>>( ) : static_cast<short>( br.Get<char>( ) );
		}

		++m_quantTableCount;
	}
}

void JFIF::HandleDHTMarker( const std::vector<BYTE>& buffer, const size_t length )
{
	ByteBufferReader br( buffer.data(), length );

	while ( br )
	{
		BYTE tableIdentifier = br.Get<BYTE>( );

		int tableClass = ( tableIdentifier & 0xF0 ) >> 4;
		int tableID = ( tableIdentifier & 0x0F );

		assert( tableID < MAX_TABLE );

		std::array<BYTE, MAX_CODE_LENGTH> counter = {};

		for ( auto& count : counter )
		{
			count = br.Get<char>( );
		}

		HuffmanTable& table = m_huffmanTable[tableClass][tableID];

		int range = 0;
		int remain = 65536;
		BYTE symbol = 0;

		for ( int i = 0; i < 16; ++i )
		{
			remain >>= 1;
			for ( int j = 0; j < counter[i]; ++j )
			{
				symbol = br.Get<BYTE>( );
				range += remain;
				table.emplace_back( range - 1, i + 1, symbol );
			}
		}
	}
}

void JFIF::HandleSOSMarker( const std::vector<BYTE>& buffer, const size_t length )
//void JFIF::HandleSOSMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length )
{
	ByteBufferReader br( buffer.data(), length );

	m_comCount = static_cast<int>( br.Get<BYTE>( ) );

	m_scanDesc.clear( );
	m_scanDesc.reserve( m_comCount );

	for ( int i = 0; i < m_comCount; ++i )
	{
		BYTE comID = br.Get<BYTE>( );
		BYTE huffID = br.Get<BYTE>( );

		m_scanDesc.emplace_back( comID, huffID );
	}

	//BYTE spectralStart = br.Get<BYTE>( );
	//BYTE spectralEnd = br.Get<BYTE>( );
	//BYTE successiveAppr = br.Get<BYTE>( );
}

void JFIF::HandleScan( std::vector<BYTE>& buffer, const size_t length )
{
	size_t compressed = length;

	// Change 0xFF00 to 0xFF
	for ( size_t i = 0, j = 0; i < length && j < length; ++i, ++j )
	{
		buffer[i] = buffer[j];

		if ( buffer[j] == 0xFF )
		{
			++j;
			--compressed;
		}
	}

	BitReader bt( buffer.data(), compressed );

	for ( int y = 0; y < m_mcuHeight; ++y )
	{
		for ( int x = 0; x < m_mcuWidth; ++x )
		{
			ProcessDecode( bt, x, y );
		}
	}

	Convert2RGB( );
}

void JFIF::HandleDRIMarker( const std::vector<BYTE>& buffer, const size_t length )
{
	ByteBufferReader br( buffer.data(), length );

	m_restartInterval = br.Get<short>( );
}

void JFIF::HandleJFIFAppMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t )
{
	// Don't need to handle JFIP app marker yet

	//ByteBufferReader br( &buffer[5], length - 5 );

	//char major = br.Get<ByteArray<1>>( );
	//char minor = br.Get<ByteArray<1>>( );
	//char units = br.Get<ByteArray<1>>( );

	//short xDensity = br.Get<ByteArray<2>>( );
	//short yDensity = br.Get<ByteArray<2>>( );
	//
	//char xThumbnail = br.Get<ByteArray<1>>( );
	//char yThumbnail = br.Get<ByteArray<1>>( );
}

void JFIF::HandleOtherAppMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t )
{
	//Don't have to handle other app markers
}

void JFIF::HandleSOFMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length )
{
	ByteBufferReader br( buffer, length );

	// skip samplingPrecision
	br.Get<BYTE>();

	short height = br.Get<ByteArray<2>>();
	short width = br.Get<ByteArray<2>>();
	m_height = static_cast<unsigned int>(height);
	m_width = static_cast<unsigned int>(width);

	m_bytePerPixel = m_comCount = static_cast<int>(br.Get<BYTE>());
	m_frameDesc.clear();
	m_frameDesc.reserve( m_comCount );

	m_componentPixel.resize( m_comCount );
	for ( auto& pixel : m_componentPixel )
	{
		pixel.resize( m_width * m_height );
	}

	m_colors.resize( m_width * m_height * m_bytePerPixel );

	BYTE comID = 0;
	BYTE samplingFreq = 0;
	BYTE quantTableID = 0;

	for ( int i = 0; i < m_comCount; ++i )
	{
		comID = br.Get<BYTE>();
		samplingFreq = br.Get<BYTE>();
		quantTableID = br.Get<BYTE>();

		m_frameDesc.emplace_back( comID, samplingFreq, quantTableID );
		m_maxSamplingFreqX = std::max( m_maxSamplingFreqX, m_frameDesc[i].m_xSamplingFreq );
		m_maxSamplingFreqY = std::max( m_maxSamplingFreqY, m_frameDesc[i].m_ySamplingFreq );
	}

	for ( int i = 0; i < m_comCount; ++i )
	{
		auto& frame = m_frameDesc[i];
		frame.m_width = m_width * frame.m_xSamplingFreq / m_maxSamplingFreqX;
		frame.m_height = m_height * frame.m_ySamplingFreq / m_maxSamplingFreqY;;
	}

	m_mcuSizeX = m_maxSamplingFreqX << 3;
	m_mcuSizeY = m_maxSamplingFreqY << 3;
	m_mcuWidth = static_cast<int>(std::ceil( static_cast<float>(m_width) / m_mcuSizeX ));
	m_mcuHeight = static_cast<int>(std::ceil( static_cast<float>(m_height) / m_mcuSizeY ));
}

void JFIF::HandleDQTMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length )
{
	ByteBufferReader br( buffer, length );

	while ( br )
	{
		BYTE tableIdentifier = br.Get<BYTE>();

		bool isWORD = ((tableIdentifier & 0xF0) >> 4) == 1;
		BYTE tableID = (tableIdentifier & 0x0F);

		assert( tableID < MAX_TABLE );

		for ( int i = 0; i < QUANT_TALBE_SIZE; ++i )
		{
			m_quantTable[tableID][i] = isWORD ? br.Get<ByteArray<2>>() : static_cast<short>(br.Get<char>());
		}

		++m_quantTableCount;
	}
}

void JFIF::HandleDHTMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length )
{
	ByteBufferReader br( buffer, length );

	while ( br )
	{
		BYTE tableIdentifier = br.Get<BYTE>();

		int tableClass = (tableIdentifier & 0xF0) >> 4;
		int tableID = (tableIdentifier & 0x0F);

		assert( tableID < MAX_TABLE );

		std::array<BYTE, MAX_CODE_LENGTH> counter = {};

		for ( auto& count : counter )
		{
			count = br.Get<char>();
		}

		HuffmanTable& table = m_huffmanTable[tableClass][tableID];

		int range = 0;
		int remain = 65536;
		BYTE symbol = 0;

		for ( int i = 0; i < 16; ++i )
		{
			remain >>= 1;
			for ( int j = 0; j < counter[i]; ++j )
			{
				symbol = br.Get<BYTE>();
				range += remain;
				table.emplace_back( range - 1, i + 1, symbol );
			}
		}
	}
}

void JFIF::HandleSOSMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length )
{
	ByteBufferReader br( buffer, length );

	m_comCount = static_cast<int>(br.Get<BYTE>());

	m_scanDesc.clear();
	m_scanDesc.reserve( m_comCount );

	for ( int i = 0; i < m_comCount; ++i )
	{
		BYTE comID = br.Get<BYTE>();
		BYTE huffID = br.Get<BYTE>();

		m_scanDesc.emplace_back( comID, huffID );
	}

	//BYTE spectralStart = br.Get<BYTE>( );
	//BYTE spectralEnd = br.Get<BYTE>( );
	//BYTE successiveAppr = br.Get<BYTE>( );
}

void JFIF::HandleScan( BYTE( &buffer )[MAX_BUFFER], const size_t length )
{
	size_t compressed = length;

	// Change 0xFF00 to 0xFF
	for ( size_t i = 0, j = 0; i < length && j < length; ++i, ++j )
	{
		buffer[i] = buffer[j];

		if ( buffer[j] == 0xFF )
		{
			++j;
			--compressed;
		}
	}

	BitReader bt( buffer, compressed );

	for ( int y = 0; y < m_mcuHeight; ++y )
	{
		for ( int x = 0; x < m_mcuWidth; ++x )
		{
			ProcessDecode( bt, x, y );
		}
	}

	Convert2RGB();
}

void JFIF::HandleDRIMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length )
{
	ByteBufferReader br( buffer, length );

	m_restartInterval = br.Get<short>();
}


void JFIF::ProcessDecode( BitReader& bt, const int mcuX, const int mcuY )
{
	using VldTable = std::array<int, QUANT_TALBE_SIZE>;
	using VldTables = std::vector<VldTable>;
	VldTables vldTables;
	vldTables.resize( m_comCount );

	BYTE code;
	
	for ( int i = 0; i < m_comCount; ++i )
	{
		for ( int y = 0; y < m_frameDesc[i].m_ySamplingFreq; ++y )
		{
			for ( int x = 0; x < m_frameDesc[i].m_xSamplingFreq; ++x )
			{
				VldTable& vldTable = vldTables[i];
				vldTable.fill( 0 );

				const HuffmanTable& dcTable = m_huffmanTable[DC][m_scanDesc[i].m_dcID];
				m_scanDesc[i].m_predDC += GetVLD( bt, dcTable, code );

				assert( m_quantTableCount > m_frameDesc[i].m_quantTableID );
				const QuantTable& quantTable = m_quantTable[m_frameDesc[i].m_quantTableID];
				vldTable[0] = m_scanDesc[i].m_predDC * quantTable[0];

				const HuffmanTable& acTable = m_huffmanTable[AC][m_scanDesc[i].m_acID];
				int coef = 0;
				constexpr int VALID_QUANT_TALBE_INDEX = QUANT_TALBE_SIZE - 1;
				do
				{
					int value = GetVLD( bt, acTable, code );
					if ( code == 0 )
					{
						break;
					}
					coef += (code >> 4) + 1;
					vldTable[QTABLE_ORDER[coef]] = value * quantTable[coef];
				} while( coef < VALID_QUANT_TALBE_INDEX );

				DoRowIDCT( vldTable );
				DoColIDCT( vldTable );

				std::vector<int>& pixel = m_componentPixel[i];

				int startX = mcuX * MCU_WIDTH * m_maxSamplingFreqX + ( x * MCU_WIDTH );
				int startY = mcuY * MCU_WIDTH * m_maxSamplingFreqY + ( y * MCU_WIDTH );

				int shiftX = 0;
				int shiftY = 0;

				while ( m_maxSamplingFreqY > ( m_frameDesc[i].m_xSamplingFreq << shiftX ) )
				{
					++shiftX;
				}

				while ( m_maxSamplingFreqY > ( m_frameDesc[i].m_ySamplingFreq << shiftY ) )
				{
					++shiftY;
				}

				for ( int h = 0; h < MCU_WIDTH << shiftY; ++h )
				{
					int stride = (startY + h) * m_width;
					int* input = &vldTable[(h >> shiftY) * MCU_WIDTH];

					for ( int w = 0; w < MCU_WIDTH << shiftX; ++w )
					{
						size_t colorIdx = stride + (startX + w);

						if ( colorIdx >= pixel.size( ) )
						{
							continue;
						}

						pixel[colorIdx] = input[w >> shiftX] + 128;
					}
				}
			}
		}
	}
}

void JFIF::DoRowIDCT( std::array<int, QUANT_TALBE_SIZE>& table )
{
	FormulaIDCT8x8::DoRowIDCT( table.data() );
}

void JFIF::DoColIDCT( std::array<int, QUANT_TALBE_SIZE>& table )
{
	FormulaIDCT8x8::DoColIDCT( table.data() );
}

void JFIF::Convert2RGB( )
{
	for ( size_t i = 0; i < m_componentPixel[0].size( ); ++i )
	{
		int y = m_componentPixel[0][i];
		int cb = m_componentPixel[1][i];
		int cr = m_componentPixel[2][i];

		std::tuple<BYTE, BYTE, BYTE> color = YCbCr2RGB( y, cb, cr );

		m_colors[i * 3] = std::get<0>( color );
		m_colors[i * 3 + 1] = std::get<1>( color );
		m_colors[i * 3 + 2] = std::get<2>( color );
	}
}

int JFIF::GetVLD( BitReader& bt, const HuffmanTable& huffmanTable, BYTE& code/*out*/ )
{
	int value = bt.ShowBit( 16 );

	auto found = std::lower_bound( huffmanTable.begin( ), huffmanTable.end( ), value,
									[]( const HuffmanInfo& lhs, int rhs )
									{
										return lhs.m_codeRange < rhs;
									} );
	assert( found != huffmanTable.end( ) );

	bt.SkipBit( found->m_codeLength );

	code = found->m_symbol;
	int bits = found->m_symbol & 0xF;
	if ( bits == 0 )
	{
		return 0;
	}

	value = bt.GetBit( bits );
	if ( value < (1 << (bits - 1)) )
	{
		value += ((-1) << bits) + 1;
	}

	return value;
}
