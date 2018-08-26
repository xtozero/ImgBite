#include "stdafx.h"
#include "JFIF.h"

#include "../DCT.hpp"
#include "../BitReader.hpp"
#include "../ByteBufferReader.hpp"
#include "../Util.h"

#include <bitset>
#include <iostream>
#include <numeric>
#include <sstream>

bool JFIF::Load( const char * filePath )
{
	Initialize( );
	InitHandler( );

	m_imgFile = std::ifstream( filePath, std::ifstream::binary );

	if ( !m_imgFile )
	{
		return false;
	}

	BYTE marker = SOI;
	while ( marker != EOI )
	{
		marker = ReadNextMarker( );
		HandleMarker( marker );
	}

	m_imgFile.close( );

	return true;
}

BYTE JFIF::ReadNextMarker( )
{
	BYTE marker[2] = { 0 };

	while ( m_imgFile )
	{
		m_imgFile.read( reinterpret_cast<char*>(marker), sizeof( marker ) );
		if ( marker[0] == 0xFF && marker[1] != 0 )
		{
			return marker[1];
		}
	}

	return 0;
}

std::streampos JFIF::FindNextMarkerPos( )
{
	constexpr int MAX_BUFFER = 256;

	std::streampos prev = m_imgFile.tellg( );
	std::array<BYTE, MAX_BUFFER> buffer = {};
	bool needCheckNext = false;
	int count = 0;
	bool exitLoop = false;

	while ( m_imgFile && !exitLoop )
	{
		m_imgFile.read( reinterpret_cast<char*>(buffer.data( )), MAX_BUFFER );

		if ( needCheckNext && buffer[0] != 0x00 )
		{
			--count;
			break;
		}

		for ( int i = 0; i < MAX_BUFFER - 1; ++i )
		{
			if ( buffer[i] == 0xFF && buffer[i + 1] != 0x00 )
			{
				exitLoop = true;
				break;
			}
			++count;
		}

		if ( !exitLoop )
		{
			needCheckNext = (buffer[MAX_BUFFER - 1] == 0xFF);
			++count;
		}
	}

	m_imgFile.clear();
	m_imgFile.seekg( prev );
	prev += count;
	return prev;
}

size_t JFIF::ReadFieldLength( )
{
	ByteArray<2> size;
	m_imgFile.read( reinterpret_cast<char*>(&size), sizeof( size ) );

	// Length of segment include length field. so need to sub 2byte
	return size.GetHostOrder<unsigned short>( ) - 2;
}

void JFIF::InitHandler( )
{
	m_markerHandler.fill( &JFIF::HandleUnsupportedAppMarker );

	m_markerHandler[SOF0] = &JFIF::HandleSOFMarker;
	m_markerHandler[DHT] = &JFIF::HandleDHTMarker;
	m_markerHandler[SOS] = &JFIF::HandleSOSMarker;
	m_markerHandler[SOI] = &JFIF::HandleIgnoreAppMarker;
	m_markerHandler[DQT] = &JFIF::HandleDQTMarker;
	m_markerHandler[DNL] = &JFIF::HandleIgnoreAppMarker;
	m_markerHandler[DRI] = &JFIF::HandleIgnoreAppMarker;
	m_markerHandler[EOI] = &JFIF::HandleIgnoreAppMarker;
	m_markerHandler[APP0] = &JFIF::HandleJFIFAppMarker;
	m_markerHandler[APP1] = &JFIF::HandleIgnoreAppMarker;
	m_markerHandler[APP12] = &JFIF::HandleIgnoreAppMarker;
	m_markerHandler[APP13] = &JFIF::HandleIgnoreAppMarker;
	m_markerHandler[COM] = &JFIF::HandleIgnoreAppMarker;
}

