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
	Shader();
	~Shader();

	void GetRSAndPSO_1stPass(ComPtr<ID3D12RootSignature>& rootSignature, ComPtr<ID3D12PipelineState>& pipelineState)
	{
		rootSignature = m_1stPassRootSignature;
		pipelineState = m_1stPassPipelineState;
	}

	void GetRSAndPSO_2ndPass(ComPtr<ID3D12RootSignature>& rootSignature, ComPtr<ID3D12PipelineState>& pipelineState)
	{
		rootSignature = m_2ndPassRootSignature;
		pipelineState = m_2ndPassPipelineState;
	}

	void CreateRSAndPSO(const wchar_t* p_1stVsPath, const wchar_t* p_1sPsPath, const wchar_t* p_2ndVsPath, const wchar_t* p_2ndPsPath);


private:
	ComPtr<ID3DBlob> m_1stPassVertexShaderBlob;
	ComPtr<ID3DBlob> m_1stPassPixelShaderBlob;
	ComPtr<ID3DBlob> m_2ndPassVertexShaderBlob;
	ComPtr<ID3DBlob> m_2ndPassPixelShaderBlob;

	// Root signature
	ComPtr<ID3D12RootSignature> m_1stPassRootSignature;
	ComPtr<ID3D12RootSignature> m_2ndPassRootSignature;
	// Pipeline state object.
	ComPtr<ID3D12PipelineState> m_1stPassPipelineState;
	ComPtr<ID3D12PipelineState> m_2ndPassPipelineState;

	void _create1st();
	void _create2nd();
};