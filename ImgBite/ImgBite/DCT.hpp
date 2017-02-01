#pragma once

#include <cmath>
#include <vector>

const double pi = std::acos( -1 );

template <typename T>
class IDCT final
{
public:
	static void DoRowIDCT( int* table );
	static void DoColIDCT( int* table );
};

template<typename T>
void IDCT<T>::DoRowIDCT( int* table )
{
	T::DoRowIDCT( table );
}

template<typename T>
void IDCT<T>::DoColIDCT( int* table )
{
	T::DoColIDCT( table );
}

template<int N>
class FormulaIDCT final
{
public:
	static void DoRowIDCT( int* table );
	static void DoColIDCT( int* table );

private:
	static const int DOUBLE_N = N * 2;

	static void DoIDCT1D( int* table, const int stride );
};

template<int N>
void FormulaIDCT<N>::DoRowIDCT( int* table )
{
	for ( int i = 0; i < N; ++i )
	{
		DoIDCT1D( table, 1 );
		table += N;
	}
}

template<int N>
void FormulaIDCT<N>::DoColIDCT( int* table )
{
	for ( int i = 0; i < N; ++i )
	{
		DoIDCT1D( table, N );
		++table;
	}
}

template<int N>
void FormulaIDCT<N>::DoIDCT1D( int* table, const int stride )
{
	std::vector<int> results;
	results.reserve( N );

	double halfN = sqrt( 2.f / N );
	double sum = 0;

	for ( int i = 0; i < N; ++i )
	{
		for ( int j = 0, k = 0; j < N; ++j, k += stride )
		{
			double c = ( j == 0 ) ? 1.f / sqrt(2) : 1.f;
			sum += c * table[k] * cos( ( 2.f * i + 1.f ) * pi * j / DOUBLE_N );
		}
		sum *= halfN;

		results.push_back( static_cast<int>( std::round( sum ) ) );
		sum = 0.;
	}

	int* dest = table;
	for ( auto idct : results )
	{
		*dest = idct;
		dest += stride;
	}
}

using FormulaIDCT8x8 = IDCT<FormulaIDCT<8>>;

template<int N>
class FastIDCT final
{
public:
	FastIDCT( )
	{
		static_assert(false, "Not implemented");
	}
};

template<>
class FastIDCT<8> final
{
public:
	static void DoRowIDCT( int* table );
	static void DoColIDCT( int* table );

private:
	static constexpr int W1 = 2841; /* 2048*sqrt(2)*cos(1*pi/16) */
	static constexpr int W2 = 2676; /* 2048*sqrt(2)*cos(2*pi/16) */
	static constexpr int W3 = 2408; /* 2048*sqrt(2)*cos(3*pi/16) */
	static constexpr int W5 = 1609; /* 2048*sqrt(2)*cos(5*pi/16) */
	static constexpr int W6 = 1108; /* 2048*sqrt(2)*cos(6*pi/16) */
	static constexpr int W7 = 565;  /* 2048*sqrt(2)*cos(7*pi/16) */

	static int Clip( int value )
	{
		return std::max( -128, std::min( 127, value ) );
	}

	static void DoIDCT1DRow( int* table );
	static void DoIDCT1DCol( int* table );
};

void FastIDCT<8>::DoRowIDCT( int* table )
{
	for ( int i = 0; i < 8; ++i )
	{
		DoIDCT1DRow( &table[i * 8] );
	}
}

void FastIDCT<8>::DoColIDCT( int* table )
{
	for ( int i = 0; i < 8; ++i )
	{
		DoIDCT1DCol( &table[i] );
	}
}

inline void FastIDCT<8>::DoIDCT1DRow( int* table )
{
	int x0, x1, x2, x3, x4, x5, x6, x7, x8;

	if ( !((x1 = table[4] << 11) | (x2 = table[6]) | (x3 = table[2]) |
		(x4 = table[1]) | (x5 = table[7]) | (x6 = table[5]) | (x7 = table[3])) )
	{
		table[0] = table[1] = table[2] = table[3] = table[4] = table[5] = table[6] = table[7] = table[0] << 3;
		return;
	}

	x0 = (table[0] << 11) + 128;

	// first
	x8 = W7 * (x4 + x5);
	x4 = x8 + (W1 - W7) * x4;
	x5 = x8 - (W1 + W7) * x5;
	x8 = W3 * (x6 + x7);
	x6 = x8 - (W3 - W5) * x6;
	x7 = x8 - (W3 + W5) * x7;

	// second
	x8 = x0 + x1;
	x0 -= x1;
	x1 = W6 * (x3 + x2);
	x2 = x1 - (W2 + W6) * x2;
	x3 = x1 + (W2 - W6) * x3;
	x1 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;

	// third
	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;

	// fourth
	table[0] = (x7 + x1) >> 8;
	table[1] = (x3 + x2) >> 8;
	table[2] = (x0 + x4) >> 8;
	table[3] = (x8 + x6) >> 8;
	table[4] = (x8 - x6) >> 8;
	table[5] = (x0 - x4) >> 8;
	table[6] = (x3 - x2) >> 8;
	table[7] = (x7 - x1) >> 8;
}

inline void FastIDCT<8>::DoIDCT1DCol( int* table )
{
	int x0, x1, x2, x3, x4, x5, x6, x7, x8;

	if ( !((x1 = (table[8 * 4] << 8)) | (x2 = table[8 * 6]) | (x3 = table[8 * 2]) |
		(x4 = table[8 * 1]) | (x5 = table[8 * 7]) | (x6 = table[8 * 5]) | (x7 = table[8 * 3])) )
	{
		x1 = Clip( (table[8 * 0] + 32) >> 6 );
		table[8 * 0] = table[8 * 1] = table[8 * 2] = table[8 * 3] = table[8 * 4] =
			table[8 * 5] = table[8 * 6] = table[8 * 7] = table[8 * 0] = x1;
		return;
	}

	x0 = (table[8 * 0] << 8) + 8192;

	// first
	x8 = W7 * (x4 + x5) + 4;
	x4 = (x8 + (W1 - W7) * x4) >> 3;
	x5 = (x8 - (W1 + W7) * x5) >> 3;
	x8 = W3 * (x6 + x7) + 4;
	x6 = (x8 - (W3 - W5) * x6) >> 3;
	x7 = (x8 - (W3 + W5) * x7) >> 3;

	// second
	x8 = x0 + x1;
	x0 -= x1;
	x1 = W6 * (x3 + x2) + 4;
	x2 = (x1 - (W2 + W6) * x2) >> 3;
	x3 = (x1 + (W2 - W6) * x3) >> 3;
	x1 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;

	// third
	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;

	// fourth
	table[8 * 0] = Clip( (x7 + x1) >> 14 );
	table[8 * 1] = Clip( (x3 + x2) >> 14 );
	table[8 * 2] = Clip( (x0 + x4) >> 14 );
	table[8 * 3] = Clip( (x8 + x6) >> 14 );
	table[8 * 4] = Clip( (x8 - x6) >> 14 );
	table[8 * 5] = Clip( (x0 - x4) >> 14 );
	table[8 * 6] = Clip( (x3 - x2) >> 14 );
	table[8 * 7] = Clip( (x7 - x1) >> 14 );
}

using FastIDCT8x8 = IDCT<FastIDCT<8>>;