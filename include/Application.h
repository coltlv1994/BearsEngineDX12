#pragma once
#include <CommonHeaders.h>

#include <map>
#include <cassert>
#include <chrono>

// The number of swap chain back buffers.
constexpr uint8_t NUM_OF_FRAMES = 2;

// This class handles the window

class Application
{
public:

	static Application& GetInstance(HINSTANCE p_hInst, const std::wstring& p_windowTitle, int p_width, int p_height, bool p_isVSync = false);

	static Application& GetInstance();

	// will distribute via hwndMapper
	// to get Application address
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void Run();

	// Get functions
	ComPtr<ID3D12Device2> GetDevice()
	{
		return m_Device;
	}

	ComPtr<ID3D12CommandAllocator> GetCommandAllocator()
	{
		return m_CommandAllocators[m_CurrentBackBufferIndex];
	}

	UINT GetCurrentBackBufferIndex()
	{
		return m_CurrentBackBufferIndex;
	}

	ComPtr<ID3D12Resource> GetCurrentBackBuffer()
	{
		return m_BackBuffers[m_CurrentBackBufferIndex];
	}

	ComPtr<ID3D12DescriptorHeap> GetRTVDescriptorHeap()
	{
		return m_RTVDescriptorHeap;
	}

	UINT GetRTVDescriptorSize()
	{
		return m_RTVDescriptorSize;
	}

	ComPtr<ID3D12DescriptorHeap> GetDSVDescriptorHeap()
	{
		return m_DSVHeap;
	}

	//UINT GetRTVDescriptorSize()
	//{
	//	return m_RTVDescriptorSize;
	//}

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
	// Depth buffer.
	ComPtr<ID3D12Resource> m_DepthBuffer;
	// Descriptor heap for depth buffer.
	ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

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