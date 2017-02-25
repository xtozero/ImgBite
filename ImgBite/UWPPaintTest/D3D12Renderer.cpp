#include "pch.h"
#include "D3D12Renderer.h"

#include "../ImgBite/PNG/PNG.h"

#include "RendererUtils.hpp"

#include "VertexDefines.h"

#include <d3dcompiler.h>
#include <string>

#pragma comment( lib, "ImgBite.lib" )

bool CD3D12Renderer::OnInitialize( const std::pair<UINT, UINT>& resolution )
{
	if ( !CreateDeviceDependentResources( ) )
	{
		return false;
	}

	if ( !CreateResolutionDependentResources( resolution ) )
	{
		return false;
	}

	{
		D3D12_DESCRIPTOR_RANGE descRanges[2] = {};

		descRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descRanges[0].NumDescriptors = 1;
		descRanges[0].BaseShaderRegister = 0;
		descRanges[0].RegisterSpace = 0;
		descRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		descRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		descRanges[1].NumDescriptors = 1;
		descRanges[1].BaseShaderRegister = 0;
		descRanges[1].RegisterSpace = 0;
		descRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rootParam[2] = {};
		rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParam[0].DescriptorTable.pDescriptorRanges = &descRanges[0];
		rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParam[1].DescriptorTable.pDescriptorRanges = &descRanges[1];
		rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		ComPtr<ID3D10Blob> signature;

		D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
		signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		signatureDesc.NumParameters = _countof( rootParam );
		signatureDesc.pParameters = rootParam;

		if ( !CallComFunc( D3D12SerializeRootSignature, &signatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr ) )
		{
			return false;
		}

		if ( !CallComFunc( &ID3D12Device::CreateRootSignature, m_device, 0, signature->GetBufferPointer( ), signature->GetBufferSize( ), IID_PPV_ARGS( &m_rootSignature ) ) )
		{
			return false;
		}

		ComPtr<ID3D10Blob> vs = CompileShader( GetFullPath( L"VsRectShader.fx" ), "vsMain", "vs_5_0" );
		ComPtr<ID3D10Blob> ps = CompileShader( GetFullPath( L"PsRectShader.fx" ), "psMain", "ps_5_0" );

		std::array<D3D12_INPUT_ELEMENT_DESC, 2> inputElements = CDX12Vertex::GetInputDesc( );

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
		pipelineDesc.pRootSignature = m_rootSignature.Get( );
		pipelineDesc.VS.pShaderBytecode = vs->GetBufferPointer( );
		pipelineDesc.VS.BytecodeLength = vs->GetBufferSize( );
		pipelineDesc.PS.pShaderBytecode = ps->GetBufferPointer( );
		pipelineDesc.PS.BytecodeLength = ps->GetBufferSize( );
		pipelineDesc.BlendState.AlphaToCoverageEnable = false;
		pipelineDesc.BlendState.IndependentBlendEnable = false;
		pipelineDesc.BlendState.RenderTarget[0].BlendEnable = false;
		pipelineDesc.BlendState.RenderTarget[0].LogicOpEnable = false;
		pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		pipelineDesc.SampleMask = 0xFFFFFFFF;
		pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		pipelineDesc.DepthStencilState.DepthEnable = false;
		pipelineDesc.DepthStencilState.StencilEnable = false;
		pipelineDesc.InputLayout.NumElements = static_cast<UINT>( inputElements.size() );
		pipelineDesc.InputLayout.pInputElementDescs = inputElements.data();
		pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineDesc.NumRenderTargets = 1;
		pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		pipelineDesc.SampleDesc.Count = 1;
		pipelineDesc.SampleDesc.Quality = 0;
		pipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		if ( !CallComFunc( &ID3D12Device::CreateGraphicsPipelineState, m_device, &pipelineDesc, IID_PPV_ARGS( &m_pipelineState ) ) )
		{
			return false;
		}

		CDX12Vertex vertices[] = {
			{ { 1.f, 1.f, 0.f }, { 1.f, 0.f } },
			{ { 1.f, -1.f, 0.f }, { 1.f, 1.f } },
			{ { -1.f, 1.f, 0.f }, { 0.f, 0.f } },

			{ { -1.f, -1.f, 0.f }, { 0.f, 1.f } },
			{ { -1.f, 1.f, 0.f }, { 0.f, 0.f } },
			{ { 1.f, -1.f, 0.f }, { 1.f, 1.f } },
		};

		D3D12_HEAP_PROPERTIES heapProperty = {};
		heapProperty.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperty.VisibleNodeMask = 1;
		heapProperty.CreationNodeMask = 1;

		D3D12_RESOURCE_DESC rsrcDesc = {};
		rsrcDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		rsrcDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		rsrcDesc.Width = sizeof( vertices );
		rsrcDesc.Height = 1;
		rsrcDesc.DepthOrArraySize = 1;
		rsrcDesc.MipLevels = 1;
		rsrcDesc.Format = DXGI_FORMAT_UNKNOWN;
		rsrcDesc.SampleDesc.Count = 1;
		rsrcDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		rsrcDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		if ( !CallComFunc( &ID3D12Device::CreateCommittedResource,
							m_device,
							&heapProperty,
							D3D12_HEAP_FLAG_NONE,
							&rsrcDesc,
							D3D12_RESOURCE_STATE_GENERIC_READ,
							nullptr,
							IID_PPV_ARGS( &m_vertexBuffer ) ) )
		{
			return false;
		}

		D3D12_RANGE range = { 0, 0 };
		void* pData = nullptr;

		if ( !CallComFunc( &ID3D12Resource::Map, m_vertexBuffer, 0, &range, &pData ) )
		{
			return false;
		}
		memcpy( pData, static_cast<void*>(vertices), sizeof( vertices ) );
		m_vertexBuffer->Unmap( 0, nullptr );

		m_vertexBufferView.StrideInBytes = sizeof( CDX12Vertex );
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.SizeInBytes = sizeof( vertices );
	}

	if ( !CreateAssets( ) )
	{
		return false;
	}

	WaitForGPU( );

	return true;
}

