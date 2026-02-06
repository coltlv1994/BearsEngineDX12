#include <DX12LibPCH.h>
#include <Application.h>
#include "resource.h"
#include <UIManager.h>
#include <MeshManager.h>
#include <MessageQueue.h>

#include <Editor.h>
#include <CommandQueue.h>
#include <Window.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

constexpr wchar_t WINDOW_CLASS_NAME[] = L"DX12RenderWindowClass";

using WindowPtr = std::shared_ptr<Window>;
using WindowMap = std::map< HWND, WindowPtr >;
using WindowNameMap = std::map< std::wstring, WindowPtr >;

static Application* gs_pSingelton = nullptr;
static WindowMap gs_Windows;
static WindowNameMap gs_WindowByName;

static std::shared_ptr<BearWindow> gs_activeWindow = nullptr;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// A wrapper struct to allow shared pointers for the window class.
struct MakeWindow : public Window
{
	MakeWindow(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
		: Window(hWnd, windowName, clientWidth, clientHeight, vSync)
	{
	}
};

Application::Application(HINSTANCE hInst)
	: m_hInstance(hInst)
	, m_TearingSupported(false)
{
	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window 
	// to achieve 100% scaling while still allowing non-client window content to 
	// be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);


#if defined(_DEBUG)
	// Always enable the debug layer before doing anything DX12 related
	// so all possible errors generated while creating DX12 objects
	// are caught by the debug layer.
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif

	WNDCLASSEXW wndClass = { 0 };

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = &WndProc;
	wndClass.hInstance = m_hInstance;
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(APP_ICON));
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = WINDOW_CLASS_NAME;
	wndClass.hIconSm = LoadIcon(m_hInstance, MAKEINTRESOURCE(APP_ICON));

	if (!RegisterClassExW(&wndClass))
	{
		MessageBoxA(NULL, "Unable to register the window class.", "Error", MB_OK | MB_ICONERROR);
	}

	m_dxgiAdapter = GetAdapter(false);
	if (m_dxgiAdapter)
	{
		m_d3d12Device = CreateDevice(m_dxgiAdapter);
	}
	if (m_d3d12Device)
	{
		m_DirectCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_ComputeCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		m_CopyCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_COPY);

		m_TearingSupported = CheckTearingSupport();

		m_resourceUploadBatch = new ResourceUploadBatch(m_d3d12Device.Get());

		m_srvHeap = CreateDescriptorHeap(MAX_SIZE_IN_SRV_HEAP, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		sizeOfSrvHeapOffset = GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}

void Application::Create(HINSTANCE hInst)
{
	if (!gs_pSingelton)
	{
		gs_pSingelton = new Application(hInst);
	}
}

Application& Application::Get()
{
	assert(gs_pSingelton);
	return *gs_pSingelton;
}

void Application::Destroy()
{
	if (gs_pSingelton)
	{
		assert(gs_Windows.empty() && gs_WindowByName.empty() &&
			"All windows should be destroyed before destroying the application instance.");

		delete gs_pSingelton;
		gs_pSingelton = nullptr;
	}
}

Application::~Application()
{
	Flush();
}

Microsoft::WRL::ComPtr<IDXGIAdapter4> Application::GetAdapter(bool bUseWarp)
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	if (bUseWarp)
	{
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
					D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}

	return dxgiAdapter4;
}
Microsoft::WRL::ComPtr<ID3D12Device2> Application::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
{
	ComPtr<ID3D12Device2> d3d12Device2;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));
	//    NAME_D3D12_OBJECT(d3d12Device2);

		// Enable debug messages in debug mode.
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif

	return d3d12Device2;
}

bool Application::CheckTearingSupport()
{
	BOOL allowTearing = FALSE;

	// Rather than create the DXGI 1.5 factory interface directly, we create the
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
	// graphics debugging tools which will not support the 1.5 factory interface 
	// until a future update.
	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing));
		}
	}

	return allowTearing == TRUE;
}

bool Application::IsTearingSupported() const
{
	return m_TearingSupported;
}

