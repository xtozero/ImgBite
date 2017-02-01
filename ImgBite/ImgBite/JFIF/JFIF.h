#pragma once

#include "../Image.h"

#include <array>
#include <fstream>

class JFIFDebugger;
class BitReader;

class JFIF : public IMAGE
{
public:
	bool Load( const char* filePath );

	enum MARKER_TYPE
	{
		SOF0 = 0xC0,		// Baseline DCT
		DHT = 0xC4,
		SOS = 0xDA,
		RST0 = 0xD0,
		SOI = 0xD8,
		DQT = 0xDB,
		DNL = 0xDC,
		DRI = 0xDD,
		RSTF = 0xDF,
		EOI = 0xD9,
		APP0 = 0xE0,
		APP1 = 0xE1,
		APP12 = 0xEC,		// Adobe Ducky
		APP13 = 0xEE,		// Adobe
		COM = 0xFE
	};

public:
	// Dubug class
	friend JFIFDebugger;

	JFIF( ) = default;
	~JFIF( ) = default;
	JFIF( const JFIF& ) = default;
	JFIF( JFIF&& ) = default;
	JFIF& operator=( const JFIF& ) = default;
	JFIF& operator=( JFIF&& ) = default;
private:
	// literals
	static constexpr int QUANT_TALBE_SIZE = 64;
	static constexpr int MAX_TABLE = 4;

	static constexpr int MAX_CODE_LENGTH = 16;
	static constexpr int MCU_WIDTH = 8;
	static constexpr int MCU_SIZE = MCU_WIDTH * MCU_WIDTH;
	static constexpr int CHANNEL_COUNT = 3;

	// quantization table access order
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

	//-------------------------------------------------------------------------------------------------------------
	//	Functions
	//-------------------------------------------------------------------------------------------------------------

	// File Read Functions
	BYTE ReadNextMarker( );
	std::streampos FindNextMarkerPos( );
	size_t ReadFieldLength( );

	// JFIF Marker Handle Functions
	void InitHandler( );
	void HandleMarker( const BYTE marker );
	void HandleJFIFAppMarker( );
	void HandleUnsupportedAppMarker( );
	void HandleIgnoreAppMarker( );
	void HandleSOFMarker( );
	void HandleDQTMarker( );
	void HandleDHTMarker( );
	void HandleSOSMarker( );
	void HandleScan( );
	void HandleDRIMarker( );

	// Decode Functions
	void ProcessDecode( BitReader& bt, const int mcuX, const int mcuY );
	
	class HuffmanInfo;
	using HuffmanTable = std::vector<HuffmanInfo>;
	std::tuple<int, BYTE> GetVLD( BitReader& bt, const HuffmanTable& huffmanTable );
	
	// Inverse Discrete Cosine Transform Functions
	void DoRowIDCT( std::array<int, QUANT_TALBE_SIZE>& table );
	void DoColIDCT( std::array<int, QUANT_TALBE_SIZE>& table );

	void Convert2RGB( );

	//-------------------------------------------------------------------------------------------------------------
	//	Variables
	//-------------------------------------------------------------------------------------------------------------
	using HandleMarkerFunc = void (JFIF::*)(void);
	std::array<HandleMarkerFunc, 0x100> m_markerHandler;

	std::ifstream m_imgFile;
	std::vector<BYTE> m_internalBuf = {};
	size_t m_internalBufLength = 0;

	class HuffmanInfo
	{
	public:
		HuffmanInfo( const unsigned short range, const BYTE codeLength, const BYTE symbol ) :
			m_codeRange( range ),
			m_codeLength( codeLength ),
			m_symbol( symbol )
		{}

		template<typename T> constexpr bool operator<( T&& rhs ) const { return m_codeRange < rhs; }

		int m_codeRange = 0;
		BYTE m_codeLength = 0;
		BYTE m_symbol = 0;
	};

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
	};
	std::vector<FrameInfo> m_frameDesc = {};

	int m_mcuSizeX = 0;
	int m_mcuSizeY = 0;
	int m_mcuWidth = 0;
	int m_mcuHeight = 0;
	BYTE m_comCount = 0;

	std::vector<std::vector<int>> m_componentPixel = {};

	BYTE m_maxSamplingFreqX = 0;
	BYTE m_maxSamplingFreqY = 0;
};

