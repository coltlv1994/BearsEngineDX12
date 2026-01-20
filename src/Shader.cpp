#include <Shader.h>
#include <Helpers.h>
#include <cstring>
#include <DirectXMath.h>
#include <CommandQueue.h>
using namespace DirectX;

#include <Application.h>

// For now, two passes share the same input layout
// POSITION: float3, NORMAL: float3, TEXCOORD: float2
static D3D12_INPUT_ELEMENT_DESC firstPassInputLayout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

static UINT firstPassInputLayoutCount = sizeof(firstPassInputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);

// Deferred rendering is second pass
//static D3D12_INPUT_ELEMENT_DESC secondPassInputLayout[] = {
//    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//};

Shader::Shader(const wchar_t* p_1stVsPath, const wchar_t* p_1stPsPath, const wchar_t* p_2ndVsPath, const wchar_t* p_2ndPsPath)
{
	m_1stVsPath = p_1stVsPath;
	m_1stPsPath = p_1stPsPath;
	m_2ndVsPath = p_2ndVsPath;
	m_2ndPsPath = p_2ndPsPath;

    _createRSAndPSO();
}

Shader::~Shader()
{
}

void Shader::_createRSAndPSO()
{
    ThrowIfFailed(D3DReadFileToBlob((L"shaders\\" + m_1stVsPath + L".cso").c_str(), &m_1stPassVertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob((L"shaders\\" + m_1stPsPath + L".cso").c_str(), &m_1stPassPixelShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob((L"shaders\\" + m_2ndVsPath + L".cso").c_str(), &m_2ndPassVertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob((L"shaders\\" + m_2ndPsPath + L".cso").c_str(), &m_2ndPassPixelShaderBlob));

	_create1st();
	_create2nd();
}

void Shader::RebuildShaders()
{
    ThrowIfFailed(D3DCompileFromFile((L"shaders\\" + m_1stVsPath + L".hlsl").c_str(), nullptr, nullptr,
		"main", "vs_5_1", 0, 0, &m_1stPassVertexShaderBlob, nullptr));

    ThrowIfFailed(D3DCompileFromFile((L"shaders\\" + m_1stPsPath + L".hlsl").c_str(), nullptr, nullptr,
        "main", "ps_5_1", 0, 0, &m_1stPassPixelShaderBlob, nullptr));

    ThrowIfFailed(D3DCompileFromFile((L"shaders\\" + m_2ndVsPath + L".hlsl").c_str(), nullptr, nullptr,
        "main", "vs_5_1", 0, 0, &m_2ndPassVertexShaderBlob, nullptr));

    ThrowIfFailed(D3DCompileFromFile((L"shaders\\" + m_2ndPsPath + L".hlsl").c_str(), nullptr, nullptr,
        "main", "ps_5_1", 0, 0, &m_2ndPassPixelShaderBlob, nullptr));

    _create1st();
    _create2nd();
}

void Shader::_create1st()
{
    auto device = Application::Get().GetDevice();

    // Create a root signature.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if ((((HRESULT)(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) < 0))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    // A single 32-bit constant root parameter that is used by the vertex shader.
    // first pass don't handle lights, only textures is enough
    CD3DX12_ROOT_PARAMETER1 rootParameters[2];
    CD3DX12_DESCRIPTOR_RANGE1 descriptorRange = CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);
    rootParameters[0].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL); // texture
    rootParameters[1].InitAsConstants(sizeof(VertexShaderInput) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // MVP matrix

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_ANISOTROPIC;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 8;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(2, rootParameters, 1, &sampler, rootSignatureFlags);

    // Serialize the root signature.
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
        featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    // Create the root signature.
    ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_1stPassRootSignature)));

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 3;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT; // diffuse
    rtvFormats.RTFormats[1] = DXGI_FORMAT_R32_FLOAT; // specular
    rtvFormats.RTFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT; // normal

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineState;
    memset(&graphicsPipelineState, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    graphicsPipelineState.InputLayout = { firstPassInputLayout, static_cast<UINT>(firstPassInputLayoutCount)};
    graphicsPipelineState.pRootSignature = m_1stPassRootSignature.Get();
    graphicsPipelineState.VS = CD3DX12_SHADER_BYTECODE(m_1stPassVertexShaderBlob.Get());
    graphicsPipelineState.PS = CD3DX12_SHADER_BYTECODE(m_1stPassPixelShaderBlob.Get());
    graphicsPipelineState.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    graphicsPipelineState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    graphicsPipelineState.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    graphicsPipelineState.SampleMask = UINT_MAX;
    graphicsPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphicsPipelineState.NumRenderTargets = rtvFormats.NumRenderTargets;
    graphicsPipelineState.RTVFormats[0] = rtvFormats.RTFormats[0];
    graphicsPipelineState.RTVFormats[1] = rtvFormats.RTFormats[1];
    graphicsPipelineState.RTVFormats[2] = rtvFormats.RTFormats[2];
    graphicsPipelineState.SampleDesc.Count = 1;
    graphicsPipelineState.SampleDesc.Quality = 0;
    graphicsPipelineState.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&graphicsPipelineState, IID_PPV_ARGS(&m_1stPassPipelineState)));
}

