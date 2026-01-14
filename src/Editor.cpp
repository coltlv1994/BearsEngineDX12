#include <Editor.h>

#include <Application.h>
#include <CommandQueue.h>
#include <Helpers.h>
#include <Window.h>
#include <utility>
#include <UIManager.h>
#include <MeshManager.h>
#include <MessageQueue.h>
#include <thread>
#include <DDSTextureLoader.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3dx12.h>
#include <d3dcompiler.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <algorithm> // For std::min and std::max.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

using namespace DirectX;

// Clamp a value between a min and max range.
template<typename T>
constexpr const T& clamp(const T& val, const T& min, const T& max)
{
	return val < min ? min : val > max ? max : val;
}

Editor::Editor(const std::wstring& name, int width, int height, bool vSync)
	: super(name, width, height, vSync)
	, m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
	, m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
	, m_ContentLoaded(false)
{
}


bool Editor::LoadContent()
{
	Application& app = Application::Get();
	auto device = app.GetDevice();
	auto commandQueue = app.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetD3D12CommandQueue();

	// Create the descriptor heap for the depth-stencil view.
	m_DSVHeap = app.CreateDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	// Allocating SRV descriptors (for textures), and set the size to 128
	m_SRVHeap = app.CreateDescriptorHeap(128, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	MeshManager::Get().SetSRVHeap(m_SRVHeap);
	MeshManager::Get().CreateDefaultTexture();

	// Setup ImGui binding for DirectX 12
	UIManager::Get().InitializeD3D12(device, commandQueue, m_SRVHeap, Window::BufferCount);

	m_ContentLoaded = true;

	// Resize/Create the depth buffer.
	ResizeDepthBuffer(GetClientWidth(), GetClientHeight());

	// Setup the main camera.
	XMFLOAT4 camPosition = XMFLOAT4(0.0f, 0.0f, -10.0f, 1.0f);
	m_mainCamera.SetPosition(XMLoadFloat4(&camPosition));

	UIManager::Get().SetMainCamera(&m_mainCamera);
	MeshManager::Get().InitializeLightManager(camPosition);

	UIManager::Get().StartListeningThread();
	MeshManager::Get().StartListeningThread();

	return true;
}

void Editor::ResizeDepthBuffer(int width, int height)
{
	if (m_ContentLoaded)
	{
		// Flush any GPU commands that might be referencing the depth buffer.
		Application::Get().Flush();

		width = std::max(1, width);
		height = std::max(1, height);

		auto device = Application::Get().GetDevice();

		// Resize screen dependent resources.
		// Create a depth buffer.
		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		optimizedClearValue.DepthStencil = { 1.0f, 0 };

		static CD3DX12_HEAP_PROPERTIES heap_default = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC tex2d = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
			1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		ThrowIfFailed(device->CreateCommittedResource(
			&heap_default,
			D3D12_HEAP_FLAG_NONE,
			&tex2d,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optimizedClearValue,
			IID_PPV_ARGS(&m_DepthBuffer)
		));

		// Update the depth-stencil view.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;
		dsv.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv,
			m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
	}
}

void Editor::OnResize(ResizeEventArgs& e)
{
	if (e.Width != GetClientWidth() || e.Height != GetClientHeight())
	{
		super::OnResize(e);

		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
			static_cast<float>(e.Width), static_cast<float>(e.Height));

		m_mainCamera.SetAspectRatio(static_cast<float>(e.Width) / static_cast<float>(e.Height));

		ResizeDepthBuffer(e.Width, e.Height);
	}
}

void Editor::UnloadContent()
{
	m_ContentLoaded = false;
}

