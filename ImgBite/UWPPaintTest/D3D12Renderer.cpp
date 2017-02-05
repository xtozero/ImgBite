#include "pch.h"
#include "D3D12Renderer.h"

#include <functional>

template <typename Func, typename... Args>
decltype(auto) CallComFunc( Func&& function, Args&&... args )
{
	return CallComFuncIntenal( std::is_member_function_pointer<Func>( ), std::forward<Func>( function ), std::forward<Args>( args )... );
}

template <typename Func, typename... Args>
decltype(auto) CallComFuncIntenal( std::false_type, Func function, Args&&... args )
{
	return ( static_cast<HRESULT>( function( std::forward<Args>( args )... ) ) >= 0 );
}

template <typename Func, typename ClassType, typename... Args>
decltype(auto) CallComFuncIntenal( std::true_type, Func function, ClassType&& self, Args&&... args )
{
	return ( static_cast<HRESULT>( ( (*self.Get()).*function )(std::forward<Args>( args )...) ) >= 0 );
}

bool CD3D12Renderer::OnInitialize( )
{
	if ( !CallComFunc( D3D12CreateDevice, nullptr, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS( &m_device ) ) )
	{
		return false;
	}

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

		if ( !CallComFunc( &IDXGIFactory5::CreateSwapChainForCoreWindow,
							factory,
							m_commandQueue.Get(),
							reinterpret_cast<IUnknown*>(Windows::UI::Core::CoreWindow::GetForCurrentThread()),
							&swapChainDesc,
							nullptr,
							&swapChain) )
		{
			return false;
		}

		if ( swapChain.As( &m_swapChain ) )
		{
			return false;
		}

		m_frameIdx = m_swapChain->GetCurrentBackBufferIndex( );

		D3D12_DESCRIPTOR_HEAP_DESC rtvheapDesc = {};
		rtvheapDesc.NumDescriptors = MAX_FRAME_BUFFER;
		rtvheapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

		if ( !CallComFunc( &ID3D12Device::CreateDescriptorHeap, m_device, &rtvheapDesc, IID_PPV_ARGS( &m_rtViews ) ) )
		{
			return false;
		}

		m_rtvViewSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtViews->GetCPUDescriptorHandleForHeapStart( );

		for ( int i = 0; i < MAX_FRAME_BUFFER; ++i )
		{
			if ( !CallComFunc( &IDXGISwapChain4::GetBuffer, m_swapChain, i, IID_PPV_ARGS( &m_frameBuffers[i] ) ) )
			{
				return false;
			}

			m_device->CreateRenderTargetView( m_frameBuffers[i].Get( ), nullptr, rtvHandle );
			rtvHandle.ptr += m_rtvViewSize;
		}
	}

	{
		ComPtr<ID3D10Blob> signature;
		
		D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
		signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		if ( !CallComFunc( D3D12SerializeRootSignature, &signatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr ) )
		{
			return false;
		}

		if ( !CallComFunc( &ID3D12Device::CreateRootSignature, m_device, 0, signature->GetBufferPointer( ), signature->GetBufferSize( ), IID_PPV_ARGS( &m_rootSignature ) ) )
		{
			return false;
		}

		D3D12_INPUT_ELEMENT_DESC inputElements[] = {
			{},
			{} };
	}

	m_fenceEvent = CreateEvent( nullptr, false, false, L"Fence Evnet" );

	if ( !CallComFunc( &ID3D12Device::CreateFence, m_device, m_fenceValue[m_frameIdx], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &m_fence ) ) )
	{
		return false;
	}

	++m_fenceValue[m_frameIdx];

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

	if ( !CallComFunc( &ID3D12CommandAllocator::Reset, m_commandAlloc[m_frameIdx] ) )
	{
		return;
	}

	if ( !CallComFunc( &ID3D12GraphicsCommandList::Reset, m_commandList[m_frameIdx], m_commandAlloc[m_frameIdx].Get( ), nullptr ) )
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtViews->GetCPUDescriptorHandleForHeapStart( );
	rtvHandle.ptr += m_frameIdx * m_rtvViewSize;

	D3D12_RESOURCE_BARRIER rtvBarrier = {};
	rtvBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	rtvBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	rtvBarrier.Transition.pResource = m_frameBuffers[m_frameIdx].Get();
	rtvBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	rtvBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	rtvBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	float color[4] = { 1.f, 0.f, 0.f, 0.f };
	m_commandList[m_frameIdx]->ClearRenderTargetView( rtvHandle, color, 0, nullptr );

	D3D12_RESOURCE_BARRIER presentBarrier = {};
	presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	presentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	presentBarrier.Transition.pResource = m_frameBuffers[m_frameIdx].Get( );
	presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	if ( !CallComFunc( &ID3D12GraphicsCommandList::Close, m_commandList[m_frameIdx] ) )
	{
		return;
	}

	ID3D12CommandList* commandList[] = { m_commandList[m_frameIdx].Get( ) };
	m_commandQueue->ExecuteCommandLists( 1, commandList );

	if ( !CallComFunc( &IDXGISwapChain4::Present, m_swapChain, 0, 0 ) )
	{
		return;
	}
}

void CD3D12Renderer::WaitForNextFrame( )
{
	UINT64 currentValue = m_fenceValue[m_frameIdx];

	if ( !CallComFunc( &ID3D12CommandQueue::Signal, m_commandQueue, m_fence.Get( ), currentValue ) )
	{
		return;
	}

	if ( currentValue < m_fence->GetCompletedValue( ) )
	{
		if ( !CallComFunc( &ID3D12Fence::SetEventOnCompletion, m_fence, currentValue, m_fenceEvent ) )
		{
			return;
		}
		WaitForSingleObject( m_fenceEvent, INFINITE );
	}

	m_frameIdx = m_swapChain->GetCurrentBackBufferIndex( );
	m_fenceValue[m_frameIdx] = currentValue + 1;
}

void CD3D12Renderer::WaitForGPU( )
{
	if ( !CallComFunc( &ID3D12CommandQueue::Signal, m_commandQueue, m_fence.Get( ), m_fenceValue[m_frameIdx] ) )
	{
		return;
	}

	if ( !CallComFunc( &ID3D12Fence::SetEventOnCompletion, m_fence, m_fenceValue[m_frameIdx], m_fenceEvent ) )
	{
		return;
	}
	WaitForSingleObject( m_fenceEvent, INFINITE );

	++m_fenceValue[m_frameIdx];
}