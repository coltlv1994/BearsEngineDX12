#include <BearWindow.h>
#include <Helpers.h>
#include <Application.h>

#include <d3dx12.h>

BearWindow::BearWindow(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
	: m_hWnd(hWnd)
	, m_windowName(windowName)
	, m_width(clientWidth)
	, m_height(clientHeight)
	, m_isVSync(vSync)
	, m_isFullscreen(false)
{
	Application& app = Application::Get();

	m_isTearingSupported = app.IsTearingSupported();

	m_dxgiSwapChain = CreateSwapChain();

	m_d3d12RTVDescriptorHeap = app.CreateDescriptorHeap(BufferCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_firstPassRTVDescriptorHeap = app.CreateDescriptorHeap(FirstPassRTVCount * BufferCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_rtvDescriptorSize = app.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	UpdateRenderTargetViews();
}

void BearWindow::UpdateRenderTargetViews()
{
	auto device = Application::Get().GetDevice();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_d3d12RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE firstPassRtvHandle(m_firstPassRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	DXGI_FORMAT mRtvFormat[3] = {
		DXGI_FORMAT_R32G32B32A32_FLOAT, // diffuse
		DXGI_FORMAT_R32_FLOAT, // specular
		DXGI_FORMAT_R32G32B32A32_FLOAT // normal
	};

	CD3DX12_HEAP_PROPERTIES heapProperty(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC resourceDesc;
	ZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.MipLevels = 1;

	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Width = (UINT)m_width;
	resourceDesc.Height = (UINT)m_height;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	for (int i = 0; i < BufferCount; ++i)
	{
		// back buffers
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(m_dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		m_d3d12BackBuffers[i] = backBuffer;

		rtvHandle.Offset(1, m_rtvDescriptorSize);

		// first pass render targets
		// ComPtr<ID3D12Resource> will automatically release
		for (UINT j = 0; j < FirstPassRTVCount; ++j)
		{
			// Create a texture to be used as the first pass render target.
			resourceDesc.Format = mRtvFormat[j];

			ThrowIfFailed(
				device->CreateCommittedResource(
					&heapProperty,
					D3D12_HEAP_FLAG_NONE,
					&resourceDesc,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					nullptr,
					IID_PPV_ARGS(&m_firstPassRTVs[i * FirstPassRTVCount + j])));

			// Create the render target view for the first pass render target.
			device->CreateRenderTargetView(m_firstPassRTVs[i * FirstPassRTVCount + j].Get(), nullptr, firstPassRtvHandle);
			firstPassRtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}
}