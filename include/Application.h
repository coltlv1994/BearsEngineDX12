#pragma once
#include <CommonHeaders.h>
#include <EntityManager.h>
#include <Mesh.h>

#include <map>
#include <cassert>
#include <chrono>

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

	void AddEntity(const wchar_t* p_objFilePath);
	void AddEntity(Mesh* p_mesh_p);

	void FlushCommandQueue();

	// Get functions
	ComPtr<ID3D12Device2> GetDevice()
	{
		return m_device;
	}

	ComPtr<ID3D12CommandAllocator> GetCommandAllocator()
	{
		return m_commandAllocators[m_currentBackBufferIndex];
	}

	UINT GetCurrentBackBufferIndex()
	{
		return m_currentBackBufferIndex;
	}

	ComPtr<ID3D12Resource> GetCurrentBackBuffer()
	{
		return m_backBuffers[m_currentBackBufferIndex];
	}

	ComPtr<ID3D12DescriptorHeap> GetRTVDescriptorHeap()
	{
		return m_RTVDescriptorHeap;
	}

	UINT GetRTVDescriptorSize()
	{
		return m_rtvDescriptorSize;
	}

	ComPtr<ID3D12DescriptorHeap> GetDSVDescriptorHeap()
	{
		return m_DSVHeap;
	}

	ComPtr<ID3D12CommandQueue> GetCommandQueue()
	{
		return m_commandQueue;
	}

	uint64_t GetFenceValueForCurrentBackBuffer()
	{
		return m_frameFenceValues[m_currentBackBufferIndex];
	}

	void SetFenceValueArrayForCurrentBackBuffer(uint64_t p_newFenceValue)
	{
		m_frameFenceValues[m_currentBackBufferIndex] = p_newFenceValue;
	}

	void SignalCommandQueue(uint64_t p_value)
	{
		m_commandQueue->Signal(m_fence.Get(), p_value);
	}

	void GetWidthAndHeight(int& out_width, int& out_height)
	{
		out_width = m_width;
		out_height = m_height;
	}

	void ExecuteCommandList_DEBUG(ComPtr<ID3D12GraphicsCommandList2> p_CommandList);

	void MoveToNextFrame();

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
	ComPtr<ID3D12Device2> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<IDXGISwapChain4> m_swapChain;
	ComPtr<ID3D12Resource> m_backBuffers[NUM_OF_FRAMES];
	ComPtr<ID3D12GraphicsCommandList2> m_commandList;
	ComPtr<ID3D12CommandAllocator> m_commandAllocators[NUM_OF_FRAMES];
	ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
	UINT m_rtvDescriptorSize;
	UINT m_currentBackBufferIndex;
	// Depth buffer.
	ComPtr<ID3D12Resource> m_DepthBuffer;
	// Descriptor heap for depth buffer.
	ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	// Synchronization objects
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_frameFenceValues[NUM_OF_FRAMES] = {0};
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

	// Entity manager of this class
	EntityManager m_entityManager;

	Application(HINSTANCE p_hInst, const std::wstring& p_windowTitle, int p_width, int p_height, bool p_isVSync = true);

	void _windowInit();
	void _d3d12Init();

	bool _checkTearingSupport();
	ComPtr<IDXGIAdapter4> _getAdapter(bool useWarp);
	ComPtr<ID3D12Device2> _createDevice(ComPtr<IDXGIAdapter4> adapter);
	ComPtr<ID3D12CommandQueue> _createCommandQueue(D3D12_COMMAND_LIST_TYPE type);
	ComPtr<IDXGISwapChain4> _createSwapChain();
	ComPtr<ID3D12DescriptorHeap> _createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
	void _updateRenderTargetViews();
	ComPtr<ID3D12CommandAllocator> _createCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	ComPtr<ID3D12GraphicsCommandList2> _createCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);
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
	void _render2();
	void _resize(uint32_t width, uint32_t height);
	void _setFullscreen(bool fullscreen);
	void _renderEntities();
};

extern std::map<HWND, Application*> hwndMapper;