#include "../ImgBite/JFIF/JFIF.h"

#pragma comment( lib, "ImgBite.lib" )

int main( )
{
	for ( int i = 0; i < 100; ++i )
	{
		JFIF jpg;
		jpg.Load( "../Image/lena.jpg" );
	}

	return 0;
}