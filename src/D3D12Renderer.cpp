#include "D3D12Renderer.h"
#include "Application.h"
#include "CommandQueue.h"
#include "UIManager.h"
#include "MeshManager.h"

#include <DirectXMath.h>
using namespace DirectX;

D3D12Renderer::D3D12Renderer(const wchar_t* p_1stVsPath, const wchar_t* p_1sPsPath, const wchar_t* p_2ndVsPath, const wchar_t* p_2ndPsPath)
{
	m_shader_p = new Shader(L"FirstPassVertexShader", L"FirstPassPixelShader",
		L"SecondPassVertexShader", L"SecondPassPixelShader");

	m_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
}

void D3D12Renderer::Render(BearWindow& window)
{
	// acquire render resources, read-only
	const RenderResource& currentRR = window.PrepareForRender();
	unsigned int currentBackBufferIndex = currentRR.currentBackBufferIndex;

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

	// bind render targets
	commandList->OMSetRenderTargets(BearWindow::FirstPassRTVCount, &currentRR.firstPassRTV, TRUE, &currentRR.dsv);

	// Camera information needed from BearWindow
	//XMMATRIX vpMatrix = m_mainCamera.GetViewProjectionMatrix();
	//XMMATRIX invPVMatrix = m_mainCamera.GetInvPVMatrix();

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

	MeshManager::Get().RenderAllMeshes(commandList, vpMatrix);

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
	MeshManager::Get().RenderAllMeshes2ndPass(commandList, currentBackBufferIndex, invScreenPVMatrix);

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