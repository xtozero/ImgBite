#pragma once
#include <array>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <utility>
#include <wrl\client.h>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class CD3D12Renderer
{
public:
	bool OnInitialize( );
	void OnDestory( );
	void OnRender( );

	bool CreateDeviceDependentResources( );
	bool CreateResolutionDependentResources( const std::pair<UINT, UINT>& resolution = {} );

private:
	void WaitForNextFrame( );
	void WaitForGPU( );

	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;

	static constexpr int MAX_FRAME_BUFFER = 2;
	std::array<ComPtr<ID3D12CommandAllocator>, MAX_FRAME_BUFFER> m_commandAlloc;
	std::array<ComPtr<ID3D12GraphicsCommandList>, MAX_FRAME_BUFFER> m_commandList;
	
	ComPtr<IDXGISwapChain4> m_swapChain;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	std::array<ComPtr<ID3D12Resource>, MAX_FRAME_BUFFER> m_backBuffers;
	ComPtr<ID3D12DescriptorHeap> m_rtViews;
	int m_rtvViewSize = 0;

	UINT m_curBufferIdx = 0;
	ComPtr<ID3D12Fence> m_fence;
	HANDLE m_fenceEvent = nullptr;
	std::array<UINT64, MAX_FRAME_BUFFER> m_fenceValue = {};

	RECT m_windowRect = {};
	D3D12_VIEWPORT m_viewport = {};

	D3D12_VERTEX_BUFFER_VIEW m_bufferView = {};
	ComPtr<ID3D12Resource> m_vertexBuffer;
};