void CD3D12Renderer::OnDestory( )
{
	WaitForGPU( );
	CloseHandle( m_fenceEvent );
}

void CD3D12Renderer::OnRender( )
{
	WaitForNextFrame( );

	if ( !CallComFunc( &ID3D12CommandAllocator::Reset, m_commandAlloc[m_curBufferIdx] ) )
	{
		return;
	}

	if ( !CallComFunc( &ID3D12GraphicsCommandList::Reset, m_commandList[m_curBufferIdx], m_commandAlloc[m_curBufferIdx].Get( ), m_pipelineState.Get() ) )
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtViews->GetCPUDescriptorHandleForHeapStart( );
	rtvHandle.ptr += m_curBufferIdx * m_rtvViewSize;

	D3D12_RESOURCE_BARRIER rtvBarrier = {};
	rtvBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	rtvBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	rtvBarrier.Transition.pResource = m_backBuffers[m_curBufferIdx].Get();
	rtvBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	rtvBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	rtvBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandList[m_curBufferIdx]->ResourceBarrier( 1, &rtvBarrier );

	m_commandList[m_curBufferIdx]->SetGraphicsRootSignature( m_rootSignature.Get( ) );
	m_commandList[m_curBufferIdx]->SetPipelineState( m_pipelineState.Get( ) );

	ID3D12DescriptorHeap* ppHeaps[] = { m_shaderResourceView.Get( ), m_sampler.Get( ) };
	m_commandList[m_curBufferIdx]->SetDescriptorHeaps( _countof( ppHeaps ), ppHeaps );
	m_commandList[m_curBufferIdx]->SetGraphicsRootDescriptorTable( 0, m_shaderResourceView->GetGPUDescriptorHandleForHeapStart( ) );
	m_commandList[m_curBufferIdx]->SetGraphicsRootDescriptorTable( 1, m_sampler->GetGPUDescriptorHandleForHeapStart( ) );

	m_commandList[m_curBufferIdx]->OMSetRenderTargets( 1, &rtvHandle, false, nullptr );

	float color[4] = { 1.f, 0.f, 0.f, 0.f };
	m_commandList[m_curBufferIdx]->ClearRenderTargetView( rtvHandle, color, 0, nullptr );
	m_commandList[m_curBufferIdx]->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	m_commandList[m_curBufferIdx]->IASetVertexBuffers( 0, 1, &m_vertexBufferView );

	m_commandList[m_curBufferIdx]->RSSetViewports( 1, &m_viewport );
	m_commandList[m_curBufferIdx]->RSSetScissorRects( 1, &m_windowRect );

	m_commandList[m_curBufferIdx]->DrawInstanced( 6, 1, 0, 0 );

	D3D12_RESOURCE_BARRIER presentBarrier = {};
	presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	presentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	presentBarrier.Transition.pResource = m_backBuffers[m_curBufferIdx].Get( );
	presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandList[m_curBufferIdx]->ResourceBarrier( 1, &presentBarrier );

	if ( !CallComFunc( &ID3D12GraphicsCommandList::Close, m_commandList[m_curBufferIdx] ) )
	{
		return;
	}

	ID3D12CommandList* commandList[] = { m_commandList[m_curBufferIdx].Get( ) };
	m_commandQueue->ExecuteCommandLists( 1, commandList );

	if ( !CallComFunc( &IDXGISwapChain::Present, m_swapChain, 1, 0 ) )
	{
		return;
	}
}

