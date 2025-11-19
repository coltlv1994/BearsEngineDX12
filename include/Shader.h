#pragma once
#include <CommonHeaders.h>

class Shader
{
public:
	Shader(const wchar_t* p_vsPath, const wchar_t* p_psPath, D3D12_INPUT_ELEMENT_DESC* p_inputLayout_p, size_t p_inputLayoutCount);
	~Shader();

	D3D12_INPUT_ELEMENT_DESC& GetInputLayout();
	size_t GetInputLayoutCount();
	const void* GetVsByteCode();
	const void* GetPsByteCode();


	void CreateRootSigniture();

private:
	ComPtr<ID3DBlob> m_vertexShaderBlob;
	ComPtr<ID3DBlob> m_pixelShaderBlob;
	D3D12_INPUT_ELEMENT_DESC* m_inputLayout_p;
	size_t m_inputLayoutCount;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
};