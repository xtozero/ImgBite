#pragma once

#include "../Image.h"

#include <array>

class JFIFDebugger;
class BitReader;

class JFIF : public IMAGE
{
public:
	bool Load( const char* filePath );

	enum MARKER_TYPE
	{
		SOI = 0xD8,
		APP0 = 0xE0,
		APPF = 0xEF,
		DQT = 0xDB,
		SOF0 = 0xC0,
		SOFF = 0xCF,
		DHT = 0xC4,
		SOS = 0xDA,
		DRI = 0xDD,
		RST0 = 0xD0,
		RSTF = 0xDF,
		DNL = 0xDC,
		EOI = 0xD9,
		COM = 0xFE
	};

public:
	//Dubug class
	friend JFIFDebugger;

private:
	static constexpr int MAX_BUFFER = 65536 << 3;

	static constexpr int QUANT_TALBE_SIZE = 64;
	static constexpr int MAX_TABLE = 4;

	//	quantization table access order
	static constexpr BYTE QTABLE_ORDER[QUANT_TALBE_SIZE] = {
		0,	1,	8,	16,	9,	2,	3,	10,
		17,	24,	32,	25,	18,	11,	4,	5,
		12,	19,	26,	33,	40,	48,	41,	34,
		27,	20,	13,	6,	7,	14,	21,	28,
		35,	42,	49,	56,	57,	50,	43,	36,
		29,	22,	15,	23,	30,	37,	44,	51,
		58,	59,	52,	45,	38,	31,	39,	46,
		53,	60,	61,	54,	47,	55,	62,	63,
	};

	static constexpr int MAX_CODE_LENGTH = 16;

	static constexpr int MCU_WIDTH = 8;
	static constexpr int MCU_SIZE = MCU_WIDTH * MCU_WIDTH;
	static constexpr int CHANNEL_COUNT = 3;

	BYTE ReadNextMarker( std::ifstream& file ) const;
	std::streampos FindNextMarkerPos( std::ifstream& file );

	size_t ReadFieldLength( std::ifstream& file );

	void HandleMarker( std::ifstream& file, const BYTE marker );
	
	void HandleJFIFAppMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length );
	void HandleOtherAppMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length );
	void HandleSOFMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length );

	void HandleDQTMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length );
	void HandleDHTMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length );
	void HandleSOSMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length );
	void HandleScan( BYTE( &buffer )[MAX_BUFFER], const size_t length );
	void HandleDRIMarker( const BYTE( &buffer )[MAX_BUFFER], const size_t length );

	void ProcessDecode( BitReader& bt, const int mcuX, const int mcuY );
	void DoRowIDCT( std::array<int, QUANT_TALBE_SIZE>& table );
	void DoColIDCT( std::array<int, QUANT_TALBE_SIZE>& table );

	void Convert2RGB( );

	class HuffmanInfo
	{
	public:
		HuffmanInfo( const unsigned short range, const BYTE codeLength, const BYTE symbol ) :
			m_codeRange( range ),
			m_codeLength( codeLength ),
			m_symbol( symbol )
		{}

		int m_codeRange = 0;
		BYTE m_codeLength = 0;
		BYTE m_symbol = 0;
	};

	using HuffmanTable = std::vector<HuffmanInfo>;
	int GetVLD( BitReader& bt, const HuffmanTable& huffmanTable, BYTE& code/*out*/ );

	using QuantTable = std::array<unsigned short, QUANT_TALBE_SIZE>;
	using QuantTables = std::array<QuantTable, MAX_TABLE>;
	QuantTables m_quantTable = {};
	int m_quantTableCount = 0;

	enum HUFFMAN_TABLE_TYPE
	{
		DC = 0,
		AC,
		TABLE_TYPE_COUNT,
	};

	using HuffmanTables = std::array<HuffmanTable, MAX_TABLE>;
	std::array<HuffmanTables, HUFFMAN_TABLE_TYPE::TABLE_TYPE_COUNT> m_huffmanTable = {};

	short m_restartInterval = 0;

	class ScanInfo
	{
	public:
		ScanInfo( const BYTE comID, const BYTE huffID ) :
			m_comID( comID ), 
			m_dcID( huffID >> 4 & 0xF) , 
			m_acID( huffID & 0xF )
		{}

		BYTE m_comID = 0;
		BYTE m_dcID = 0;
		BYTE m_acID = 0;
		int m_predDC = 0;
	};
	std::vector<ScanInfo> m_scanDesc = {};

	class FrameInfo
	{
	public:
		FrameInfo( const BYTE comID, const BYTE samplingFreq, const BYTE quantTableID ) :
			m_comID( comID ),
			m_xSamplingFreq( samplingFreq >> 4 & 0xF ),
			m_ySamplingFreq( samplingFreq & 0xF ),
			m_quantTableID( quantTableID )
		{}

		BYTE m_comID = 0;
		BYTE m_xSamplingFreq = 0;
		BYTE m_ySamplingFreq = 0;
		BYTE m_quantTableID = 0;
		int m_width = 0;
		int m_height = 0;
	};
	std::vector<FrameInfo> m_frameDesc = {};

	int m_mcuSizeX = 0;
	int m_mcuSizeY = 0;
	int m_mcuWidth = 0;
	int m_mcuHeight = 0;
	BYTE m_comCount = 0;

	std::vector<std::vector<int>> m_componentPixel;

	BYTE m_maxSamplingFreqX = 0;
	BYTE m_maxSamplingFreqY = 0;
};

