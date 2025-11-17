#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> // For CommandLineToArgvW

#include <algorithm>
#include <cassert>
#include <chrono>
#include <exception>
#include <map>

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#ifdef CreateWindow
#undef CreateWindow
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// D3D12 headers
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// Helper function
static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

// The number of swap chain back buffers.
constexpr uint8_t NUM_OF_FRAMES = 2;

// This class handles the window

class Application
{
public:

	static Application& GetInstance(HINSTANCE p_hInst, const std::wstring& p_windowTitle, int p_width, int p_height, bool p_isVSync = false)
	{
		static Application m_app = Application(p_hInst, p_windowTitle, p_width, p_height, p_isVSync);
		return m_app;
	}

	// will distribute via hwndMapper
	// to get Application address
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void Run();

private:
	HINSTANCE m_hInst;
	std::wstring m_windowTitle;
	int m_height;
	int m_width;
	bool m_isVSync = true;
	const wchar_t* m_windowClassName = L"BearsEngineMainApplication";
	bool m_IsInitialized;
	HWND m_hwnd;
	RECT m_WindowRect; // Window rectangle (used to toggle fullscreen state).

	// DirectX 12 Objects
	ComPtr<ID3D12Device2> m_Device;
	ComPtr<ID3D12CommandQueue> m_CommandQueue;
	ComPtr<IDXGISwapChain4> m_SwapChain;
	ComPtr<ID3D12Resource> m_BackBuffers[NUM_OF_FRAMES];
	ComPtr<ID3D12GraphicsCommandList> m_CommandList;
	ComPtr<ID3D12CommandAllocator> m_CommandAllocators[NUM_OF_FRAMES];
	ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
	UINT m_RTVDescriptorSize;
	UINT m_CurrentBackBufferIndex;

	// Synchronization objects
	ComPtr<ID3D12Fence> m_Fence;
	uint64_t m_FenceValue = 0;
	uint64_t m_FrameFenceValues[NUM_OF_FRAMES] = {};
	HANDLE m_FenceEvent;

	// By default, enable V-Sync.
	// Can be toggled with the V key.
	bool m_TearingSupported = false;
	// By default, use windowed mode.
	// Can be toggled with the Alt+Enter or F11
	bool m_Fullscreen = false;

	// The number of swap chain back buffers.
	// Use WARP adapter
	bool m_UseWarp = false;

	Application(HINSTANCE p_hInst, const std::wstring& p_windowTitle, int p_width, int p_height, bool p_isVSync = true);

	void _windowInit();
	void _d3d12Init();

	bool _checkTearingSupport();
	ComPtr<IDXGIAdapter4> _getAdapter(bool useWarp);
	ComPtr<ID3D12Device2> _createDevice(ComPtr<IDXGIAdapter4> adapter);
	ComPtr<ID3D12CommandQueue> _createCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	ComPtr<IDXGISwapChain4> _createSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height);
	ComPtr<ID3D12DescriptorHeap> _createDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type);
	void _updateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
	ComPtr<ID3D12CommandAllocator> _createCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	ComPtr<ID3D12GraphicsCommandList> _createCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);
	ComPtr<ID3D12Fence> _createFence(ComPtr<ID3D12Device2> device);
	HANDLE _createEventHandle();

	// call proper WndProc()
	LRESULT _wndProc(UINT message, WPARAM wParam, LPARAM lParam);

	// Rendering functions
	uint64_t _signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue);
	void _waitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
	void _flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent);
	void _update();
	void _render();
	void _resize(uint32_t width, uint32_t height);
	void _setFullscreen(bool fullscreen);
};

extern std::map<HWND, Application*> hwndMapper;