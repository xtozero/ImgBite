#pragma once

#include "../Image.h"

#include <fstream>

class NETPBM : public IMAGE
{
public:
	bool Load( const char* filePath );

private:
	void HandlePortableBitMap( std::ifstream& file );
	void HandlePortableGrayMap( std::ifstream& file );
	void HandlePortablePixMap( std::ifstream& file );

	void HandlePortableBinaryBitMap( std::ifstream& file );
	void HandlePortableBinaryGrayMap( std::ifstream& file );
	void HandlePortableBinaryPixMap( std::ifstream& file );
};