bool CD3D12Renderer::CreateDeviceDependentResources( )
{
	if ( !CallComFunc( D3D12CreateDevice, nullptr, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS( &m_device ) ) )
	{
		return false;
	}

	m_rtvViewSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

	for ( int i = 0; i < MAX_FRAME_BUFFER; ++i )
	{
		if ( !CallComFunc( &ID3D12Device::CreateCommandAllocator, m_device, D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &m_commandAlloc[i] ) ) )
		{
			return false;
		}
	}

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	if ( !CallComFunc( &ID3D12Device::CreateCommandQueue, m_device, &queueDesc, IID_PPV_ARGS( &m_commandQueue ) ) )
	{
		return false;
	}


	m_fenceEvent = CreateEvent( nullptr, false, false, L"Fence Evnet" );

	if ( !CallComFunc( &ID3D12Device::CreateFence, m_device, m_fenceValue[m_curBufferIdx], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &m_fence ) ) )
	{
		return false;
	}

	++m_fenceValue[m_curBufferIdx];

	for ( int i = 0; i < MAX_FRAME_BUFFER; ++i )
	{
		if ( !CallComFunc( &ID3D12Device::CreateCommandList,
			m_device,
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_commandAlloc[i].Get( ),
			nullptr,
			IID_PPV_ARGS( &m_commandList[i] ) ) )
		{
			return false;
		}

		if ( !CallComFunc( &ID3D12GraphicsCommandList::Close, m_commandList[i] ) )
		{
			return false;
		}
	}

	D3D12_DESCRIPTOR_HEAP_DESC rtvheapDesc = {};
	rtvheapDesc.NumDescriptors = MAX_FRAME_BUFFER;
	rtvheapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	if ( !CallComFunc( &ID3D12Device::CreateDescriptorHeap, m_device, &rtvheapDesc, IID_PPV_ARGS( &m_rtViews ) ) )
	{
		return false;
	}

	return true;
}

