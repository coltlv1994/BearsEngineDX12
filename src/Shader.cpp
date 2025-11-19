#include <Shader.h>
#include <cstring>
#include <DirectXMath.h>
using namespace DirectX;

#include <Application.h>

Shader::Shader(const wchar_t* p_vsPath, const wchar_t* p_psPath, D3D12_INPUT_ELEMENT_DESC* p_inputLayout_p, size_t p_inputLayoutCount)
{
	ThrowIfFailed(D3DReadFileToBlob(p_vsPath, &m_vertexShaderBlob));
	ThrowIfFailed(D3DReadFileToBlob(p_psPath, &m_pixelShaderBlob));
	m_inputLayoutCount = p_inputLayoutCount;
	m_inputLayout_p = new D3D12_INPUT_ELEMENT_DESC[m_inputLayoutCount];
	memcpy(m_inputLayout_p, p_inputLayout_p, sizeof(D3D12_INPUT_ELEMENT_DESC) * p_inputLayoutCount);
}

Shader::~Shader()
{
	delete[] m_inputLayout_p;
}

D3D12_INPUT_ELEMENT_DESC& Shader::GetInputLayout()
{
	return *m_inputLayout_p;
}

size_t Shader::GetInputLayoutCount()
{
	return m_inputLayoutCount;
}

const void* Shader::GetVsByteCode()
{
	return m_vertexShaderBlob.Get();
}

const void* Shader::GetPsByteCode()
{
	return m_pixelShaderBlob.Get();
}

void Shader::CreateRootSigniture()
{
	// generate root signiture
	Application& app = Application::GetInstance();
	ComPtr<ID3D12Device2> device = app.GetDevice();

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// A single 32-bit constant root parameter that is used by the vertex shader.
	CD3DX12_ROOT_PARAMETER1 rootParameters[1] = { };
	rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

	// Serialize the root signature.
	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
		featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
	// Create the root signature.
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}
