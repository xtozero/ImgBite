Texture2D baseTexture : register(t0);
SamplerState baseTextureSampler : register(s0);

struct PIXEL_INPUT
{
	float4 m_position : SV_POSITION;
	float2 m_texcoord : TEXCOORD;
};

float4 psMain( PIXEL_INPUT input ) : SV_TARGET
{
	float4 color = baseTexture.Sample( baseTextureSampler, input.m_texcoord );
	return color;
}