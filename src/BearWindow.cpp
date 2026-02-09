#include <BearWindow.h>
#include <Helpers.h>
#include <Application.h>
#include <CommandQueue.h>
#include <UIManager.h>
#include <MeshManager.h>

#include <d3dx12.h>
#include <WinUser.h>

#include <algorithm>
#include <functional>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

BearWindow::BearWindow(const std::wstring& windowName, int clientWidth, int clientHeight, bool isPhysicsEnabled, double tickInterval)
	: m_windowName(windowName)
	, m_width(clientWidth)
	, m_height(clientHeight)
	, m_isPhysicsEnabled(isPhysicsEnabled)
	, m_isFullscreen(false)
	, m_tickInterval(tickInterval)
{
	m_isTearingSupported = Application::Get().IsTearingSupported();
}

bool BearWindow::Initialize(const wchar_t* p_windowClassName, HINSTANCE p_hInstance)
{
	// create hwnd
	RECT windowRect = { 0, 0, m_width, m_height };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	m_hWnd = CreateWindowW(p_windowClassName, m_windowName.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, nullptr, p_hInstance, nullptr);

	if (!m_hWnd)
	{
		MessageBoxA(NULL, "Could not create the render window.", "Error", MB_OK | MB_ICONERROR);
		return false;
	}

	m_dxgiSwapChain = CreateSwapChain();

	Application& app = Application::Get();
	m_rtvHeap = app.CreateDescriptorHeap(TotalRTVCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_rtvDescriptorSize = app.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	m_dsvHeap = app.CreateDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	m_offsetInSRVHeap = app.AllocateInSRVHeap(RequiredSizeInSRVHeap);

	if (m_offsetInSRVHeap == 0)
	{
		ThrowIfFailed(E_FAIL);
	}

	_createBackBuffersAndViewport();
	_updateRTVAndDSV();

	// Camera control
	UIManager::Get().SetMainCamera(&m_camera);
	XMFLOAT4 camPosition = XMFLOAT4(m_camera.GetPosition().m128_f32);
	MeshManager::Get().InitializeLightManager(camPosition);
	m_camera.SetAspectRatio(static_cast<float>(m_width) / m_height);

	if (m_isPhysicsEnabled == false)
	{
		// Editor windows do not have physics enabled by default
		// Set IMGUI UI
		UIManager::Get().InitializeWindow(m_hWnd);
	}
	else
	{
		_createD3D11on12Resources();
	}

	UpdateRenderResource();
}

void BearWindow::UpdateRenderResource()
{
	std::wstring debugStr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_GPU_DESCRIPTOR_HANDLE srvStartHandle = Application::Get().GetSRVHeapGPUHandle(m_offsetInSRVHeap);
	static const unsigned int srvIncrementSize = Application::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (unsigned int i = 0; i < BufferCount; ++i)
	{
		RenderResource& renderResource = m_renderResources[i];
		renderResource.currentBackBufferIndex = i;

		// unchanged values between frames
		renderResource.resourceArray = m_windowResources;
		renderResource.isPhysicsEnabled = m_isPhysicsEnabled;
		renderResource.depthBufferResourceIndex = TotalRTVCount; // the last one is for depth buffer
		renderResource.dsv = dsvHandle;
		// the SRV for depth buffer is right after all the SRVs for first pass RTVs, which is 9+1
		renderResource.depthBufferSRV = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvStartHandle, BufferCount * FirstPassRTVCount, srvIncrementSize);

		// these values will change between frames
		renderResource.firstPassResourceStartIndex = i * FirstPassRTVCount;
		renderResource.backBufferResourceIndex = BufferCount * FirstPassRTVCount + i;
		renderResource.firstPassRTV = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvStartHandle, renderResource.firstPassResourceStartIndex, m_rtvDescriptorSize);
		renderResource.secondPassRTV = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvStartHandle, renderResource.backBufferResourceIndex, m_rtvDescriptorSize);

		renderResource.secondPassSRV = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvStartHandle, renderResource.firstPassResourceStartIndex, srvIncrementSize);

		// Done already; keep for a reference
		//renderResource.d3d11wrappedBackBuffer = m_wrappedBackBuffers[i].Get();
		//renderResource.d2dRenderTarget = m_d2dRenderTargets[i].Get();
	}
}