void Shader::_create2nd()
{
    auto device = Application::Get().GetDevice();

    // Create a root signature.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if ((((HRESULT)(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) < 0))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    // A single 32-bit constant root parameter that is used by the vertex shader.
    CD3DX12_ROOT_PARAMETER1 rootParameters[4];
    CD3DX12_DESCRIPTOR_RANGE1 descriptorRange1 = CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);
    CD3DX12_DESCRIPTOR_RANGE1 descriptorRange2 = CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
    rootParameters[0].InitAsConstantBufferView(0); // Light CB
    rootParameters[1].InitAsDescriptorTable(1, &descriptorRange1, D3D12_SHADER_VISIBILITY_PIXEL); // G-buffer inputs
    rootParameters[2].InitAsDescriptorTable(1, &descriptorRange2, D3D12_SHADER_VISIBILITY_PIXEL); // G-buffer inputs, depth
    rootParameters[3].InitAsConstants(sizeof(SecondPassRootConstants) / 4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL); // MVP matrix

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_ANISOTROPIC;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 8;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(4, rootParameters, 1, &sampler, rootSignatureFlags);

    // Serialize the root signature.
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
        featureData.HighestVersion, &rootSignatureBlob, &errorBlob);
    ThrowIfFailed(hr);
    // Create the root signature.
    ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_2ndPassRootSignature)));

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineState;
    memset(&graphicsPipelineState, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    graphicsPipelineState.InputLayout = { firstPassInputLayout, static_cast<UINT>(firstPassInputLayoutCount) };
    graphicsPipelineState.pRootSignature = m_2ndPassRootSignature.Get();
    graphicsPipelineState.VS = CD3DX12_SHADER_BYTECODE(m_2ndPassVertexShaderBlob.Get());
    graphicsPipelineState.PS = CD3DX12_SHADER_BYTECODE(m_2ndPassPixelShaderBlob.Get());
    graphicsPipelineState.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    graphicsPipelineState.RasterizerState.DepthClipEnable = false;
    graphicsPipelineState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    graphicsPipelineState.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    graphicsPipelineState.DepthStencilState.DepthEnable = false;
    graphicsPipelineState.SampleMask = UINT_MAX;
    graphicsPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphicsPipelineState.NumRenderTargets = 1;
    graphicsPipelineState.RTVFormats[0] = rtvFormats.RTFormats[0];
    graphicsPipelineState.SampleDesc.Count = 1;
    graphicsPipelineState.SampleDesc.Quality = 0;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&graphicsPipelineState, IID_PPV_ARGS(&m_2ndPassPipelineState)));
}