void JFIF::HandleMarker( const BYTE marker )
{
	// Don't have to handle these markers
	if ( marker == SOI || marker == EOI )
	{
		return;
	}

	m_internalBufLength = ReadFieldLength( );

	m_internalBuf.resize( m_internalBufLength );
	m_imgFile.read( reinterpret_cast<char*>(m_internalBuf.data( )), m_internalBufLength );

	(this->*m_markerHandler[marker])( );
}

void JFIF::HandleJFIFAppMarker( )
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

void JFIF::HandleUnsupportedAppMarker( )
{
	// Handle unsupported app markers
	assert( false );
}

void JFIF::HandleIgnoreAppMarker( )
{
}

void JFIF::HandleSOFMarker( )
{
	ByteBufferReader br( m_internalBuf.data(), m_internalBufLength );

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

	m_mcuSizeX = m_maxSamplingFreqX << 3;
	m_mcuSizeY = m_maxSamplingFreqY << 3;
	m_mcuWidth = static_cast<int>( std::ceil( static_cast<float>( m_width ) / m_mcuSizeX ) );
	m_mcuHeight = static_cast<int>( std::ceil( static_cast<float>( m_height ) / m_mcuSizeY ) );
}

void JFIF::HandleDQTMarker(  )
{
	ByteBufferReader br( m_internalBuf.data(), m_internalBufLength );
	
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

void JFIF::HandleDHTMarker( )
{
	ByteBufferReader br( m_internalBuf.data(), m_internalBufLength );

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
		int remain = 0x10000; //2^16
		BYTE symbol = 0;

		for ( int i = 0; i < MAX_CODE_LENGTH; ++i )
		{
			remain >>= 1;
			for ( int j = 0; j < counter[i]; ++j )
			{
				symbol = br.Get<BYTE>( );
				range += remain;
				table.emplace_back( range - 1, i + 1, symbol );
			}
		}

		if ( range < 0xFFFF )
		{
			table.emplace_back( 0xFFFF, 0, 0 );
		}
	}
}

void JFIF::HandleSOSMarker( )
{
	ByteBufferReader br( m_internalBuf.data(), m_internalBufLength );

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

	HandleScan( );
}

void JFIF::HandleScan( )
{
	std::streampos cur = m_imgFile.tellg( );
	size_t compressed = m_internalBufLength = static_cast<size_t>(FindNextMarkerPos( ) - cur);
	m_internalBuf.resize( m_internalBufLength );
	m_imgFile.read( reinterpret_cast<char*>(m_internalBuf.data( )), m_internalBufLength );

	// Change 0xFF00 to 0xFF
	for ( size_t i = 0, j = 0; i < m_internalBufLength && j < m_internalBufLength; ++i, ++j )
	{
		m_internalBuf[i] = m_internalBuf[j];

		if ( m_internalBuf[j] == 0xFF )
		{
			++j;
			--compressed;
		}
	}

	BitReader bt( m_internalBuf.data(), compressed );

	for ( int y = 0; y < m_mcuHeight; ++y )
	{
		for ( int x = 0; x < m_mcuWidth; ++x )
		{
			ProcessDecode( bt, x, y );
		}
	}

	Convert2RGB( );
}

void JFIF::HandleDRIMarker( )
{
	ByteBufferReader br( m_internalBuf.data(), m_internalBufLength );

	m_restartInterval = br.Get<short>( );
}

