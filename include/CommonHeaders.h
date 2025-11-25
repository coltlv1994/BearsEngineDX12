#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> // For CommandLineToArgvW

#include <algorithm>
#include <exception>
#include <vector>

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#ifdef CreateWindow
#undef CreateWindow
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// D3D12 headers
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// The number of swap chain back buffers.
constexpr uint8_t NUM_OF_FRAMES = 3;

// Helper function
static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

typedef struct PipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
    CD3DX12_PIPELINE_STATE_STREAM_VS VS;
    CD3DX12_PIPELINE_STATE_STREAM_PS PS;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
} PipelineStateStream;

// Global function definition
void LoadOBJFile(
	const wchar_t* p_objFilePath,
	std::vector<float>& p_vertices,
	std::vector<float>& p_normals,
	std::vector<uint32_t>& p_triangles,
	std::vector<uint32_t>& p_triangleNormalIndex);