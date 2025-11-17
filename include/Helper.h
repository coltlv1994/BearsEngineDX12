#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> // For CommandLineToArgvW

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

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <exception>

// Helper functon
static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

// The number of swap chain back buffers.
constexpr uint8_t g_NumFrames = 2;

// Use WARP adapter
extern bool g_UseWarp;

extern uint32_t g_ClientWidth;
extern uint32_t g_ClientHeight;

// Set to true once the DX12 objects have been initialized.
extern bool g_IsInitialized;

// Window handle.
extern HWND g_hWnd;
// Window rectangle (used to toggle fullscreen state).
extern RECT g_WindowRect;

// DirectX 12 Objects
extern ComPtr<ID3D12Device2> g_Device;
extern ComPtr<ID3D12CommandQueue> g_CommandQueue;
extern ComPtr<IDXGISwapChain4> g_SwapChain;
extern ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
extern ComPtr<ID3D12GraphicsCommandList> g_CommandList;
extern ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];
extern ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
extern UINT g_RTVDescriptorSize;
extern UINT g_CurrentBackBufferIndex;

// Synchronization objects
extern ComPtr<ID3D12Fence> g_Fence;
extern uint64_t g_FenceValue;
extern uint64_t g_FrameFenceValues[g_NumFrames];
extern HANDLE g_FenceEvent;

// By default, enable V-Sync.
// Can be toggled with the V key.
extern bool g_VSync;
extern bool g_TearingSupported;
// By default, use windowed mode.
// Can be toggled with the Alt+Enter or F11
extern bool g_Fullscreen;

// Function declarations
void ParseCommandLineArguments();
void EnableDebugLayer();
void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName);
HWND CreateWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width, uint32_t height);
ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);
ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter);
ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
bool CheckTearingSupport();
ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount);
ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);
ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device);
HANDLE CreateEventHandle();
uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue);
void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent);
void Update();
void Render();
void Resize(uint32_t width, uint32_t height);
void SetFullscreen(bool fullscreen);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);