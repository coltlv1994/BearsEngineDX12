#include <Mesh.h>
#include <Application.h>

#include <DirectXMath.h>
using namespace DirectX;

#include <sstream>
#include <fstream>

Mesh::Mesh(const wchar_t* p_verticesPath, const wchar_t* p_indicesPath)
{
	m_vertices.clear();
	m_indices.clear();

    // read file
	_readFromFile(p_verticesPath, p_indicesPath);

    // create command list
    Application& app = Application::GetInstance();
    ComPtr<ID3D12Device2> device = app.GetDevice();
    ComPtr<ID3D12CommandAllocator> commandAllocator = app.GetCommandAllocator();
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

    // TODO: Create buffer view
    // Upload vertex buffer data.
    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    UpdateBufferResource(m_commandList,`
        &m_VertexBuffer, &intermediateVertexBuffer,
        _countof(g_Vertices), sizeof(VertexPosColor), g_Vertices);

    // Create the vertex buffer view.
    m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
    m_VertexBufferView.SizeInBytes = sizeof(g_Vertices);
    m_VertexBufferView.StrideInBytes = sizeof(VertexPosColor);

    // Upload index buffer data.
    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    UpdateBufferResource(m_commandList,
        &m_IndexBuffer, &intermediateIndexBuffer,
        _countof(g_Indicies), sizeof(WORD), g_Indicies);

    // Create index buffer view.
    m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_IndexBufferView.SizeInBytes = sizeof(g_Indicies);
}

ComPtr<ID3D12GraphicsCommandList2> Mesh::PopulateCommandList()
{
	// Populate command queue
	Application& app = Application::GetInstance();
	ComPtr<ID3D12Device2> device = app.GetDevice();
	ComPtr<ID3D12CommandAllocator> commandAllocator = app.GetCommandAllocator();
    ComPtr<ID3D12DescriptorHeap> RTVDescriptorHeap = app.GetRTVDescriptorHeap();
    UINT m_RTVDescriptorSize = app.GetRTVDescriptorSize();
    ComPtr<ID3D12DescriptorHeap> DSVDescriptorHeap = app.GetDSVDescriptorHeap();

	// populate command
    UINT currentBackBufferIndex = app.GetCurrentBackBufferIndex();
    auto backBuffer = app.GetCurrentBackBuffer();
    auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        currentBackBufferIndex, m_RTVDescriptorSize);
    auto dsv = DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    // Clear the render targets.
    {
        TransitionResource(commandList, backBuffer,
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

        commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    ComPtr<ID3D12PipelineState> pipelineState = m_shader_p->GetPipelineState();
    ComPtr<ID3D12RootSignature> rootSigniture = m_shader_p->GetRootSigniture();

    commandList->SetPipelineState(pipelineState.Get());
    commandList->SetGraphicsRootSignature(rootSigniture.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    commandList->IASetIndexBuffer(&m_IndexBufferView);

    commandList->RSSetViewports(1, &m_Viewport);
    commandList->RSSetScissorRects(1, &m_ScissorRect);

    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    // Update the MVP matrix
    XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
    mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

    commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);

	ThrowIfFailed(commandList->Close());

	return commandList;
}

void Mesh::UseShader(Shader* p_shader_p)
{
	// clear previous root signiture

	// set new shader
	m_shader_p = p_shader_p;
}

Shader& Mesh::GetShaderObjectByRef()
{
    return *m_shader_p;
}

// Transition a resource
void Mesh::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
    Microsoft::WRL::ComPtr<ID3D12Resource> resource,
    D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource.Get(),
        beforeState, afterState);

    commandList->ResourceBarrier(1, &barrier);
}


void Mesh::_readFromFile(const wchar_t* verticesPath, const wchar_t* indicesPath)
{
    std::ifstream vertexFile(verticesPath);
    std::ifstream triangleFile(indicesPath);

    if (vertexFile.is_open() && triangleFile.is_open())
    {
        // open is good
        float x, y, z, nx, ny, nz;

        while (vertexFile >> x >> y >> z >> nx >> ny >> nz)
        {
            m_vertices.push_back(x);
            m_vertices.push_back(y);
            m_vertices.push_back(z);
            m_vertices.push_back(nx);
            m_vertices.push_back(ny);
            m_vertices.push_back(nz);
        }

        UINT index0, index1, index2;
        while (triangleFile >> index0 >> index1 >> index2)
        {
            // NOTE: original file is arranged counterclockwise but D3D by default uses clockwise.
            m_indices.push_back(index0);
            m_indices.push_back(index2);
            m_indices.push_back(index1);
        }
    }
    else
    {
        exit(1);
    }
}