void BearWindow::_updateRTVAndDSV()
{
	Application& app = Application::Get();
	auto device = app.GetDevice();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

	DXGI_FORMAT mRtvFormat[3] = {
		DXGI_FORMAT_R32G32B32A32_FLOAT, // diffuse
		DXGI_FORMAT_R32_FLOAT, // specular
		DXGI_FORMAT_R32G32B32A32_FLOAT // normal
	};

	CD3DX12_HEAP_PROPERTIES heapProperty(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC resourceDesc;
	ZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.MipLevels = 1;

	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Width = (UINT)m_width;
	resourceDesc.Height = (UINT)m_height;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	// first pass at front
	for (int i = 0; i < BufferCount; ++i)
	{
		// first pass render targets
		for (UINT j = 0; j < FirstPassRTVCount; ++j)
		{
			// Create a texture to be used as the first pass render target.
			resourceDesc.Format = mRtvFormat[j];

			ThrowIfFailed(
				device->CreateCommittedResource(
					&heapProperty,
					D3D12_HEAP_FLAG_NONE,
					&resourceDesc,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					nullptr,
					IID_PPV_ARGS(&m_windowResources[i * FirstPassRTVCount + j])));

			// Create the render target view for the first pass render target.
			device->CreateRenderTargetView(m_windowResources[i * FirstPassRTVCount + j].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	// and depth buffer
	// Create a depth buffer.
	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil = { 1.0f, 0 };

	CD3DX12_RESOURCE_DESC tex2d = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height,
		1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&tex2d,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&m_windowResources[TotalRTVCount]) // the last is for depth buffer resource
	));

	// Update the depth-stencil view.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(m_windowResources[TotalRTVCount].Get(), &dsvDesc,
		m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// update SRVs for the first pass RTVs and depth buffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Application::Get().GetSRVHeapCPUHandle(m_offsetInSRVHeap));
	static const unsigned int incrementSize = Application::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC descSRV;
	ZeroMemory(&descSRV, sizeof(descSRV));
	descSRV.Texture2D.MipLevels = 1;
	descSRV.Texture2D.MostDetailedMip = 0;
	descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	for (UINT i = 0; i < BufferCount; i++) {
		for (UINT j = 0; j < FirstPassRTVCount; j++) {
			descSRV.Format = mRtvFormat[j];
			device->CreateShaderResourceView(m_windowResources[i * 3 + j].Get(), &descSRV, srvHandle);
			srvHandle.Offset(1, incrementSize);
		}
	}

	// depth buffer SRV
	descSRV.Format = DXGI_FORMAT_R32_FLOAT;
	device->CreateShaderResourceView(m_windowResources[TotalRTVCount].Get(), &descSRV, srvHandle);
}

void BearWindow::_createBackBuffersAndViewport()
{
	static ComPtr<ID3D12Device2> device = Application::Get().GetDevice();
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), BufferCount * FirstPassRTVCount, m_rtvDescriptorSize);

	// then back buffers
	for (int i = 0; i < BufferCount; ++i)
	{
		// back buffers
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(m_dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		m_windowResources[BufferCount * FirstPassRTVCount + i] = backBuffer;

		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
}

void BearWindow::_resizeBackBuffersAndViewport()
{
	if (m_isPhysicsEnabled)
	{
		auto d3d11On12Device = UIManager::Get().GetD3D11On12Device();

		for (int i = 0; i < BufferCount; ++i)
		{
			//d3d11On12Device->ReleaseWrappedResources(m_wrappedBackBuffers[m_currentBackBufferIndex].GetAddressOf(), 1);
			m_wrappedBackBuffers[i].Reset();
			m_d2dRenderTargets[i].Reset();
		}

		UIManager::Get().CleanD3D11DeviceContextForResize();
	}

	Application::Get().Flush();

	for (int i = 0; i < BufferCount; ++i)
	{
		m_windowResources[BufferCount * FirstPassRTVCount + i].Reset();
	}

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	ThrowIfFailed(m_dxgiSwapChain->GetDesc(&swapChainDesc));
	ThrowIfFailed(m_dxgiSwapChain->ResizeBuffers(BufferCount, m_width,
		m_height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

	m_currentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

	_createBackBuffersAndViewport();
	if (m_isPhysicsEnabled == true)
	{
		_createD3D11on12Resources();
	}
	_updateRTVAndDSV();
}

void BearWindow::Show()
{
	::ShowWindow(m_hWnd, SW_SHOW);
}

void BearWindow::Hide()
{
	::ShowWindow(m_hWnd, SW_HIDE);
}

void BearWindow::Destroy()
{
	if (m_hWnd)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
}

void BearWindow::OnResize(int p_newWidth, int p_newHeight)
{
	// Update the window size.
	if (m_width != p_newWidth || m_height != p_newHeight)
	{
		m_width = std::max<int>(1, p_newWidth);
		m_height = std::max<int>(1, p_newHeight);

		//Application::Get().Flush();

		_resizeBackBuffersAndViewport();

		m_camera.SetAspectRatio(static_cast<float>(m_width) / static_cast<float>(m_height));
	}
}

ComPtr<IDXGISwapChain4> BearWindow::CreateSwapChain()
{
	Application& app = Application::Get();

	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = BufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = m_isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	ID3D12CommandQueue* pCommandQueue = app.GetCommandQueue()->GetD3D12CommandQueue().Get();

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
		pCommandQueue,
		m_hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

	m_currentBackBufferIndex = dxgiSwapChain4->GetCurrentBackBufferIndex();

	return dxgiSwapChain4;
}

unsigned int BearWindow::Present()
{
	UINT presentFlags = m_isTearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(m_dxgiSwapChain->Present(0, presentFlags));
	m_currentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

	return m_currentBackBufferIndex;
}

bool BearWindow::Tick(RenderResource& out_RR)
{
	// return value is bool
	// if false, no tick
	// if true, update
	bool returnValue = false;

	// only delta time and viewport may change between frames, so we update them here
	m_windowClock.Tick();

	double deltaSeconds = m_windowClock.GetDeltaSeconds();
	m_totalTime += deltaSeconds;
	m_timeSinceLastTick += deltaSeconds;

	// frame rate control
	if (m_timeSinceLastTick > m_tickInterval)
	{
		out_RR = m_renderResources[m_currentBackBufferIndex];

		// only delta time and viewport may change between frames, so we update them here
		out_RR.deltaTime = static_cast<float>(m_timeSinceLastTick);
		out_RR.viewport = m_viewport;
		++m_frameCount;

		m_timeSinceLastTick = 0.0;

		returnValue = true;
	}
	else
	{
		returnValue = false;
	}

	if (m_totalTime >= 1.0)
	{
		double fps = m_frameCount / m_totalTime;
		std::wstring newTitle = m_windowName + L" - FPS: " + std::to_wstring(fps);
		SetWindowTextW(m_hWnd, newTitle.c_str());
		m_totalTime = 0.0;
		m_frameCount = 0;
	}

	return returnValue;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT BearWindow::WindowMessageHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// return value decides if the message processing should pass on
	// assume this function is called first
	if (m_isPhysicsEnabled == false)
	{
		ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam);
	}
	else
	{
		// should be demo window
		// it should handle game inputs
		if (wParam == VK_ESCAPE && message == WM_KEYDOWN)
		{
			// switch back to main window
			Application::Get().SwitchToMainWindow();
		}
	}

	return DefWindowProcW(hwnd, message, wParam, lParam);
}

void BearWindow::GetCameraMatrices(XMMATRIX& out_viewProjMatrix, XMMATRIX& out_invPVMatrix) const
{
	// Editor windows do not have physics enabled by default
	out_viewProjMatrix = m_camera.GetViewProjectionMatrix();
	out_invPVMatrix = m_camera.GetInvPVMatrix();
}

void BearWindow::_createD3D11on12Resources()
{
	static const float dpi = static_cast<float>(Application::Get().GetDPI());
	static D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };

	static D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
		dpi,
		dpi);

	static Microsoft::WRL::ComPtr<ID3D11On12Device> d3d11On12Device = UIManager::Get().GetD3D11On12Device();
	static Microsoft::WRL::ComPtr<ID2D1DeviceContext2> d2d1DeviceContext = UIManager::Get().GetD2DDeviceContext();

	// Create a wrapped 11On12 resource of this back buffer. Since we are
	// rendering all D3D12 content first and then all D2D content, we specify
	// the In resource state as RENDER_TARGET - because D3D12 will have last
	// used it in this state - and the Out resource state as PRESENT. When
	// ReleaseWrappedResources() is called on the 11On12 device, the resource
	// will be transitioned to the PRESENT state.
	for (UINT i = 0; i < BufferCount; ++i)
	{
		ThrowIfFailed(d3d11On12Device->CreateWrappedResource(
			m_windowResources[BufferCount * FirstPassRTVCount + i].Get(),
			&d3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(&m_wrappedBackBuffers[i])
		));

		ComPtr<IDXGISurface> surface;
		ThrowIfFailed(m_wrappedBackBuffers[i].As(&surface));
		ThrowIfFailed(d2d1DeviceContext->CreateBitmapFromDxgiSurface(
			surface.Get(),
			&bitmapProperties,
			&m_d2dRenderTargets[i]
		));

		m_renderResources[i].d3d11wrappedBackBuffer = m_wrappedBackBuffers[i].Get();
		m_renderResources[i].d2dRenderTarget = m_d2dRenderTargets[i].Get();
	}
}