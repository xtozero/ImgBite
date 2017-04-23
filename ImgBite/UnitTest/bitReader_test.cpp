#include "Catch/include/catch.hpp"

#include "../ImgBite/BitReader.hpp"

TEST_CASE( "Test BitReader" )
{
	BYTE testBuffer[20] = { 0xF, 0xE, 0xD, 0xC, 0xB,
							0xA, 0x10, 0x9, 0x8, 0x7, 
							0x6, 0x5, 0x4, 0x3, 0x2, 
							0x1, 0x0, 0x1, 0x2, 0x3 };

	BitReader bt( testBuffer, 20 );
	REQUIRE( bt.ShowBit( 16 ) == 0x0F0E );

	bt.SkipBit( 16 );
	REQUIRE( bt.ShowBit( 7 ) == ( 0xD >> 1 ) );
	REQUIRE( bt.ShowBit( 6 ) == ( 0xD >> 2 ) );
	REQUIRE( bt.ShowBit( 5 ) == ( 0xD >> 3 ) );
	REQUIRE( bt.ShowBit( 4 ) == ( 0xD >> 4 ) );
	REQUIRE( bt.ShowBit( 3 ) == ( 0xD >> 5 ) );

	bt.SkipBit( 6 );
	REQUIRE( bt.ShowBit( 12 ) == 0x010C0 >> 2 );
}