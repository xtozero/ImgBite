#include "catch.hpp"

#include "../ImgBite/PNG/PNG.h"

#include <WinSock2.h>

#pragma comment( lib, "ImgBite.lib" )
#pragma comment( lib, "Ws2_32.lib" )

TEST_CASE( "Declare static lib png class" )
{
	PNG png;
}

TEST_CASE( "Call static lib png function" )
{
	PNG png;
	REQUIRE_FALSE( png.Load( "file path" ) );
	REQUIRE( png.GetHeight( ) == 0 );
	REQUIRE( png.GetWidth( ) == 0 );
}

TEST_CASE( "Byte Array assign" )
{
	ByteArray<4> intergerType;
	intergerType = 4;

	REQUIRE( sizeof( intergerType ) == sizeof(int) );
	REQUIRE( intergerType.Get<int>( ) == 4 );

	ByteArray<4> longType;
	longType = 4L;

	REQUIRE( sizeof( longType ) == sizeof( long ) );
	REQUIRE( longType.Get<long>( ) == 4L );

	ByteArray<8> longlongType;
	longlongType = 4LL;

	REQUIRE( sizeof( longlongType ) == sizeof( long long ) );
	REQUIRE( longlongType.Get<long long>( ) == 4LL );

	ByteArray<4> floatType;
	floatType = 123.456f;

	REQUIRE( sizeof( floatType ) == sizeof( float ) );
	REQUIRE( floatType.Get<float>( ) == 123.456f );

	ByteArray<8> doubleType;
	doubleType = 9876.54321;

	REQUIRE( sizeof( doubleType ) == sizeof( double ) );
	REQUIRE( doubleType.Get<double>( ) == 9876.54321 );
}

TEST_CASE( "Network Order Compare" )
{
	unsigned __int64 target = 1234567;

	ByteArray<8> bytes;
	bytes = target;

	REQUIRE( bytes.Get<unsigned __int64>( ) == target );
	REQUIRE( bytes.GetNetworkOrder<unsigned __int64>( ) == htonll( target ) );
	REQUIRE( bytes.GetHostOrder<unsigned __int64>( ) == ntohll( target ) );
}

TEST_CASE( "PNG Read" )
{
	PNG png;
	REQUIRE( png.Load( "../Image/lena.png" ) );

	REQUIRE( png.GetWidth() == 512 );
	REQUIRE( png.GetHeight() == 512 );

	REQUIRE( png.GetBytePerChannel( ) == 1 );
	REQUIRE( png.GetBytePerPixel( ) == 3 );
	REQUIRE( png.GetChannelDepth( ) == 8 );
}

TEST_CASE( "Read Invalid PNG" )
{
	PNG png;
	REQUIRE_FALSE( png.Load( "../Image/lenna.png" ) );

	REQUIRE( png.GetWidth( ) == 0 );
	REQUIRE( png.GetHeight( ) == 0 );

	REQUIRE( png.GetBytePerChannel( ) == 0 );
	REQUIRE( png.GetBytePerPixel( ) == 0 );
	REQUIRE( png.GetChannelDepth( ) == 0 );
}

TEST_CASE( "Byte per Channel Calculate" )
{
	auto testFunc = []( unsigned char bitDepth )
	{
		return static_cast<unsigned char>(std::ceil( static_cast<float>(bitDepth) / 8 )); // byte per channel
	};

	REQUIRE( testFunc( 1 ) == 1 );
	REQUIRE( testFunc( 2 ) == 1 );
	REQUIRE( testFunc( 3 ) == 1 );
	REQUIRE( testFunc( 4 ) == 1 );
	REQUIRE( testFunc( 5 ) == 1 );
	REQUIRE( testFunc( 6 ) == 1 );
	REQUIRE( testFunc( 7 ) == 1 );
	REQUIRE( testFunc( 8 ) == 1 );

	REQUIRE( testFunc( 9 ) == 2 );
	REQUIRE( testFunc( 10 ) == 2 );
	REQUIRE( testFunc( 11 ) == 2 );
	REQUIRE( testFunc( 12 ) == 2 );
	REQUIRE( testFunc( 13 ) == 2 );
	REQUIRE( testFunc( 14 ) == 2 );
	REQUIRE( testFunc( 15 ) == 2 );
	REQUIRE( testFunc( 16 ) == 2 );
}