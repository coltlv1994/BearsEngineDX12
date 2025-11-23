#pragma once
#include <CommonHeaders.h>
#include <vector>
#include <map>

class Shader
{
public:
	Shader(const wchar_t* p_vsPath, const wchar_t* p_psPath, D3D12_INPUT_ELEMENT_DESC* p_inputLayout_p, size_t p_inputLayoutCount);
	~Shader();

	D3D12_INPUT_ELEMENT_DESC& GetInputLayout();
	size_t GetInputLayoutCount();
	const void* GetVsByteCode();
	const void* GetPsByteCode();

	bool PopulateRootConstants(std::vector<size_t>& p_constantSize, D3D12_SHADER_VISIBILITY p_enumVisibility);
	void SetRootConstants();
	void Create();
	ComPtr<ID3D12PipelineState> GetPipelineState()
	{
		return m_pipelineState;
	}

	ComPtr<ID3D12RootSignature> GetRootSigniture()
	{
		return m_rootSignature;
	}

private:
	ComPtr<ID3DBlob> m_vertexShaderBlob;
	ComPtr<ID3DBlob> m_pixelShaderBlob;
	D3D12_INPUT_ELEMENT_DESC* m_inputLayout_p;
	size_t m_inputLayoutCount;
	std::map<D3D12_SHADER_VISIBILITY, std::vector<size_t>> m_numOfConstants;

	ComPtr<ID3D12RootSignature> m_rootSignature;

	// Pipeline state object.
	ComPtr<ID3D12PipelineState> m_pipelineState;
};