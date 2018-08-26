#pragma once

#include <cassert>
#include <cmath>

inline void Rgbe2float( const unsigned char* rgbe, float* dest, float rcpExposure )
{
	float f;

	if ( rgbe[3] )
	{
		f = ldexpf( 1.0, rgbe[3] - static_cast<int>( 128 + 8 ) );
		dest[0] = rcpExposure * rgbe[0] * f;
		dest[1] = rcpExposure * rgbe[1] * f;
		dest[2] = rcpExposure * rgbe[2] * f;
	}
	else
	{
		dest[0] = dest[1] = dest[2] = 0.f;
	}
}

inline void sRGB2XYZ( const float* src, float* dest )
{
	assert( src != dest && "src and dest must have different address" );

	dest[0] = 0.4124564f * src[0] + 0.3575761f * src[1] + 0.1804375f * src[2]; // X
	dest[1] = 0.2126729f * src[0] + 0.7151522f * src[1] + 0.0721750f * src[2]; // Y
	dest[2] = 0.0193339f * src[0] + 0.1191920f * src[1] + 0.9503041f * src[2]; // Z
}

inline void XYZ2Yxy( const float* src, float* dest )
{
	assert( src != dest && "src and dest must have different address" );

	float denumerator = src[0] + src[1] + src[2];
	if ( denumerator == 0 )
	{
		dest[0] = dest[1] = dest[2] = 0.f;
	}
	else
	{
		dest[0] = src[1]; // Y
		dest[1] = src[0] / denumerator; // X / X + Y + Z
		dest[2] = src[1] / denumerator; // Y / X + Y + Z
	}
}

inline void Yxy2XYZ( const float* src, float* dest )
{
	assert( src != dest && "src and dest must have different address" );

	dest[1] = src[0]; // Y
	if ( src[0] > 0 && src[1] > 0 && src[2] > 0 )
	{
		float denumerator = src[0] / src[2];
		dest[0] = src[1] * denumerator; // X
		dest[2] = ( 1.f - src[1] - src[2] ) * denumerator; // Z
	}
	else
	{
		dest[0] = 0;
		dest[2] = 0;
	}
}

inline void XYZ2sRGB( const float* src, float* dest )
{
	assert( src != dest && "src and dest must have different address" );

	dest[0] = 3.2404542f * src[0] + -1.5371385f * src[1] + -0.4985314f * src[2]; // r
	dest[1] = -0.9692660f * src[0] + 1.8760108f * src[1] + 0.0415560f * src[2]; // g
	dest[2] = 0.0556434f * src[0] + -0.2040259f * src[1] + 1.0572252f * src[2]; // b
}

inline float ConvertsRGB2Luminance( const float* sRGB )
{
	return 0.2126729f * sRGB[0] + 0.7151522f * sRGB[1] + 0.0721750f * sRGB[2];
}

template <typename T>
inline T clamp( T value, T min, T max )
{
	return std::max( min, std::min( max, value ) );
}

// Fast but lossy
inline void YCbCr2RGBFast( int y, int cb, int cr, BYTE& r, BYTE& g, BYTE& b )
{
	y &= 0xff, cb &= 0xff, cr &= 0xff;
	cb -= 128;
	cr -= 128;

	r = static_cast<BYTE>( clamp( y + ( ( 91881 * cr ) >> 16 ), 0, 255 ) );
	g = static_cast<BYTE>( clamp( y - ( ( 22554 * cb ) >> 16 ) - ( ( 46802 * cr ) >> 16 ), 0, 255 ) );
	b = static_cast<BYTE>( clamp( y + ( ( 116130 * cb ) >> 16 ), 0, 255 ) );
}

// https://en.wikipedia.org/wiki/YCbCr#JPEG_conversion
inline void YCbCr2RGB( int y, int cb, int cr, BYTE& r, BYTE& g, BYTE& b )
{
	r = static_cast<BYTE>( clamp<double>( round( 1.402 * ( cr - 128 ) + y ), 0, 255 ) );
	g = static_cast<BYTE>( clamp<double>( round( -0.34414 * ( cb - 128 ) - 0.71414 * ( cr - 128 ) + y ), 0, 255 ) );
	b = static_cast<BYTE>( clamp<double>( round( 1.772 * ( cb - 128 ) + y ), 0, 255 ) );
}