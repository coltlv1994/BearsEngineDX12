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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetD3D12CommandQueue();

	// Create the descriptor heap for the depth-stencil view.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));

	// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
	// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
	D3D12_DESCRIPTOR_HEAP_DESC desHeapDesc = {};
	desHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desHeapDesc.NumDescriptors = 1;
	desHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&desHeapDesc, IID_PPV_ARGS(&m_SRVHeap)));

	// one texture for now, later update will support multiple textures with texture selector/manager
	unsigned char* data = ReadAndUploadTexture(L"textures\\2k_earth_daymap.jpg");
	//ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
	//	device.Get(), Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetCommandList().Get(),
	//	L"textures\\2k_earth_daymap.dds",
	//	m_texture, m_textureUploadHeap));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_texture->GetDesc().Format; // replace with texture
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = m_texture->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_SRVHeap->GetCPUDescriptorHandleForHeapStart());

	m_ContentLoaded = true;

	// Resize/Create the depth buffer.
	ResizeDepthBuffer(GetClientWidth(), GetClientHeight());

	// after resize(), the command list is executed.
	stbi_image_free(data);

	// Setup ImGui binding for DirectX 12
	UIManager::Get().InitializeD3D12(device, commandQueue, m_SRVHeap, Window::BufferCount);

	// Setup the main camera.
	m_mainCamera.SetPosition(0.0f, 0.0f, -10.0f);
	UIManager::Get().SetMainCamera(&m_mainCamera);

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

	totalTime += e.ElapsedTime; // delta time
	frameCount++;

	if (totalTime > 1.0)
	{
		double fps = frameCount / totalTime;

		char buffer[512];
		sprintf_s(buffer, "FPS: %f\n", fps);
		OutputDebugStringA(buffer);

		frameCount = 0;
		totalTime = 0.0;
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

		FLOAT clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };

		ClearRTV(commandList, rtv, clearColor);
		ClearDepth(commandList, dsv);
		ID3D12DescriptorHeap* ppHeaps[] = { m_SRVHeap.Get() };
		commandList->SetDescriptorHeaps(1, ppHeaps);
	}

	commandList->RSSetViewports(1, &m_Viewport);
	commandList->RSSetScissorRects(1, &m_ScissorRect);

	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
	CD3DX12_GPU_DESCRIPTOR_HANDLE textureHandle(m_SRVHeap->GetGPUDescriptorHandleForHeapStart());
	//auto mCbvSrvDescriptorSize = Application::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//textureHandle.Offset(1, mCbvSrvDescriptorSize);

	XMMATRIX vpMatrix = m_mainCamera.GetViewProjectionMatrix();

	UIManager::Get().CreateImGuiWindowContent();

	// Render all meshes in a separate thread
	// NOTE: for now, mutex is not required since CreateImGuiWindowContent() do not write anything to commandlist
	std::thread meshRenderThread(&MeshManager::RenderAllMeshes, &MeshManager::Get(), commandList, vpMatrix, textureHandle);

	// Wait mesh rendering to be finished, then draw ImGui on backbuffer
	meshRenderThread.join();
	UIManager::Get().Draw(commandList);

	// Present
	{
		TransitionResource(commandList, backBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		m_FenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

		currentBackBufferIndex = m_pWindow->Present();

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

unsigned char* Editor::ReadAndUploadTexture(const wchar_t* textureFilePath)
{
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	ComPtr<ID3D12Resource> textureUploadHeap;

	static CD3DX12_HEAP_PROPERTIES heap_default = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	char texturePath[500];
	wcstombs_s(nullptr, texturePath, textureFilePath, 500);

	int width, height, channels;
	unsigned char* imageData = stbi_load(texturePath, &width, &height, &channels, STBI_rgb_alpha);

	// Describe and create a Texture2D.
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	ThrowIfFailed(device->CreateCommittedResource(
		&heap_default,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_texture)));

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

	static CD3DX12_HEAP_PROPERTIES heap_upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC buffer_default_size = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	// Create the GPU upload buffer.
	ThrowIfFailed(device->CreateCommittedResource(
		&heap_upload,
		D3D12_HEAP_FLAG_NONE,
		&buffer_default_size,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)));

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &imageData[0];
	textureData.RowPitch = width * 4;
	textureData.SlicePitch = textureData.RowPitch * height;

	UpdateSubresources(commandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);

	return imageData;
}