bool CD3D12Renderer::CreateResolutionDependentResources( const std::pair<UINT, UINT>& resolution )
{
	WaitForGPU( );

	for ( int i = 0; i < MAX_FRAME_BUFFER; ++i )
	{
		m_backBuffers[i] = nullptr;
	}

	if ( m_swapChain )
	{
		if ( !CallComFunc( &IDXGISwapChain::ResizeBuffers, m_swapChain, MAX_FRAME_BUFFER, resolution.first, resolution.second, DXGI_FORMAT_R8G8B8A8_UNORM, 0 ) )
		{
			return false;
		}
	}
	else
	{
		ComPtr<IDXGIFactory5> factory;
		if ( !CallComFunc( CreateDXGIFactory1, IID_PPV_ARGS( &factory ) ) )
		{
			return false;
		}

		ComPtr<IDXGISwapChain1> swapChain;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = MAX_FRAME_BUFFER;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

		if ( !CallComFunc( &IDXGIFactory2::CreateSwapChainForCoreWindow,
			factory,
			m_commandQueue.Get( ),
			reinterpret_cast<IUnknown*>(Windows::UI::Core::CoreWindow::GetForCurrentThread( )),
			&swapChainDesc,
			nullptr,
			&swapChain ) )
		{
			return false;
		}

		if ( swapChain.As( &m_swapChain ) )
		{
			return false;
		}
	}

	m_curBufferIdx = m_swapChain->GetCurrentBackBufferIndex( );
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtViews->GetCPUDescriptorHandleForHeapStart( );
	for ( int i = 0; i < MAX_FRAME_BUFFER; ++i )
	{
		if ( !CallComFunc( &IDXGISwapChain::GetBuffer, m_swapChain, i, IID_PPV_ARGS( &m_backBuffers[i] ) ) )
		{
			return false;
		}

		m_device->CreateRenderTargetView( m_backBuffers[i].Get( ), nullptr, rtvHandle );
		rtvHandle.ptr += m_rtvViewSize;
	}

	m_windowRect = { 0L, 0L, static_cast<long>( resolution.first ), static_cast<long>( resolution.second ) };
	m_viewport = { 0.f, 0.f, static_cast<float>( resolution.first ), static_cast<float>( resolution.second ), 0.f, 1.0f };

	return true;
}

void CD3D12Renderer::WaitForNextFrame( )
{
	UINT64 currentValue = m_fenceValue[m_curBufferIdx];

	if ( !CallComFunc( &ID3D12CommandQueue::Signal, m_commandQueue, m_fence.Get( ), currentValue ) )
	{
		return;
	}

	m_curBufferIdx = m_swapChain->GetCurrentBackBufferIndex( );
	if ( m_fence->GetCompletedValue() < m_fenceValue[m_curBufferIdx] )
	{
		if ( !CallComFunc( &ID3D12Fence::SetEventOnCompletion, m_fence, currentValue, m_fenceEvent ) )
		{
			return;
		}
		WaitForSingleObject( m_fenceEvent, INFINITE );
	}

	m_fenceValue[m_curBufferIdx] = currentValue + 1;
}

void CD3D12Renderer::WaitForGPU( )
{
 	if ( !CallComFunc( &ID3D12CommandQueue::Signal, m_commandQueue, m_fence.Get( ), m_fenceValue[m_curBufferIdx] ) )
	{
		return;
	}

	if ( !CallComFunc( &ID3D12Fence::SetEventOnCompletion, m_fence, m_fenceValue[m_curBufferIdx], m_fenceEvent ) )
	{
		return;
	}
	WaitForSingleObject( m_fenceEvent, INFINITE );

	++m_fenceValue[m_curBufferIdx];
}

