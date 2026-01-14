#include <Shader.h>
#include <Helpers.h>
#include <cstring>
#include <DirectXMath.h>
#include <CommandQueue.h>
using namespace DirectX;

#include <Application.h>

Shader::Shader(const wchar_t* p_vsPath, const wchar_t* p_psPath)
{
	ThrowIfFailed(D3DReadFileToBlob(p_vsPath, &m_vertexShaderBlob));
	ThrowIfFailed(D3DReadFileToBlob(p_psPath, &m_pixelShaderBlob));
}

Shader::~Shader()
{
}

ComPtr<ID3DBlob> Shader::GetVertexShaderBlob()
{
	return m_vertexShaderBlob;
}

ComPtr<ID3DBlob> Shader::GetPixelShaderBlob()
{
	return m_pixelShaderBlob;
}

ComPtr<ID3D12RootSignature> Shader::GetRootSigniture()
{
	return m_RootSignature;
}

ComPtr<ID3D12PipelineState> Shader::GetPipelineState()
{
    return m_PipelineState;
}

void Shader::CreateRootSignitureAndPipelineStream(D3D12_INPUT_ELEMENT_DESC* inputLayout_p, size_t inputLayoutCount)
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
    CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParameters[0].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL); // texture
    rootParameters[1].InitAsConstants(sizeof(VertexShaderInput) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // MVP and t_i_model matrices
	rootParameters[2].InitAsConstantBufferView(1); // Material CB
	rootParameters[3].InitAsConstantBufferView(2); // Light CB

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
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
        featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    // Create the root signature.
    ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineState;
    memset(&graphicsPipelineState, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    graphicsPipelineState.InputLayout = { inputLayout_p, static_cast<UINT>(inputLayoutCount) };
    graphicsPipelineState.pRootSignature = m_RootSignature.Get();
    graphicsPipelineState.VS = CD3DX12_SHADER_BYTECODE(m_vertexShaderBlob.Get());
    graphicsPipelineState.PS = CD3DX12_SHADER_BYTECODE(m_pixelShaderBlob.Get());
    graphicsPipelineState.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    graphicsPipelineState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    graphicsPipelineState.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    graphicsPipelineState.SampleMask = UINT_MAX;
    graphicsPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphicsPipelineState.NumRenderTargets = 1;
    graphicsPipelineState.RTVFormats[0] = rtvFormats.RTFormats[0];
    graphicsPipelineState.SampleDesc.Count = 1;
    graphicsPipelineState.SampleDesc.Quality = 0;
    graphicsPipelineState.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&graphicsPipelineState, IID_PPV_ARGS(&m_PipelineState)));

}