std::shared_ptr<Window> Application::CreateRenderWindow(const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
{
	// First check if a window with the given name already exists.
	WindowNameMap::iterator windowIter = gs_WindowByName.find(windowName);
	if (windowIter != gs_WindowByName.end())
	{
		return windowIter->second;
	}

	RECT windowRect = { 0, 0, clientWidth, clientHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hWnd = CreateWindowW(WINDOW_CLASS_NAME, windowName.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, nullptr, m_hInstance, nullptr);

	if (!hWnd)
	{
		MessageBoxA(NULL, "Could not create the render window.", "Error", MB_OK | MB_ICONERROR);
		return nullptr;
	}

	WindowPtr pWindow = std::make_shared<MakeWindow>(hWnd, windowName, clientWidth, clientHeight, vSync);

	gs_Windows.insert(WindowMap::value_type(hWnd, pWindow));
	gs_WindowByName.insert(WindowNameMap::value_type(windowName, pWindow));

	return pWindow;
}

void Application::DestroyWindow(std::shared_ptr<Window> window)
{
	if (window) window->Destroy();
}

void Application::DestroyWindow(const std::wstring& windowName)
{
	WindowPtr pWindow = GetWindowByName(windowName);
	if (pWindow)
	{
		DestroyWindow(pWindow);
	}
}

std::shared_ptr<Window> Application::GetWindowByName(const std::wstring& windowName)
{
	std::shared_ptr<Window> window;
	WindowNameMap::iterator iter = gs_WindowByName.find(windowName);
	if (iter != gs_WindowByName.end())
	{
		window = iter->second;
	}

	return window;
}


int Application::Run(std::shared_ptr<Editor> editor)
{
	if (!editor->Initialize()) return 1;
	if (!editor->LoadContent()) return 2;

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Flush any commands in the commands queues before quiting.
	Flush();

	editor->UnloadContent();
	editor->Destroy();

	return static_cast<int>(msg.wParam);
}

void Application::Quit(int exitCode)
{
	PostQuitMessage(exitCode);
}

Microsoft::WRL::ComPtr<ID3D12Device2> Application::GetDevice() const
{
	return m_d3d12Device;
}

std::shared_ptr<CommandQueue> Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
	std::shared_ptr<CommandQueue> commandQueue;
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		commandQueue = m_DirectCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		commandQueue = m_ComputeCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		commandQueue = m_CopyCommandQueue;
		break;
	default:
		assert(false && "Invalid command queue type.");
	}

	return commandQueue;
}

void Application::Flush()
{
	m_DirectCommandQueue->Flush();
	m_ComputeCommandQueue->Flush();
	m_CopyCommandQueue->Flush();
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Application::CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = type;
	desc.NumDescriptors = numDescriptors;
	desc.Flags = flags;
	desc.NodeMask = 0;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	ThrowIfFailed(m_d3d12Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

UINT Application::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	return m_d3d12Device->GetDescriptorHandleIncrementSize(type);
}


// Remove a window from our window lists.
static void RemoveWindow(HWND hWnd)
{
	WindowMap::iterator windowIter = gs_Windows.find(hWnd);
	if (windowIter != gs_Windows.end())
	{
		WindowPtr pWindow = windowIter->second;
		gs_WindowByName.erase(pWindow->GetWindowName());
		gs_Windows.erase(windowIter);
	}
}

// Convert the message ID into a MouseButton ID
MouseButtonEventArgs::MouseButton DecodeMouseButton(UINT messageID)
{
	MouseButtonEventArgs::MouseButton mouseButton = MouseButtonEventArgs::None;
	switch (messageID)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Left;
	}
	break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Right;
	}
	break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Middel;
	}
	break;
	}

	return mouseButton;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
		return true;

	// any message that imgui won't handle goes down here

	if (gs_activeWindow)
	{
		switch (message)
		{
		case WM_PAINT:
		{
			Application::Get().RenderBearWindow(gs_activeWindow);
		}
		break;
		case WM_SIZE:
		{
			int width = ((int)(short)LOWORD(lParam));
			int height = ((int)(short)HIWORD(lParam));

			gs_activeWindow->OnResize(width, height);
		}
		break;
		case WM_DESTROY:
		{
			// If a window is being destroyed, remove it from the 
			// window maps.
			gs_activeWindow->Destroy();
			gs_activeWindow = nullptr;
			PostQuitMessage(0);

			//if (gs_Windows.empty())
			//{
			//	// If there are no more windows, quit the application.
			//	PostQuitMessage(0);
			//}
		}
		break;
		default:
			return DefWindowProcW(hwnd, message, wParam, lParam);
		}
	}
	else
	{
		return DefWindowProcW(hwnd, message, wParam, lParam);
	}

	return 0;
}

