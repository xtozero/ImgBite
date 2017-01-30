#include "catch.hpp"

#include "../ImgBite/JFIF/JFIF.h"
#include "../ImgBite/JFIF/JFIFDebugger.h"

#include <WinSock2.h>

#pragma comment( lib, "ImgBite.lib" )
#pragma comment( lib, "Ws2_32.lib" )

TEST_CASE( "Declare static lib jfif class" )
{
	JFIF jfif;
}

TEST_CASE( "Call static lib jfif function" )
{
	JFIF jfif;
	REQUIRE_FALSE( jfif.Load( "file path" ) );
	REQUIRE( jfif.GetHeight( ) == 0 );
	REQUIRE( jfif.GetWidth( ) == 0 );
}

TEST_CASE( "JFIF Read" )
{
	JFIF jfif;
	REQUIRE( jfif.Load( "../Image/huff_simple.jpg" ) );

	REQUIRE( jfif.GetHeight( ) == 8 );
	REQUIRE( jfif.GetWidth( ) == 16 );
	REQUIRE( jfif.GetBytePerPixel( ) == 3 );
	REQUIRE( jfif.GetByteStream( ).size( ) == 8 * 16 * 3 );
}

TEST_CASE( "JFIF Quantization Table Parse" )
{
	JFIF jfif;
	REQUIRE( jfif.Load( "../Image/huff_simple.jpg" ) );

	JFIFDebugger debuger( jfif );
	debuger.PrintQuantTable();
}

TEST_CASE( "JFIF Huffman Table Parse" )
{
	JFIF jfif;
	REQUIRE( jfif.Load( "../Image/huff_simple.jpg" ) );

	JFIFDebugger debuger( jfif );
	debuger.PrintHuffmanTable( );
}

TEST_CASE( "Load All Test Image" )
{
	JFIF huff_simple;
	REQUIRE( huff_simple.Load( "../Image/huff_simple.jpg" ) );

	JFIF lena;
	REQUIRE( lena.Load( "../Image/lena.jpg" ) );

	JFIF test;
	REQUIRE( test.Load( "../Image/test.jpg" ) );

	JFIF uv;
	REQUIRE( uv.Load( "../Image/uv.jpg" ) );
}