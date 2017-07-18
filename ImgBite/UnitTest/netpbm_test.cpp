#include "Catch/include/catch.hpp"

#include "../ImgBite/NETPBM/NETPBM.h"

TEST_CASE( "Declare static lib NETPBM class" )
{
	NETPBM netpbm;
}

TEST_CASE( "Call static lib NETPBM function" )
{
	NETPBM netpbm;
	REQUIRE_FALSE( netpbm.Load( "file path" ) );
	REQUIRE( netpbm.GetHeight( ) == 0 );
	REQUIRE( netpbm.GetWidth( ) == 0 );
}

TEST_CASE( "Read Portable BitMap" )
{
	NETPBM netpbm;
	REQUIRE( netpbm.Load( "../Image/NETPBM/elephant.pbm" ) );

	REQUIRE( netpbm.GetWidth( ) == 512 );
	REQUIRE( netpbm.GetHeight( ) == 512 );

	REQUIRE( netpbm.GetBytePerChannel( ) == 1 );
	REQUIRE( netpbm.GetBytePerPixel( ) == 1 );
	REQUIRE( netpbm.GetChannelDepth( ) == 1 );
}

TEST_CASE( "Read Portable GrayMap" )
{
	NETPBM netpbm;
	REQUIRE( netpbm.Load( "../Image/NETPBM/elephant.pgm" ) );

	REQUIRE( netpbm.GetWidth( ) == 512 );
	REQUIRE( netpbm.GetHeight( ) == 512 );

	REQUIRE( netpbm.GetBytePerChannel( ) == 1 );
	REQUIRE( netpbm.GetBytePerPixel( ) == 1 );
	REQUIRE( netpbm.GetChannelDepth( ) == 8 );
}

TEST_CASE( "Read Portable PixMap" )
{
	NETPBM netpbm;
	REQUIRE( netpbm.Load( "../Image/NETPBM/elephant.ppm" ) );

	REQUIRE( netpbm.GetWidth( ) == 512 );
	REQUIRE( netpbm.GetHeight( ) == 512 );

	REQUIRE( netpbm.GetBytePerChannel( ) == 1 );
	REQUIRE( netpbm.GetBytePerPixel( ) == 3 );
	REQUIRE( netpbm.GetChannelDepth( ) == 8 );
}