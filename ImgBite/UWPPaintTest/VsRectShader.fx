struct VERTEX_INPUT
{
	float3 m_position : POSITION;
	float2 m_texcoord : TEXCOORD;
};

struct VERTEX_OUTPUT
{
	float4 m_position : SV_POSITION;
	float2 m_texcoord : TEXCOORD;
};

VERTEX_OUTPUT vsMain( VERTEX_INPUT input )
{
	VERTEX_OUTPUT output = (VERTEX_OUTPUT)0;

	output.m_position = float4(input.m_position, 1.f);
	output.m_texcoord = input.m_texcoord;

	return output;
}