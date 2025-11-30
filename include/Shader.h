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

private:
	ComPtr<ID3DBlob> m_vertexShaderBlob;
	ComPtr<ID3DBlob> m_pixelShaderBlob;
};