bool CD3D12Renderer::CreateAssets( )
{
	PNG testPng;
	if ( !testPng.Load( GetFullPath( "lena.png" ).c_str( ) ) )
	{
		return false;
	}

	D3D12_HEAP_PROPERTIES heapProperty = {};
	heapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperty.CreationNodeMask = 1;
	heapProperty.VisibleNodeMask = 1;
	heapProperty.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC rsrcDesc = {};
	rsrcDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rsrcDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	rsrcDesc.Width = testPng.GetWidth();
	rsrcDesc.Height = testPng.GetHeight();
	rsrcDesc.DepthOrArraySize = 1;
	rsrcDesc.MipLevels = 1;
	rsrcDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rsrcDesc.SampleDesc.Count = 1;
	rsrcDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rsrcDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	if ( !CallComFunc( &ID3D12Device::CreateCommittedResource,
		m_device,
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&rsrcDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS( &m_shaderResource ) ) )
	{
		return false;
	}

	heapProperty.Type = D3D12_HEAP_TYPE_UPLOAD;

	rsrcDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rsrcDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	rsrcDesc.Width = GetRequiredIntermediateSize( m_shaderResource.Get(), 0, 1 );
	rsrcDesc.Height = 1;
	rsrcDesc.DepthOrArraySize = 1;
	rsrcDesc.MipLevels = 1;
	rsrcDesc.Format = DXGI_FORMAT_UNKNOWN;
	rsrcDesc.SampleDesc.Count = 1;
	rsrcDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	rsrcDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ComPtr<ID3D12Resource> buffer;

	if ( !CallComFunc( &ID3D12Device::CreateCommittedResource,
		m_device,
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&rsrcDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &buffer ) ) )
	{
		return false;
	}

	std::vector<unsigned char> texture;
	texture.resize( testPng.GetWidth( ) * testPng.GetHeight() * 4 );
	const std::vector<unsigned char>& colors = testPng.GetByteStream( );

	for ( int i = 0, j = -1; i < texture.size( ); i += 4 )
	{
		texture[i] = colors[++j];
		texture[i + 1] = colors[++j];
		texture[i + 2] = colors[++j];
		texture[i + 3] = 0xFF;
	}

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = texture.data( );
	textureData.RowPitch = 4 * testPng.GetWidth( );
	textureData.SlicePitch = textureData.RowPitch * testPng.GetHeight( );

	if ( !CallComFunc( &ID3D12CommandAllocator::Reset, m_commandAlloc[m_curBufferIdx] ) )
	{
		return false;
	}

	if ( !CallComFunc( &ID3D12GraphicsCommandList::Reset, m_commandList[m_curBufferIdx], m_commandAlloc[m_curBufferIdx].Get( ), m_pipelineState.Get( ) ) )
	{
		return false;
	}

	UpdateSubresource( m_commandList[m_curBufferIdx].Get( ),
						m_shaderResource.Get( ),
						buffer.Get( ),
						0,
						0,
						1,
						&textureData );

	D3D12_RESOURCE_BARRIER srBarrier = {};
	srBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	srBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	srBarrier.Transition.pResource = m_shaderResource.Get( );
	srBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	srBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	srBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandList[m_curBufferIdx]->ResourceBarrier( 1, &srBarrier );

	if ( !CallComFunc( &ID3D12GraphicsCommandList::Close, m_commandList[m_curBufferIdx] ) )
	{
		return false;
	}

	ID3D12CommandList* commandList[] = { m_commandList[m_curBufferIdx].Get( ) };
	m_commandQueue->ExecuteCommandLists( 1, commandList );

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.NodeMask = 0;

	if ( !CallComFunc( &ID3D12Device::CreateDescriptorHeap, m_device, &srvHeapDesc, IID_PPV_ARGS( &m_shaderResourceView ) ) )
	{
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_device->CreateShaderResourceView( m_shaderResource.Get( ), &srvDesc, m_shaderResourceView->GetCPUDescriptorHandleForHeapStart( ) );

	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
	samplerHeapDesc.NumDescriptors = 1;
	samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerHeapDesc.NodeMask = 0;
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	if ( !CallComFunc( &ID3D12Device::CreateDescriptorHeap, m_device, &samplerHeapDesc, IID_PPV_ARGS( &m_sampler ) ) )
	{
		return false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE samperHandle = m_sampler->GetCPUDescriptorHandleForHeapStart( );
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.BorderColor[0] = 1.f;
	samplerDesc.BorderColor[1] = 1.f;
	samplerDesc.BorderColor[2] = 1.f;
	samplerDesc.BorderColor[3] = 1.f;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.MaxLOD = FLT_MAX;
	samplerDesc.MinLOD = -FLT_MAX;
	samplerDesc.MipLODBias = 0.f;

	m_device->CreateSampler( &samplerDesc, samperHandle );

	WaitForGPU( );
	return true;
}

ComPtr<ID3D10Blob> CD3D12Renderer::CompileShader( std::wstring fileName, std::string entryPoint, std::string profile )
{
	ComPtr<ID3D10Blob> byteCode;

	CallComFunc( &D3DCompileFromFile, fileName.c_str( ), nullptr, nullptr, entryPoint.c_str( ), profile.c_str( ), 0, 0, &byteCode, nullptr );

	return byteCode;
}
