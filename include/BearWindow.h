#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <dxgi1_5.h>

#include <string>


class BearWindow
{
public:
	BearWindow() = delete;
	BearWindow(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync);

	// Create the swapchian.
	ComPtr<IDXGISwapChain4> CreateSwapChain();

	/**
	 * Show this window.
	 */
	void Show();

	/**
	 * Hide the window.
	 */
	void Hide();

	// Pass the resources needed in Application/Editor
	void PrepareForRender(
		float& out_deltaTime,
		unsigned int& out_currentBackBufferIndex,
		bool& out_isPhysicsEnabled,
		D3D12_CPU_DESCRIPTOR_HANDLE out_1stPassRTV,
		D3D12_CPU_DESCRIPTOR_HANDLE out_2ndPassRTV,
		ComPtr<ID3D12Resource> out_backBufferResource,
		ComPtr<ID3D12Resource> out_depthBufferResource,
		D3D12_VIEWPORT& out_viewport);

	/**
	 * Return the current back buffer index.
	 */
	UINT GetCurrentBackBufferIndex() const;

	/**
	 * Present the swapchain's back buffer to the screen.
	 * Returns the current back buffer index after the present.
	 */
	UINT Present();

	/**
	 * Get the render target view for the current back buffer.
	 */
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetFirstPassRenderTargetView() const;

	/**
	 * Get the back buffer resource for the current back buffer.
	 */
	ComPtr<ID3D12Resource> GetCurrentBackBuffer() const;

	// get first pass RTV's resources
	ComPtr<ID3D12Resource>* GetFirstPassRtvResource()
	{
		return m_firstPassRTVs;
	}

	// public accessible variables
	static const unsigned int BufferCount = 3;
	static const unsigned int FirstPassRTVCount = 3;
	static const unsigned int TotalOfTwoPassesRTVCount = FirstPassRTVCount + 1; // 3 for first pass, 1 for second pass

private:
	// Update the render target views for the swapchain back buffers.
	void UpdateRenderTargetViews();

	HWND m_hWnd;

	std::wstring m_windowName;

	int m_width;
	int m_height;
	bool m_isVSync;
	bool m_isFullscreen;

	ComPtr<IDXGISwapChain4> m_dxgiSwapChain;

	ComPtr<ID3D12DescriptorHeap> m_d3d12RTVDescriptorHeap; // only back buffers
	// backbuffer for final output
	ComPtr<ID3D12Resource> m_d3d12BackBuffers[BufferCount];

	// Descriptor heap for first pass RTVs
	ComPtr<ID3D12DescriptorHeap> m_firstPassRTVDescriptorHeap;
	// RTV for first pass, as G-buffer render targets
	// NOTE: don't use 2D array
	ComPtr<ID3D12Resource> m_firstPassRTVs[FirstPassRTVCount * BufferCount];

	UINT m_rtvDescriptorSize;
	UINT m_currentBackBufferIndex;

	RECT m_windowRect;
	D3D12_VIEWPORT m_Viewport;
	bool m_isTearingSupported;
};