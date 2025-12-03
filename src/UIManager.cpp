#include <UIManager.h>
#include <exception>

// UIManager singleton instance
static UIManager* gs_pSingleton = nullptr;

// copy-paste, no idea
static ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

UIManager::~UIManager()
{
	// Cleanup ImGui
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

UIManager& UIManager::Get()
{
	if (gs_pSingleton == nullptr)
	{
		gs_pSingleton = new UIManager();
	}
	return *gs_pSingleton;
}

void UIManager::Destroy()
{
	delete gs_pSingleton;
	gs_pSingleton = nullptr;
}

void UIManager::InitializeWindow(HWND hWnd)
{
	ImGui_ImplWin32_EnableDpiAwareness();
	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);
	style.FontScaleDpi = main_scale;

	ImGui_ImplWin32_Init(hWnd);
}

void UIManager::InitializeD3D12(ComPtr<ID3D12Device>device, ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12DescriptorHeap> srvHeap, int numFramesInFlight)
{
	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = device.Get();
	init_info.CommandQueue = commandQueue.Get();
	init_info.NumFramesInFlight = numFramesInFlight;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	init_info.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	g_pd3dSrvDescHeapAlloc.Create(device.Get(), srvHeap.Get());

	init_info.SrvDescriptorHeap = srvHeap.Get();
	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_pd3dSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_pd3dSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };

	ImGui_ImplDX12_Init(&init_info);
}

void UIManager::NewFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void UIManager::CreateImGuiWindowContent()
{
	// all window content creation goes here
	// for demo purposes, we just show the demo window
	bool show_demo_window = true;
	ImGui::ShowDemoWindow(&show_demo_window);

	// Render window content
	ImGui::Render();
}

void UIManager::Draw(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}