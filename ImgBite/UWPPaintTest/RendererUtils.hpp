#pragma once

#include <string>

template <typename Func, typename... Args>
decltype(auto) CallComFunc( Func&& function, Args&&... args )
{
	return CallComFuncIntenal( std::is_member_function_pointer<Func>( ), std::forward<Func>( function ), std::forward<Args>( args )... );
}

template <typename Func, typename... Args>
decltype(auto) CallComFuncIntenal( std::false_type, Func function, Args&&... args )
{
	HRESULT hr = static_cast<HRESULT>(function( std::forward<Args>( args )... ));
	return ( hr >= 0L );
}

template <typename Func, typename ClassType, typename... Args>
decltype(auto) CallComFuncIntenal( std::true_type, Func function, ClassType&& self, Args&&... args )
{
	HRESULT hr = static_cast<HRESULT>(((*self.Get( )).*function)(std::forward<Args>( args )...));
	return ( hr >= 0L );
}

static std::wstring g_wprocessPath;
static std::string g_processPath;

std::wstring GetFullPath( const std::wstring& fileName )
{
	if ( g_wprocessPath.size( ) == 0 )
	{
		wchar_t path[512];

		GetModuleFileName( nullptr, path, 512 );

		wchar_t* lastSlach = wcsrchr( path, L'\\' );
		if ( lastSlach )
		{
			*(lastSlach + 1) = L'\0';
		}

		g_wprocessPath = path;
	}

	return g_wprocessPath + fileName;
}

std::string GetFullPath( const std::string& fileName )
{
	if ( g_processPath.size( ) == 0 )
	{
		char path[512];

		GetModuleFileNameA( nullptr, path, 512 );

		char* lastSlach = strrchr( path, '\\' );
		if ( lastSlach )
		{
			*(lastSlach + 1) = '\0';
		}

		g_processPath = path;
	}

	return g_processPath + fileName;
}

UINT64 GetRequiredIntermediateSize( ID3D12Resource* pDestResource, UINT FirstSubresource, UINT NumSubresources )
{
	D3D12_RESOURCE_DESC desc = pDestResource->GetDesc( );
	UINT64 requiredSize = 0;
	
	{
		ComPtr<ID3D12Device> pDevice;
		pDestResource->GetDevice( IID_PPV_ARGS( &pDevice ) );
		pDevice->GetCopyableFootprints( &desc, FirstSubresource, NumSubresources, 0, nullptr, nullptr, nullptr, &requiredSize );
	}

	return requiredSize;
}

void MemcpySubresource( D3D12_MEMCPY_DEST* pDest, D3D12_SUBRESOURCE_DATA* pSrc, SIZE_T RowSizeInBytes, UINT NumRows, UINT NumSlices )
{
	for ( UINT z = 0; z < NumSlices; ++z )
	{
		BYTE* pDestSlice = reinterpret_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
		const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * z;
		for ( UINT y = 0; y < NumRows; ++y )
		{
			memcpy( pDestSlice + pDest->RowPitch * y, pSrcSlice + pSrc->RowPitch * y, RowSizeInBytes );
		}
	}
}

UINT64 UpdateSubresource( ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pDestResource, ID3D12Resource* pIntermediate, UINT FirstSubresource, UINT NumSubresources, UINT64 requiredSize, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts, UINT* pNumRows, UINT64* pRowSizesInBytes, D3D12_SUBRESOURCE_DATA* pSrcData )
{
	D3D12_RESOURCE_DESC intermediateDesc = pIntermediate->GetDesc( );
	D3D12_RESOURCE_DESC destDesc = pDestResource->GetDesc( );

	if ( intermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
		intermediateDesc.Width < requiredSize + pLayouts[0].Offset ||
		requiredSize >( SIZE_T ) - 1 ||
		(destDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && (FirstSubresource != 0 || NumSubresources != 1)) )
	{
		return 0;
	}

	BYTE* pData = nullptr;
	if ( FAILED( pIntermediate->Map( 0, nullptr, reinterpret_cast<void**>(&pData) ) ) )
	{
		return 0;
	}

	for ( UINT i = 0; i < NumSubresources; ++i )
	{
		if ( pRowSizesInBytes[i] > ( SIZE_T )-1 )
		{
			return 0;
		}

		D3D12_MEMCPY_DEST destData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, pLayouts[i].Footprint.RowPitch * pNumRows[i] };
		MemcpySubresource( &destData, &pSrcData[i], (SIZE_T)pRowSizesInBytes[i], pNumRows[i], pLayouts[i].Footprint.Depth );
	}
	pIntermediate->Unmap( 0, nullptr );

	if ( destDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER )
	{
		D3D12_BOX srcBox = {};
		srcBox.left = UINT( pLayouts[0].Offset );
		srcBox.right = UINT( pLayouts[0].Offset + pLayouts[0].Footprint.Width );

		pCmdList->CopyBufferRegion( pDestResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width );
	}
	else
	{
		for ( UINT i = 0; i < NumSubresources; ++i )
		{
			D3D12_TEXTURE_COPY_LOCATION dest = {};
			dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dest.pResource = pDestResource;
			dest.SubresourceIndex = i + FirstSubresource;	

			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.pResource = pIntermediate;
			src.PlacedFootprint = pLayouts[i];

			pCmdList->CopyTextureRegion( &dest, 0, 0, 0, &src, nullptr );
		}
	}

	return requiredSize;
}

UINT64 UpdateSubresource( ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pDestResource, ID3D12Resource* pIntermediate, UINT64 IntermediateOffset, UINT FirstSubresource, UINT NumSubresources, D3D12_SUBRESOURCE_DATA* pSrcData )
{
	UINT64 requiredSize = 0;
	UINT64 MemToAlloc = static_cast<UINT64>(sizeof( D3D12_PLACED_SUBRESOURCE_FOOTPRINT ) + sizeof( UINT ) + sizeof( UINT64 )) * NumSubresources;
	if ( MemToAlloc > SIZE_MAX )
	{
		return 0;
	}
	void* pMem = HeapAlloc( GetProcessHeap( ), 0, static_cast<SIZE_T>(MemToAlloc) );
	if ( pMem == nullptr )
	{
		return 0;
	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
	UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + NumSubresources);
	UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + NumSubresources);

	D3D12_RESOURCE_DESC desc = pDestResource->GetDesc( );
	{
		ComPtr<ID3D12Device> pDevice;
		pDestResource->GetDevice( IID_PPV_ARGS( &pDevice ) );
		pDevice->GetCopyableFootprints( &desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &requiredSize );
	}

	UINT64 result = UpdateSubresource( pCmdList, pDestResource, pIntermediate, FirstSubresource, NumSubresources, requiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData );
	HeapFree( GetProcessHeap( ), 0, pMem );
	return result;
}