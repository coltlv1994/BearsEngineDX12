#pragma once
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "imgui_stdlib.h"

#include <wrl.h>
using namespace Microsoft::WRL;

#include <Camera.h>
#include <MessageQueue.h>
#include <EntityInstance.h>
#include <map>
#include <Helpers.h>

#include <d3d11.h>
#include <d3d11on12.h>
#include <d2d1_3.h>
#include <d3d12.h>

struct ExampleDescriptorHeapAllocator
{
	ID3D12DescriptorHeap* Heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
	D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
	UINT                        HeapHandleIncrement;
	ImVector<int>               FreeIndices;

	void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
	{
		IM_ASSERT(Heap == nullptr && FreeIndices.empty());
		Heap = heap;
		D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
		HeapType = desc.Type;
		HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
		HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
		HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
		FreeIndices.reserve((int)desc.NumDescriptors);
		for (int n = desc.NumDescriptors; n > 0; n--)
			FreeIndices.push_back(n - 1);
	}
	void Destroy()
	{
		Heap = nullptr;
		FreeIndices.clear();
	}
	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
	{
		IM_ASSERT(FreeIndices.Size > 0);
		int idx = FreeIndices.back();
		FreeIndices.pop_back();
		out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
		out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
	}
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
	{
		int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
		int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
		IM_ASSERT(cpu_idx == gpu_idx);
		FreeIndices.push_back(cpu_idx);
	}
};

class UIManager
{
public:
	~UIManager();
	static UIManager& Get();

	static void Destroy();

	void InitializeWindow(HWND hWnd);

	void InitializeD3D12(ComPtr<ID3D12Device>device, ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12DescriptorHeap> srvHeap, int numFramesInFlight);

	void InitializeD3D11On12(ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandQueue> commandQueue, float dpi);

	void NewFrame();

	void CreateImGuiWindowContent();

	void Draw(ComPtr<ID3D12GraphicsCommandList2> commandList);

	void DrawD2DContent(RenderResource& currentRR);

	void SetMainCamera(Camera* cam);

	void StartListeningThread();

	void ReceiveMessage(Message* msg);

	Microsoft::WRL::ComPtr<ID3D11On12Device> GetD3D11On12Device() const { return m_d3d11On12Device; }
	Microsoft::WRL::ComPtr<ID2D1DeviceContext2> GetD2DDeviceContext() const { return m_d2dDeviceContext; }

	void FlushD3D11DeviceContext()
	{
		ID3D11RenderTargetView* nullViews[] = { nullptr };
		m_d3d11DeviceContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
		m_d2dDeviceContext->SetTarget(nullptr);
		m_d3d11DeviceContext->Flush();
	}

private:
	Camera* m_mainCamRef = nullptr;
	MessageQueue m_messageQueue;
	std::vector<std::string> listOfMeshes; // UI manager does not directly operate on meshes and textures
	std::vector<std::string> listOfTextures;
	std::vector<std::string> listOfSamplers;
	std::vector<Instance*> listOfInstances;
	bool errorMessage = false; // to trigger error message popup
	char errorMessageBuffer[256] = { 0 };
	uint64_t m_memInfo[2] = { 0 }; // used and total memory info
	LightConstants m_lightConstants;

	void _listen();
	void _processMessage(Message& msg);
	void _saveMap();
	bool _loadMap();
	void _clampRotation(float* rotation_p);
	void _clampScale(float* scale_p);

	// D3D11on12 for demo window UI
	ComPtr<ID3D11Device> m_d3d11Device;
	ComPtr<ID3D11DeviceContext> m_d3d11DeviceContext;
	ComPtr<ID3D11On12Device> m_d3d11On12Device;
	ComPtr<ID2D1Factory3> m_d2dFactory;
	ComPtr<ID2D1Device2> m_d2dDevice;
	ComPtr<ID2D1DeviceContext2> m_d2dDeviceContext;
	ComPtr<IDWriteFactory> m_dWriteFactory;
	ComPtr<ID2D1SolidColorBrush> m_textBrush;
	ComPtr<IDWriteTextFormat> m_textFormat;
};