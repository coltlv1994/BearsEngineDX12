/**
* The application class is used to create windows for our application.
*/
#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <memory>
#include <string>
#include <thread>
#include <mutex>

#include <d3d11.h>

class CommandQueue;

#include "ResourceUploadBatch.h"
#include "BearWindow.h"
#include "D3D12Renderer.h"

class Application
{
public:

	/**
	* Create the application singleton with the application instance handle.
	*/
	static void Create(HINSTANCE hInst);

	/**
	* Destroy the application instance and all windows created by this application instance.
	*/
	static void Destroy();
	/**
	* Get the application singleton.
	*/
	static Application& Get();

	DirectX::ResourceUploadBatch& GetRUB();

	/**
	 * Check to see if VSync-off is supported.
	 */
	bool IsTearingSupported() const;

	// New BearWindow system implementation
	int RunWithBearWindow(const std::wstring& windowName, int clientWidth, int clientHeight);

	/**
	* Request to quit the application and close all windows.
	* @param exitCode The error code to return to the invoking process.
	*/
	void Quit(int exitCode = 0);

	/**
	 * Get the Direct3D 12 device
	 */
	Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice() const;
	/**
	 * Get a command queue. Valid types are:
	 * - D3D12_COMMAND_LIST_TYPE_DIRECT : Can be used for draw, dispatch, or copy commands.
	 * - D3D12_COMMAND_LIST_TYPE_COMPUTE: Can be used for dispatch or copy commands.
	 * - D3D12_COMMAND_LIST_TYPE_COPY   : Can be used for copy commands.
	 */
	std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;

	// Flush all command queues.
	void Flush();

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

	// big srv heap management
	static const unsigned int MAX_SIZE_IN_SRV_HEAP = 65536;
	unsigned int AllocateInSRVHeap(unsigned int size);
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHeapCPUHandle(unsigned int offset) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHeapGPUHandle(unsigned int offset) const;
	ID3D12DescriptorHeap* GetSRVHeap() const
	{
		return m_srvHeap.Get();
	}

	// BearWindow system
	void RenderBearWindow(std::shared_ptr<BearWindow> window);

	void SwitchToDemoWindow();
	void SwitchToMainWindow();

	void PendingWindowSwitchCheck();

protected:

	// Create an application instance.
	Application(HINSTANCE hInst);
	// Destroy the application instance and all windows associated with this application.
	virtual ~Application();

	Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool bUseWarp);
	Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter);
	bool CheckTearingSupport();

private:
	Application(const Application& copy) = delete;
	Application& operator=(const Application& other) = delete;

	// The application instance handle that this application was created with.
	HINSTANCE m_hInstance;

	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_dxgiAdapter;
	Microsoft::WRL::ComPtr<ID3D12Device2> m_d3d12Device;

	// D3D11on12 for Direct2D interoperability
	Microsoft::WRL::ComPtr<ID3D11Device> m_d3d11Device;

	std::shared_ptr<CommandQueue> m_DirectCommandQueue;
	std::shared_ptr<CommandQueue> m_ComputeCommandQueue;
	std::shared_ptr<CommandQueue> m_CopyCommandQueue;

	bool m_TearingSupported;

	DirectX::ResourceUploadBatch* m_resourceUploadBatch;

	// one big SRV heap that stores windows' two-pass RTVs/DSV, and textures
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	unsigned int sizeOfSrvHeapOffset = 0;
	unsigned int m_topOfSrvHeap = 1; // start from 1, since 0 is used for imgui font texture
	std::mutex m_srvHeapMutex;

	// New BearWindow system
	std::shared_ptr<BearWindow> m_mainWindow; // this is the main window created at application start, UNLESS OTHERWISE SPECIFIED
	D3D12Renderer* m_renderer_p; // the renderer

	std::shared_ptr<BearWindow> m_demoWindow; // this window should have physics enabled

	bool m_pendingSwitchToDemoWindow = false;
	bool m_pendingSwitchToMainWindow = false;

	// HiDPI support
	float m_dpiScale = 1.0f;

	// refresh rate control
	double m_frameTimeInSeconds = 1.0 / 60.0;
};