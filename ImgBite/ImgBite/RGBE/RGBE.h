#pragma once

#include "../Image.h"

class RGBE : public IMAGE
{
public:
	bool Load( const char* filePath );
	void ReinhardToneMapping( float key );
	void ReinhardToneMappingVer2( float key );

private:
	struct RgbeHeaderInfo
	{
		int valid;
		char programtype[16];
		float gamma;
		float exposure;
	};

	enum RGBE_VALID_FLAG
	{
		RGBE_VALID_PROGRAMTYPE = 0x01,
		RGBE_VALID_GAMMA = 0x02,
		RGBE_VALID_EXPOSURE = 0x04
	};

	bool ReadHeader( std::ifstream& file );
	bool ReadPixel( std::ifstream& file );
	bool ReadPixelRunLength( std::ifstream& file );

	RgbeHeaderInfo m_header;
	std::vector<float> m_data;
};