DirectX::ResourceUploadBatch& Application::GetRUB()
{
	return *m_resourceUploadBatch;
}

unsigned int Application::AllocateInSRVHeap(unsigned int p_requiredSize)
{
	// 0 in this heap is reserved to ImGui;
	// if return is 0, means the allocation failed.
	unsigned int returnedOffset = 0;

	m_srvHeapMutex.lock();

	// Critical zone; no other thread can allocate from the heap until this allocation is done
	if (m_topOfSrvHeap + p_requiredSize >= MAX_SIZE_IN_SRV_HEAP)
	{
		assert(false && "Out of memory in the SRV heap.");
	}
	else
	{
		unsigned int returnedOffset = m_topOfSrvHeap;
		m_topOfSrvHeap += p_requiredSize;
	}

	m_srvHeapMutex.unlock();

	return returnedOffset;
}

D3D12_CPU_DESCRIPTOR_HANDLE Application::GetSRVHeapCPUHandle(unsigned int offset) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_srvHeap->GetCPUDescriptorHandleForHeapStart(), offset, sizeOfSrvHeapOffset);
}

D3D12_GPU_DESCRIPTOR_HANDLE Application::GetSRVHeapGPUHandle(unsigned int offset) const
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(
		m_srvHeap->GetGPUDescriptorHandleForHeapStart(), offset, sizeOfSrvHeapOffset);
}

int Application::RunWithBearWindow(const std::wstring& p_windowName, int p_width, int p_height, bool p_isVSync)
{
	// This assumes Create() has successfully completed and the application singleton has been created.

	auto device = GetDevice();
	auto commandQueue = GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetD3D12CommandQueue();

	// move hwnd creation inside
	// main window is editor window, physics is not enabled by default
	m_mainWindow = std::make_shared<BearWindow>(p_windowName, p_width, p_height, p_isVSync, false);
	m_mainWindow->Initialize(WINDOW_CLASS_NAME, m_hInstance);

	// Create D3D12 Renderer
	m_renderer_p = new D3D12Renderer(L"FirstPassVertexShader", L"FirstPassPixelShader",
		L"SecondPassVertexShader", L"SecondPassPixelShader");

	// Boot up listener threads and other preparations like Editor class
	m_mainWindow->Show();

	//MeshManager::Get().SetSRVHeap(m_SRVHeap, m_samplersHeap); // only used in MeshManager, 2nd pass rendering
	MeshManager::Get().CreateDefaultTexture();
	UIManager::Get().InitializeD3D12(device, commandQueue, m_srvHeap, Window::BufferCount); // IMGUI

	UIManager::Get().StartListeningThread();
	MeshManager::Get().StartListeningThread();

	m_activeWindow = m_mainWindow;

	// Window loop
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Flush any commands in the commands queues before quiting.
	Flush();

	//editor->UnloadContent();
	m_mainWindow->Destroy();

	return static_cast<int>(msg.wParam);
}

void Application::RenderBearWindow(std::shared_ptr<BearWindow> window)
{
	D3D12Renderer& renderer = *m_renderer_p;
	BearWindow& bw = *window;

	// physics update
	if (bw.IsPhysicsEnabled() == true)
	{
		// update physics system
	}

	renderer.Render(bw);
}