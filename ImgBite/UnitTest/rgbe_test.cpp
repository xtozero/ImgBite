#define _SCL_SECURE_NO_WARNINGS

#include "Catch/include/catch.hpp"

#include "../ImgBite/RGBE/RGBE.h"
#include "bitmap_image.hpp"

TEST_CASE( "Declare static lib RGBE class" )
{
	RGBE rgbe;
}

TEST_CASE( "Call static lib RGBE function" )
{
	RGBE rgbe;
	REQUIRE_FALSE( rgbe.Load( "file path" ) );
	REQUIRE( rgbe.GetHeight( ) == 0 );
	REQUIRE( rgbe.GetWidth( ) == 0 );
}

TEST_CASE( "Reinhard Tonemap" )
{
	RGBE rgbe;
	REQUIRE( rgbe.Load( "../Image/HDR/uffizi_probe.hdr" ) );

	REQUIRE( rgbe.GetWidth( ) == 1500 );
	REQUIRE( rgbe.GetHeight( ) == 1500 );

	rgbe.ReinhardToneMapping( 0.08f );

	unsigned int width = rgbe.GetWidth( );
	unsigned int height = rgbe.GetHeight( );

	bitmap_image toneMapImage( width, height );

	const BYTE* color = rgbe.GetByteStream( ).data( );
	for ( int y = 0; y < height; ++y )
	{
		for ( int x = 0; x < width; ++x )
		{
			toneMapImage.set_pixel( x, y, color[0], color[1], color[2] );
			color += 3;
		}
	}

	toneMapImage.save_image( "reinhardTonemap.bmp" );
}

TEST_CASE( "Reinhard Tonemap2" )
{
	RGBE rgbe;
	REQUIRE( rgbe.Load( "../Image/HDR/uffizi_probe.hdr" ) );

	REQUIRE( rgbe.GetWidth( ) == 1500 );
	REQUIRE( rgbe.GetHeight( ) == 1500 );

	rgbe.ReinhardToneMappingVer2( 0.08f );

	unsigned int width = rgbe.GetWidth( );
	unsigned int height = rgbe.GetHeight( );

	bitmap_image toneMapImage( width, height );

	const BYTE* color = rgbe.GetByteStream( ).data( );
	for ( int y = 0; y < height; ++y )
	{
		for ( int x = 0; x < width; ++x )
		{
			toneMapImage.set_pixel( x, y, color[0], color[1], color[2] );
			color += 3;
		}
	}

	toneMapImage.save_image( "reinhardTonemap2.bmp" );
}

#undef _SCL_SECURE_NO_WARNINGS