void JFIF::ProcessDecode( BitReader& bt, const int mcuX, const int mcuY )
{
	using VldTable = std::array<int, QUANT_TALBE_SIZE>;
	using VldTables = std::vector<VldTable>;
	VldTables vldTables;
	vldTables.resize( m_comCount );

	int value = 0;
	BYTE code = 0;
	
	for ( int i = 0; i < m_comCount; ++i )
	{
		std::vector<int>& pixel = m_componentPixel[i];

		for ( int y = 0; y < m_frameDesc[i].m_ySamplingFreq; ++y )
		{
			for ( int x = 0; x < m_frameDesc[i].m_xSamplingFreq; ++x )
			{
				VldTable& vldTable = vldTables[i];
				vldTable.fill( 0 );

				const HuffmanTable& dcTable = m_huffmanTable[DC][m_scanDesc[i].m_dcID];
				m_scanDesc[i].m_predDC += std::get<0>( GetVLD( bt, dcTable ) );

				assert( m_quantTableCount > m_frameDesc[i].m_quantTableID );
				const QuantTable& quantTable = m_quantTable[m_frameDesc[i].m_quantTableID];
				vldTable[0] = m_scanDesc[i].m_predDC * quantTable[0];

				const HuffmanTable& acTable = m_huffmanTable[AC][m_scanDesc[i].m_acID];
				int coef = 0;
				constexpr int VALID_QUANT_TALBE_INDEX = QUANT_TALBE_SIZE - 1;
				do
				{
					std::tie( value, code ) = GetVLD( bt, acTable );
					if ( code == 0 )
					{
						break;
					}
					coef += (code >> 4) + 1;
					vldTable[QTABLE_ORDER[coef]] = value * quantTable[coef];
				} while( coef < VALID_QUANT_TALBE_INDEX );

				DoRowIDCT( vldTable );
				DoColIDCT( vldTable );

				int startX = mcuX * MCU_WIDTH * m_maxSamplingFreqX + ( x * MCU_WIDTH );
				int startY = mcuY * MCU_WIDTH * m_maxSamplingFreqY + ( y * MCU_WIDTH );

				int shiftX = 0;
				int shiftY = 0;

				while ( m_maxSamplingFreqX > ( m_frameDesc[i].m_xSamplingFreq << shiftX ) )
				{
					++shiftX;
				}

				while ( m_maxSamplingFreqY > ( m_frameDesc[i].m_ySamplingFreq << shiftY ) )
				{
					++shiftY;
				}

				for ( int h = 0, hEnd = MCU_WIDTH << shiftY; h < hEnd; ++h )
				{
					int stride = (startY + h) * m_width;
					int* input = &vldTable[(h >> shiftY) * MCU_WIDTH];

					for ( int w = 0, wEnd = MCU_WIDTH << shiftX; w < wEnd; ++w )
					{
						size_t colorIdx = stride + (startX + w);

						if ( colorIdx >= pixel.size( ) )
						{
							continue;
						}

						pixel[colorIdx] = input[w >> shiftX] + 128; // Convert -128 ~ 127 to 0 ~ 255
					}
				}
			}
		}
	}
}

std::tuple<int, BYTE> JFIF::GetVLD( BitReader& bt, const HuffmanTable& huffmanTable )
{
	int value = bt.ShowBit( MAX_CODE_LENGTH );
	auto found = std::lower_bound( huffmanTable.begin( ), huffmanTable.end( ), value );
	assert( found != huffmanTable.end( ) );

	bt.SkipBit( found->m_codeLength );

	BYTE code = found->m_symbol;
	int bits = found->m_symbol & 0xF;
	if ( bits <= 0 )
	{
		return { 0, code };
	}

	value = bt.GetBit( bits );
	if ( value < (1 << (bits - 1)) )
	{
		value += ((-1) << bits) + 1;
	}

	return { value, code };
}

void JFIF::DoRowIDCT( std::array<int, QUANT_TALBE_SIZE>& table )
{
	FastIDCT8x8::DoRowIDCT( table.data() );
}

void JFIF::DoColIDCT( std::array<int, QUANT_TALBE_SIZE>& table )
{
	FastIDCT8x8::DoColIDCT( table.data() );
}

void JFIF::Convert2RGB( )
{
	BYTE* pColors = m_colors.data( );

	for ( size_t i = 0, end = m_width * m_height; i < end; ++i )
	{
		YCbCr2RGBFast( m_componentPixel[0][i], m_componentPixel[1][i], m_componentPixel[2][i], pColors[0], pColors[1], pColors[2] );
		pColors += m_bytePerPixel;
	}
}