void Editor::OnUpdate(UpdateEventArgs& e)
{
	static uint64_t frameCount = 0;
	static double totalTime = 0.0;

	super::OnUpdate(e);

	totalTime += e.ElapsedTime; // ElapsedTime: delta time
	frameCount++;

	if (totalTime > 1.0)
	{
		double fps = frameCount / totalTime;

		char buffer[512];
		sprintf_s(buffer, "FPS: %f\n", fps);
		OutputDebugStringA(buffer);

		frameCount = 0;
		totalTime = 0.0;

		MEMORYSTATUSEX memInfo;
		memInfo.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&memInfo);
		uint64_t memInfoData[2]; // on 64-bit system, it won't exceed 2^64 bytes
		memInfoData[1] = memInfo.ullTotalPhys; // in bytes
		memInfoData[0] = memInfo.ullAvailPhys; // in bytes

		// send message to UIManager to update memory usage display
		Message* msgUM = new Message();
		Message* msgMM = new Message();
		msgUM->type = MSG_TYPE_CPU_MEMORY_INFO;
		msgMM->type = MSG_TYPE_CPU_MEMORY_INFO;
		msgUM->SetData(memInfoData, sizeof(memInfoData));
		msgMM->SetData(memInfoData, sizeof(memInfoData));
		UIManager::Get().ReceiveMessage(msgUM);
		MeshManager::Get().ReceiveMessage(msgMM);
	}
}

// Transition a resource
void Editor::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
	Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource.Get(),
		beforeState, afterState);

	commandList->ResourceBarrier(1, &barrier);
}

// Clear a render target.
void Editor::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor)
{
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void Editor::ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth)
{
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void Editor::OnRender(RenderEventArgs& e)
{
	super::OnRender(e);

	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList();

	UINT currentBackBufferIndex = m_pWindow->GetCurrentBackBufferIndex();
	auto backBuffer = m_pWindow->GetCurrentBackBuffer();
	auto rtv = m_pWindow->GetCurrentRenderTargetView();
	auto dsv = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

	// Clear the render targets.
	{
		TransitionResource(commandList, backBuffer,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

		ClearRTV(commandList, rtv, clearColor);
		ClearDepth(commandList, dsv);
		ID3D12DescriptorHeap* ppHeaps[] = { m_SRVHeap.Get() };
		commandList->SetDescriptorHeaps(1, ppHeaps);
	}

	commandList->RSSetViewports(1, &m_Viewport);
	commandList->RSSetScissorRects(1, &m_ScissorRect);

	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	XMMATRIX vpMatrix = m_mainCamera.GetViewProjectionMatrix();
	// TODO: get camera location for lighting calculations

	UIManager::Get().CreateImGuiWindowContent();

	// Render all meshes in a separate thread
	// NOTE: for now, mutex is not required since CreateImGuiWindowContent() do not write anything to commandlist

	std::thread meshRenderThread(&MeshManager::RenderAllMeshes, &MeshManager::Get(), commandList, vpMatrix);

	// Wait mesh rendering to be finished, then draw ImGui on backbuffer
	meshRenderThread.join();
	UIManager::Get().Draw(commandList);

	// Present
	{
		TransitionResource(commandList, backBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		m_FenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

		currentBackBufferIndex = m_pWindow->Present(); // it has moved to next buffer

		commandQueue->WaitForFenceValue(m_FenceValues[currentBackBufferIndex]);
	}
}

void Editor::OnKeyPressed(KeyEventArgs& e)
{
	super::OnKeyPressed(e);

	switch (e.Key)
	{
	case KeyCode::Escape:
		Application::Get().Quit(0);
		break;
	case KeyCode::Enter:
		if (e.Alt)
		{
	case KeyCode::F11:
		m_pWindow->ToggleFullscreen();
		break;
		}
	case KeyCode::V:
		m_pWindow->ToggleVSync();
		break;
	}
}

void Editor::OnMouseWheel(MouseWheelEventArgs& e)
{
	// Update with message system to EntityManager
	//m_FoV -= e.WheelDelta;
	//m_FoV = clamp(m_FoV, 12.0f, 90.0f);

	//char buffer[256];
	//sprintf_s(buffer, "FoV: %f\n", m_FoV);
	//OutputDebugStringA(buffer);
}

D3D12_VIEWPORT& Editor::GetViewport()
{
	return m_Viewport;
}

D3D12_RECT& Editor::GetScissorRect()
{
	return m_ScissorRect;
}