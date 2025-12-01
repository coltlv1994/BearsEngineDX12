#pragma once
// D3D12 headers
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

#include <vector>
#include <map>

class Shader
{
public:
	Shader(const wchar_t* p_vsPath, const wchar_t* p_psPath);
	~Shader();
	ComPtr<ID3DBlob> GetVertexShaderBlob();
	ComPtr<ID3DBlob> GetPixelShaderBlob();
	ComPtr<ID3D12RootSignature> GetRootSigniture();
	ComPtr<ID3D12PipelineState> GetPipelineState();

	void CreateRootSignitureAndPipelineStream(D3D12_INPUT_ELEMENT_DESC* inputLayout_p, size_t inputLayoutCount);


private:
	ComPtr<ID3DBlob> m_vertexShaderBlob;
	ComPtr<ID3DBlob> m_pixelShaderBlob;
	// Root signature
	ComPtr<ID3D12RootSignature> m_RootSignature;
	// Pipeline state object.
	ComPtr<ID3D12PipelineState> m_PipelineState;
};