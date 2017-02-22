#pragma once

#include <array>
#include <d3d12.h>
#include <DirectXMath.h>

using namespace DirectX;

template <typename T>
struct CVertex
{
	static void GetInputDesc( ) noexcept { static_assert( false, "Not implemented" ); }
};

template <>
struct CVertex<D3D12_INPUT_ELEMENT_DESC>
{
	XMFLOAT3 m_position;
	XMFLOAT2 m_uv;

	using InputElements = std::array<D3D12_INPUT_ELEMENT_DESC, 2>;
	static std::array<D3D12_INPUT_ELEMENT_DESC, 2> GetInputDesc( ) noexcept
	{
		return { {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } 
		} };
	}
};

using CDX12Vertex = CVertex<D3D12_INPUT_ELEMENT_DESC>;