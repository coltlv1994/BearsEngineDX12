#include <BearWindow.h>
#include <Application.h>

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

}