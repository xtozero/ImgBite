#pragma once

#include <cmath>
#include <vector>

const double pi = std::acos( -1 );

template <typename T>
class IDCT
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
class FormulaIDCT
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