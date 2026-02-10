#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <dxgi1_5.h>
#include <DirectXMath.h>
using namespace DirectX;

#include <d3d11.h>
#include <d3d11on12.h>
#include <d2d1_3.h>

#include <string>
#include <functional>

#include "HighResolutionClock.h"
#include "Helpers.h"
#include "Camera.h"

class BearWindow
{
public:
	BearWindow() = delete;
	BearWindow(const std::wstring& windowName, int clientWidth, int clientHeight, bool isPhysicsEnabled, bool isDefaultFullScreen, double p_tickInterval);

	// create hwnd and other contents
	bool Initialize(const wchar_t* p_windowClassName, HINSTANCE p_hInstance);

	void UpdateRenderResource();
	// Window manipulation methods
	void Show();

	void Hide();

	void Destroy();

	void OnResize(int p_newWidth, int p_newHeight);

	//void ToggleFullscreen();

	// Render and present
	// Create swapchain for this window
	ComPtr<IDXGISwapChain4> CreateSwapChain();

	// Pass the resources needed in Application/Editor
	// physics is not handled here, but the window can decide whether to update physics or not based on the flag
	bool Tick(RenderResource& out_RR);

	/**
	 * Present the swapchain's back buffer to the screen.
	 * Returns the current back buffer index after the present.
	 */
	unsigned int Present();

	// Getters of attributes
	// Return the current back buffer index.
	unsigned int GetCurrentBackBufferIndex() const
	{
		return m_currentBackBufferIndex;
	}
	// Get SRV offset; this is shared between textures and other windows
	unsigned int GetOffsetInSRVHeap() const
	{
		return m_offsetInSRVHeap;
	}

	bool IsPhysicsEnabled() const
	{
		return m_isPhysicsEnabled;
	}

	HWND GetHWND() const
	{
		return m_hWnd;
	}

	void ResetWindowClock()
	{
		m_windowClock.Reset();
	}

	void CenterCursor()
	{
		POINT centerPoint = { m_windowRect.left + (m_windowRect.right - m_windowRect.left) / 2, m_windowRect.top + (m_windowRect.bottom - m_windowRect.top) / 2 };
		SetCursorPos(centerPoint.x, centerPoint.y);
	}

	void GetCameraMatrices(XMMATRIX& out_viewProjMatrix, XMMATRIX& out_invPVMatrix) const;

	LRESULT WindowMessageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	// public accessible variables
	static const unsigned int BufferCount = 3;
	static const unsigned int FirstPassRTVCount = 3;
	static const unsigned int TotalOfTwoPassesRTVCount = FirstPassRTVCount + 1; // 3 for first pass, 1 for second pass
	static const unsigned int TotalRTVCount = BufferCount * TotalOfTwoPassesRTVCount;
	static const unsigned int RequiredSizeInSRVHeap = BufferCount * FirstPassRTVCount + 1; // 3 SRV for first pass RTVs per frame + 1 SRV for depth buffer

private:
	// Update the RTVs (except back buffers) and DSV
	void _updateRTVAndDSV();

	// back buffer needs some special handling
	void _createBackBuffersAndViewport();
	void _resizeBackBuffersAndViewport();

	// d3d11 resouces
	void _createD3D11on12Resources();

	HWND m_hWnd;

	// per-window resource
	ComPtr<IDXGISwapChain4> m_dxgiSwapChain;
	unsigned int m_currentBackBufferIndex;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	ComPtr<ID3D12Resource> m_windowResources[TotalRTVCount + 1]; // one more for depth buffer
	D3D12_VIEWPORT m_viewport;
	HighResolutionClock m_windowClock;
	Camera m_camera;

	// output to Application/Editor to render
	RenderResource m_renderResources[BufferCount];

	// the big SRV heap for all windows
	unsigned int m_offsetInSRVHeap = 1;

	// per-window attributes
	std::wstring m_windowName;
	int m_width;
	int m_height;
	bool m_isFullscreen;
	bool m_isTearingSupported;
	bool m_isPhysicsEnabled;
	unsigned int m_rtvDescriptorSize;
	RECT m_windowRect;
	double m_timeSinceLastTick = 0.0;
	double m_tickInterval = 0.0;

	// Performance monitoring
	unsigned int m_frameCount = 0;
	double m_totalTime = 0.0;

	// D3D11on12 resources
	ComPtr<ID3D11Resource> m_wrappedBackBuffers[BufferCount];
	ComPtr<ID2D1Bitmap1> m_d2dRenderTargets[BufferCount];
};