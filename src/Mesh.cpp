#include <Mesh.h>
#include <Application.h>

#include <DirectXMath.h>
using namespace DirectX;

#include <sstream>
#include <fstream>

Mesh::Mesh(const wchar_t* p_objFilePath)
{
	// read file
	LoadOBJFile(p_objFilePath, m_vertices, m_normals, m_triangles, m_triangleNormalIndex);

	// create command list
	Application& app = Application::GetInstance();
	m_device = app.GetDevice();
	m_commandAllocator = app.GetCommandAllocator();
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
	m_commandList->Close();

	// Upload vertex buffer data.
	_updateBufferResources();
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
		TransitionResource(m_commandList, backBuffer,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		m_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
		m_commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	ComPtr<ID3D12PipelineState> pipelineState = m_shader_p->GetPipelineState();
	ComPtr<ID3D12RootSignature> rootSigniture = m_shader_p->GetRootSigniture();

	m_commandList->SetPipelineState(pipelineState.Get());
	m_commandList->SetGraphicsRootSignature(rootSigniture.Get());

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->IASetIndexBuffer(&m_indexBufferView);

	//m_commandList->RSSetViewports(1, &m_viewPort);

	m_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	// Update the MVP matrix
	XMMATRIX mvpMatrix = XMMatrixMultiply(m_modelMatrix, m_viewMatrix);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, m_projectionMatrix);
	m_commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

	m_commandList->DrawIndexedInstanced(m_triangles.size(), 1, 0, 0, 0);

	ThrowIfFailed(m_commandList->Close());

	return m_commandList;
}

void Mesh::UseShader(Shader* p_shader_p)
{
	// clear previous root signiture

	// set new shader
	m_shader_p = p_shader_p;

	// create new root signiture
}

Shader& Mesh::GetShaderObjectByRef()
{
	return *m_shader_p;
}

// Transition a resource
void Mesh::TransitionResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
	ComPtr<ID3D12Resource> resource,
	D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource.Get(),
		beforeState, afterState);

	commandList->ResourceBarrier(1, &barrier);
}

void Mesh::_updateBufferResources()
{
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	ComPtr<ID3D12Resource> intermediateIndexBuffer;

	size_t vertexBufferSize = m_vertices.size() * sizeof(float);
	size_t indexBufferSize = m_triangles.size() * sizeof(uint32_t);
	CD3DX12_HEAP_PROPERTIES heapTypeDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_HEAP_PROPERTIES heapTypeUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize, D3D12_RESOURCE_FLAG_NONE);
	CD3DX12_RESOURCE_DESC vertexBufferSizeDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
	CD3DX12_RESOURCE_DESC indexBufferSizeDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

	// vertex buffer
	// Create a committed resource for the GPU resource in a default heap.
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapTypeDefault,
		D3D12_HEAP_FLAG_NONE,
		&vertexBufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)));

	// Create an committed resource for the upload.
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapTypeUpload,
		D3D12_HEAP_FLAG_NONE,
		&vertexBufferSizeDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&intermediateVertexBuffer)));

	D3D12_SUBRESOURCE_DATA vertexSubresourceData = {};
	vertexSubresourceData.pData = m_vertices.data();
	vertexSubresourceData.RowPitch = vertexBufferSize;
	vertexSubresourceData.SlicePitch = vertexSubresourceData.RowPitch;

	UpdateSubresources(m_commandList.Get(),
		m_vertexBuffer.Get(), intermediateVertexBuffer.Get(),
		0, 0, 1, &vertexSubresourceData);

	// Indices buffer
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapTypeDefault,
		D3D12_HEAP_FLAG_NONE,
		&indexBufferSizeDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_indexBuffer)));

	// Create an committed resource for the upload.
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapTypeUpload,
		D3D12_HEAP_FLAG_NONE,
		&indexBufferSizeDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&intermediateIndexBuffer)));

	D3D12_SUBRESOURCE_DATA triangleSubresourceData = {};
	triangleSubresourceData.pData = m_triangles.data();
	triangleSubresourceData.RowPitch = indexBufferSize;
	triangleSubresourceData.SlicePitch = triangleSubresourceData.RowPitch;

	UpdateSubresources(m_commandList.Get(),
		m_indexBuffer.Get(), intermediateIndexBuffer.Get(),
		0, 0, 1, &triangleSubresourceData);
}