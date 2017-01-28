#include "stdafx.h"
#include "JFIFDebugger.h"

#include "JFIF.h"

#include <bitset>
#include <iomanip>
#include <iostream>

#define HEX( x ) std::hex << (int)x

void JFIFDebugger::PrintQuantTable() const
{
	std::array<short, JFIF::QUANT_TALBE_SIZE> tempTable = {};

	for ( int i = 0; i < m_debugTarget.m_quantTableCount; ++i )
	{
		std::cout << "===============================================================================" << std::endl;
		std::cout << "quant table id - " << i;

		for ( int j = 0; j < JFIF::QUANT_TALBE_SIZE; ++j )
		{
			tempTable[JFIF::QTABLE_ORDER[j]] = m_debugTarget.m_quantTable[i][j];
		}

		for ( int j = 0; j < JFIF::QUANT_TALBE_SIZE; ++j )
		{
			if ( j % 8 == 0 )
			{
				std::cout << std::endl;
			}
			std::cout << tempTable[j] << '	';
		}
		std::cout << std::endl;
	}
}

void JFIFDebugger::PrintHuffmanTable( ) const
{
	JFIF::HuffmanTables& acTables = m_debugTarget.m_huffmanTable[JFIF::AC];

	std::cout << "===============================================================================" << std::endl;
	std::cout << "ac huffman table" << std::endl;

	for ( size_t i = 0; i < acTables.size(); ++i )
	{
		if ( acTables[i].empty( ) )
		{
			continue;
		}

		std::cout << "table id - " << i << std::endl;
		for ( const auto& code : acTables[i] )
		{
			std::cout << "code length - " << static_cast<int>( code.m_codeLength ) <<
						", range - " << std::bitset<16>( code.m_codeRange ) <<
						", byte - " << HEX( code.m_symbol ) << std::endl;
		}
		std::cout << "===============================================================================" << std::endl;
	}


	std::cout << "===============================================================================" << std::endl;
	std::cout << "dc huffman table" << std::endl;

	JFIF::HuffmanTables& dcTables = m_debugTarget.m_huffmanTable[JFIF::DC];

	for ( size_t i = 0; i < dcTables.size( ); ++i )
	{
		if ( dcTables[i].empty( ) )
		{
			continue;
		}

		std::cout << "table id - " << i << std::endl;
		for ( const auto& code : dcTables[i] )
		{
			std::cout << "code length - " << static_cast<int>( code.m_codeLength ) <<
				", range - " << std::bitset<16>( code.m_codeRange ) <<
				", byte - " << HEX( code.m_symbol ) << std::endl;
		}
		std::cout << "===============================================================================" << std::endl;
	}
}
