#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <dxgi1_5.h>
#include <DirectXMath.h>
using namespace DirectX;

#include <string>
#include <functional>

#include "HighResolutionClock.h"
#include "Helpers.h"
#include "Camera.h"

class BearWindow
{
public:
	BearWindow() = delete;
	BearWindow(const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync, bool isPhysicsEnabled);

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
	RenderResource& PrepareForRender();

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

	XMMATRIX GetViewProjectionMatrix() const
	{
		if (m_isPhysicsEnabled == false)
		{
			// Editor windows do not have physics enabled by default
			return m_camera.GetViewProjectionMatrix();
		}
		else
		{
			// TODO: get these from an object's position and rotation
			return XMMatrixIdentity();
		}
	}

	XMMATRIX GetInvPVMatrix() const
	{
		if (m_isPhysicsEnabled == false)
		{
			return m_camera.GetInvPVMatrix();
		}
		else
		{
			// TODO: get these from an object's position and rotation
			return XMMatrixIdentity();
		}
	}

	// public accessible variables
	static const unsigned int BufferCount = 3;
	static const unsigned int FirstPassRTVCount = 3;
	static const unsigned int TotalOfTwoPassesRTVCount = FirstPassRTVCount + 1; // 3 for first pass, 1 for second pass
	static const unsigned int TotalRTVCount = BufferCount * TotalOfTwoPassesRTVCount;
	static const unsigned int RequiredSizeInSRVHeap = BufferCount * FirstPassRTVCount + 1; // 3 SRV for first pass RTVs per frame + 1 SRV for depth buffer

private:
	// Update the RTVs (except back buffers) and DSV
	void UpdateRTVAndDSV();

	// back buffer needs some special handling
	void CreateBackBuffersAndViewport();
	void ResizeBackBuffersAndViewport();

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
	bool m_isVSync;
	bool m_isFullscreen;
	bool m_isTearingSupported;
	bool m_isPhysicsEnabled;
	unsigned int m_rtvDescriptorSize;
	RECT m_windowRect;
};