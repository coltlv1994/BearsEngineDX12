#include "D3D12Renderer.h"
#include "Application.h"
#include "CommandQueue.h"
#include "UIManager.h"
#include "MeshManager.h"
#include "EntityInstance.h"

#include <DirectXMath.h>
using namespace DirectX;

#include <functional>
#include <vector>

D3D12Renderer::D3D12Renderer(const wchar_t* p_1stVsPath, const wchar_t* p_1sPsPath, const wchar_t* p_2ndVsPath, const wchar_t* p_2ndPsPath)
{
	m_shader_p = new Shader(L"FirstPassVertexShader", L"FirstPassPixelShader",
		L"SecondPassVertexShader", L"SecondPassPixelShader");

	m_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

	_prepare2ndPassResources();
}

void D3D12Renderer::Render(BearWindow& window)
{
	// acquire render resources, read-only
	RenderResource currentRR;

	if (!window.Tick(currentRR))
	{
		return;
	}

	unsigned int currentBackBufferIndex = currentRR.currentBackBufferIndex;
	static ID3D12DescriptorHeap* srvHeap = Application::Get().GetSRVHeap();

	// Get entity list, read only
	const std::vector<Instance*>& instanceList = MeshManager::Get().GetInstanceList();

	// first pass: render to G-buffer
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList();
	static UINT descriptorSize = Application::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	commandList->RSSetViewports(1, &currentRR.viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);

	FLOAT clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	CD3DX12_CPU_DESCRIPTOR_HANDLE firstPassRtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(currentRR.firstPassRTV);
	for (UINT i = 0; i < BearWindow::FirstPassRTVCount; i++)
	{
		_clearRTV(commandList, firstPassRtvHandle, clearColor);
		firstPassRtvHandle.Offset(1, descriptorSize);
	}

	_clearDepthBuffer(commandList, currentRR.dsv);

	// set SRV heap
	ID3D12DescriptorHeap* ppHeaps[] = { srvHeap };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// bind render targets
	commandList->OMSetRenderTargets(BearWindow::FirstPassRTVCount, &currentRR.firstPassRTV, TRUE, &currentRR.dsv);

	// Camera information needed from BearWindow
	XMMATRIX vpMatrix = window.GetViewProjectionMatrix();
	XMMATRIX invPVMatrix = window.GetInvPVMatrix();

	// treat the texture coord:
	// (screen space) x = 2u - 1, y = 1 - 2v
	XMFLOAT4X4 matScreen = XMFLOAT4X4(2.0, 0, 0, 0,
		0, -2.0, 0, 0,
		0, 0, 1, 0,
		-1, 1, 0, 1);
	XMMATRIX matS = XMLoadFloat4x4(&matScreen);

	XMMATRIX invScreenPVMatrix = XMMatrixMultiply(matS, invPVMatrix);

	if (currentRR.isPhysicsEnabled == false)
	{
		UIManager::Get().CreateImGuiWindowContent();
	}

	if (instanceList.size() > 0)
	{
		_renderFirstPass(commandList, instanceList, vpMatrix);
	}

	// second pass: render to back buffer
	for (UINT i = 0; i < BearWindow::FirstPassRTVCount; i++)
	{
		_transitionResource(commandList,
			currentRR.resourceArray[currentBackBufferIndex * BearWindow::FirstPassRTVCount + i],
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	_transitionResource(commandList,
		currentRR.resourceArray[currentRR.depthBufferResourceIndex],
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);

	_transitionResource(commandList, currentRR.resourceArray[currentRR.backBufferResourceIndex],
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	_clearRTV(commandList, currentRR.secondPassRTV, clearColor);
	commandList->OMSetRenderTargets(1, &currentRR.secondPassRTV, FALSE, nullptr);

	if (instanceList.size() > 0)
	{
		_renderSecondPass(commandList, invScreenPVMatrix, currentRR);
	}

	if (currentRR.isPhysicsEnabled == false)
	{
		UIManager::Get().Draw(commandList);
	}

	_transitionResource(commandList, currentRR.resourceArray[currentRR.backBufferResourceIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	// clean-up for next run, resource transition back to original states
	for (UINT i = 0; i < BearWindow::FirstPassRTVCount; i++)
	{
		_transitionResource(commandList,
			currentRR.resourceArray[currentBackBufferIndex * BearWindow::FirstPassRTVCount + i],
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	_transitionResource(commandList,
		currentRR.resourceArray[currentRR.depthBufferResourceIndex],
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// let GPU finish its work
	m_fenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);
	currentBackBufferIndex = window.Present(); // it has moved to next buffer
	commandQueue->WaitForFenceValue(m_fenceValues[currentBackBufferIndex]);
}

void D3D12Renderer::_transitionResource(
	ComPtr<ID3D12GraphicsCommandList2> commandList,
	ComPtr<ID3D12Resource> resource,
	D3D12_RESOURCE_STATES beforeState,
	D3D12_RESOURCE_STATES afterState)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource.Get(),
		beforeState, afterState);

	commandList->ResourceBarrier(1, &barrier);
}

// Clear a render target view.
void D3D12Renderer::_clearRTV(
	ComPtr<ID3D12GraphicsCommandList2> commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE rtv,
	FLOAT* clearColor)
{
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

// Clear the depth of a depth-stencil view.
void D3D12Renderer::_clearDepthBuffer(
	ComPtr<ID3D12GraphicsCommandList2> commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE dsv,
	FLOAT depth)
{
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void D3D12Renderer::_renderFirstPass(ComPtr<ID3D12GraphicsCommandList2> commandList, const std::vector<Instance*>& instanceList, const XMMATRIX& vpMatrix)
{
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
	m_shader_p->GetRSAndPSO_1stPass(rootSignature, pipelineState);

	commandList->SetPipelineState(pipelineState.Get());
	commandList->SetGraphicsRootSignature(rootSignature.Get());

	for (Instance* instance_p : instanceList)
	{
		// call mesh class to render it
		instance_p->Render(commandList, vpMatrix);
	}
}

void D3D12Renderer::_renderSecondPass(ComPtr<ID3D12GraphicsCommandList2> commandList, const XMMATRIX& invSPVMatrix, const RenderResource& currentRR)
{
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
	m_shader_p->GetRSAndPSO_2ndPass(rootSignature, pipelineState);

	commandList->SetPipelineState(pipelineState.Get());
	// sharing the same root signature for both passes
	commandList->SetGraphicsRootSignature(rootSignature.Get());

	// set light info here;
	commandList->SetGraphicsRootConstantBufferView(0, MeshManager::Get().GetLightCBVGPUAddress());

	SecondPassRootConstants sprc = {};
	sprc.invScreenPVMatrix = invSPVMatrix;
	commandList->SetGraphicsRoot32BitConstants(3, sizeof(sprc) / 4, &sprc, 0);

	commandList->SetGraphicsRootDescriptorTable(1, currentRR.secondPassSRV);

	// Depth
	commandList->SetGraphicsRootDescriptorTable(2, currentRR.depthBufferSRV);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	commandList->IASetVertexBuffers(0, 1, &m_2ndPassVertexBufferView);
	commandList->DrawInstanced(4, 1, 0, 0);
}

void D3D12Renderer::_prepare2ndPassResources()
{
	auto device = Application::Get().GetDevice();

	// create a full screen quad vertex buffer
	SecondPassVertexData quadVertices[] =
	{
		{ XMFLOAT3(-1.0f,  1.0f, 0.0f),  XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f,  1.0f, 0.0f),  XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f),  XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f),  XMFLOAT2(1.0f, 1.0f) },
	};

	const UINT quadVertexBufferSize = sizeof(quadVertices);

	CD3DX12_HEAP_PROPERTIES heapProperty(D3D12_HEAP_TYPE_UPLOAD);
	static CD3DX12_HEAP_PROPERTIES heap_upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC buffer_upload = CD3DX12_RESOURCE_DESC::Buffer(quadVertexBufferSize);

	// could move to default heap for better performance
	// but this is not urgently needed
	ThrowIfFailed(device->CreateCommittedResource(
		&heap_upload,
		D3D12_HEAP_FLAG_NONE,
		&buffer_upload,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_2ndPassVertexBuffer)));

	UINT8* dataBegin;
	ThrowIfFailed(m_2ndPassVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin)));
	memcpy(dataBegin, quadVertices, sizeof(quadVertices));
	m_2ndPassVertexBuffer->Unmap(0, nullptr);

	m_2ndPassVertexBufferView.BufferLocation = m_2ndPassVertexBuffer->GetGPUVirtualAddress();
	m_2ndPassVertexBufferView.StrideInBytes = sizeof(SecondPassVertexData);
	m_2ndPassVertexBufferView.SizeInBytes = sizeof(quadVertices);
}