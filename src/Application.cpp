#include <DX12LibPCH.h>
#include <Application.h>
#include "resource.h"
#include <UIManager.h>
#include <MeshManager.h>
#include <MessageQueue.h>

#include <CommandQueue.h>

#include <string>
#include <fstream>

#include <JoltHelper.h>

constexpr wchar_t WINDOW_CLASS_NAME[] = L"DX12RenderWindowClass";

static Application* gs_pSingelton = nullptr;

static std::shared_ptr<BearWindow> gs_activeWindow = nullptr;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

Application::Application(HINSTANCE hInst)
	: m_hInstance(hInst)
	, m_TearingSupported(false)
{
	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window
	// to achieve 100% scaling while still allowing non-client window content to
	// be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	auto desktopDc = GetDC(nullptr);

	m_dpi = GetDeviceCaps(desktopDc, LOGPIXELSY);
	m_dpiScale = static_cast<float>(m_dpi) / 96.0f;

	m_frameTimeInSeconds = 1.0 / static_cast<double>(std::min<int>(GetDeviceCaps(desktopDc, VREFRESH), 60));

#if defined(_DEBUG)
	// Always enable the debug layer before doing anything DX12 related
	// so all possible errors generated while creating DX12 objects
	// are caught by the debug layer.
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif

	WNDCLASSEXW wndClass = { 0 };

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = &WndProc;
	wndClass.hInstance = m_hInstance;
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(APP_ICON));
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = WINDOW_CLASS_NAME;
	wndClass.hIconSm = LoadIcon(m_hInstance, MAKEINTRESOURCE(APP_ICON));

	if (!RegisterClassExW(&wndClass))
	{
		MessageBoxA(NULL, "Unable to register the window class.", "Error", MB_OK | MB_ICONERROR);
	}

	m_dxgiAdapter = GetAdapter(false);

	if (m_dxgiAdapter)
	{
		m_d3d12Device = CreateDevice(m_dxgiAdapter);
	}

	if (m_d3d12Device)
	{
		m_DirectCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_ComputeCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		m_CopyCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_COPY);

		m_TearingSupported = CheckTearingSupport();

		m_resourceUploadBatch = new ResourceUploadBatch(m_d3d12Device.Get());

		m_srvHeap = CreateDescriptorHeap(MAX_SIZE_IN_SRV_HEAP, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		sizeOfSrvHeapOffset = GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		InitializeJoltPhysics();
	}
}

void Application::Create(HINSTANCE hInst)
{
	if (!gs_pSingelton)
	{
		gs_pSingelton = new Application(hInst);
	}
}

Application& Application::Get()
{
	assert(gs_pSingelton);
	return *gs_pSingelton;
}

void Application::Destroy()
{
	if (gs_pSingelton)
	{
		//assert(gs_Windows.empty() && gs_WindowByName.empty() &&
		//	"All windows should be destroyed before destroying the application instance.");

		delete gs_pSingelton;
		gs_pSingelton = nullptr;
	}
}

Application::~Application()
{
	Flush();
}

Microsoft::WRL::ComPtr<IDXGIAdapter4> Application::GetAdapter(bool bUseWarp)
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	if (bUseWarp)
	{
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
					D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}

	return dxgiAdapter4;
}
Microsoft::WRL::ComPtr<ID3D12Device2> Application::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
{
	ComPtr<ID3D12Device2> d3d12Device2;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));
	//    NAME_D3D12_OBJECT(d3d12Device2);

		// Enable debug messages in debug mode.
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_CREATERESOURCE_STATE_IGNORED,                  // This warning occurs with D3D11on12; it is implementation by MS, nothing we can do.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif

	return d3d12Device2;
}

bool Application::CheckTearingSupport()
{
	BOOL allowTearing = FALSE;

	// Rather than create the DXGI 1.5 factory interface directly, we create the
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the
	// graphics debugging tools which will not support the 1.5 factory interface
	// until a future update.
	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing));
		}
	}

	return allowTearing == TRUE;
}

bool Application::IsTearingSupported() const
{
	return m_TearingSupported;
}

void Application::Quit(int exitCode)
{
	PostQuitMessage(exitCode);
}

Microsoft::WRL::ComPtr<ID3D12Device2> Application::GetDevice() const
{
	return m_d3d12Device;
}

std::shared_ptr<CommandQueue> Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
	std::shared_ptr<CommandQueue> commandQueue;
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		commandQueue = m_DirectCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		commandQueue = m_ComputeCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		commandQueue = m_CopyCommandQueue;
		break;
	default:
		assert(false && "Invalid command queue type.");
	}

	return commandQueue;
}

void Application::Flush()
{
	m_DirectCommandQueue->Flush();
	m_ComputeCommandQueue->Flush();
	m_CopyCommandQueue->Flush();
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Application::CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = type;
	desc.NumDescriptors = numDescriptors;
	desc.Flags = flags;
	desc.NodeMask = 0;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	ThrowIfFailed(m_d3d12Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

UINT Application::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	return m_d3d12Device->GetDescriptorHandleIncrementSize(type);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// any message that imgui won't handle goes down here
	//static int wmpaintCount = 0;

	if (gs_activeWindow)
	{
		switch (message)
		{
		case WM_PAINT:
		{
			float frameTime = 0.0f;
			Application& app = Application::Get();
			if (app.Tick(frameTime))
			{
				if (!app.PendingWindowSwitchCheck())
				{
					break;
				}
				app.RenderBearWindow(gs_activeWindow);
			}

			break;
		}
		break;
		case WM_SIZE:
		{
			if (hwnd != gs_activeWindow->GetHWND())
			{
				break;
			}

			int width = ((int)(short)LOWORD(lParam));
			int height = ((int)(short)HIWORD(lParam));
			gs_activeWindow->OnResize(width, height);
		}
		break;
		case WM_DESTROY:
		{
			// If a window is being destroyed, remove it from the
			// window maps.
			gs_activeWindow->Destroy();
			gs_activeWindow = nullptr;
			PostQuitMessage(0);
		}
		break;
		default:
			return gs_activeWindow->WindowMessageHandler(hwnd, message, wParam, lParam);
		}
	}
	else
	{
		return DefWindowProcW(hwnd, message, wParam, lParam);
	}

	return 0;
}

DirectX::ResourceUploadBatch& Application::GetRUB()
{
	return *m_resourceUploadBatch;
}

unsigned int Application::AllocateInSRVHeap(unsigned int p_requiredSize)
{
	// 0 in this heap is reserved to ImGui;
	// if return is 0, means the allocation failed.
	unsigned int returnedOffset = 0;
	wchar_t buffer[512];

	m_srvHeapMutex.lock();

	// Critical zone; no other thread can allocate from the heap until this allocation is done
	if (m_topOfSrvHeap + p_requiredSize >= MAX_SIZE_IN_SRV_HEAP)
	{
		assert(false && "Out of memory in the SRV heap.");
	}
	else
	{
		returnedOffset = m_topOfSrvHeap;
		m_topOfSrvHeap += p_requiredSize;
	}

	m_srvHeapMutex.unlock();

	swprintf_s(buffer, 512, L"SRV heap allocation. Starting offset: %u, required size: %u, new top: %u\n", returnedOffset, p_requiredSize, m_topOfSrvHeap);
	OutputDebugStringW(buffer);
	return returnedOffset;
}

D3D12_CPU_DESCRIPTOR_HANDLE Application::GetSRVHeapCPUHandle(unsigned int offset) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_srvHeap->GetCPUDescriptorHandleForHeapStart(), offset, sizeOfSrvHeapOffset);
}

D3D12_GPU_DESCRIPTOR_HANDLE Application::GetSRVHeapGPUHandle(unsigned int offset) const
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(
		m_srvHeap->GetGPUDescriptorHandleForHeapStart(), offset, sizeOfSrvHeapOffset);
}

int Application::RunWithBearWindow(const std::wstring& p_windowName, int p_width, int p_height)
{
	// This assumes Create() has successfully completed and the application singleton has been created.

	auto device = GetDevice();
	auto commandQueue = GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetD3D12CommandQueue();

	// move hwnd creation inside
	// main window is editor window, physics is not enabled by default
	// DPI should be handled here
	int adjustedWidth = static_cast<int>(p_width * m_dpiScale);
	int adjustedHeight = static_cast<int>(p_height * m_dpiScale);

	m_mainWindow = std::make_shared<BearWindow>(p_windowName, adjustedWidth, adjustedHeight, false, false, m_frameTimeInSeconds);
	m_mainWindow->Initialize(WINDOW_CLASS_NAME, m_hInstance);

	// Create D3D12 Renderer
	m_mainWindow->Show();
	m_renderer_p = new D3D12Renderer(L"FirstPassVertexShader", L"FirstPassPixelShader",
		L"SecondPassVertexShader", L"SecondPassPixelShader");

	// Boot up listener threads and other preparations like Editor class
	MeshManager::Get().CreateDefaultTexture();
	UIManager::Get().InitializeD3D12(device, commandQueue, m_srvHeap, BearWindow::BufferCount); // IMGUI
	UIManager::Get().InitializeD3D11On12(device, commandQueue, m_dpiScale); // D3D11on12 for demo window UI

	UIManager::Get().StartListeningThread();
	MeshManager::Get().StartListeningThread();

	gs_activeWindow = m_mainWindow;
	m_windowClock.Reset();

	// Window loop
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Flush any commands in the commands queues before quiting.
	Flush();

	//editor->UnloadContent();
	m_mainWindow->Destroy();

	return static_cast<int>(msg.wParam);
}

void Application::RenderBearWindow(std::shared_ptr<BearWindow> window)
{
	if (m_gameState == GameState::DemoRunning)
	{
		// new location of cam/player
		m_gameClock.Tick();
		double totalSeconds = m_gameClock.GetTotalSeconds();
		double currentSection = std::floor(totalSeconds / m_sectionTimeInSeconds);
		float currentT = static_cast<float>(totalSeconds / m_sectionTimeInSeconds - currentSection);
		float oneMinusT = 1.0f - currentT;

		XMVECTOR newPosition = XMVectorZero();

		if (currentSection > m_numOfCurveSections)
		{
			// game should end
			SetGameState(GameState::DemoWin);
			return;
		}
		else
		{
			const XMVECTOR& p1 = m_bezierCurvePoints[static_cast<size_t>(currentSection) * 2];
			const XMVECTOR& p2 = m_bezierCurvePoints[static_cast<size_t>(currentSection) * 2 + 1];

			static XMVECTOR sectionEnd = XMVectorSet(10.0f, 0.0f, 10.0f, 1.0f);

			newPosition = 
				// starting point is always (0, 0, 0), so we can skip that part
				3.0f * std::powf(oneMinusT, 2.0f) * currentT * p1 +
				3.0f * oneMinusT * std::powf(currentT, 2.0f) * p2 +
				std::powf(currentT, 3.0f) * sectionEnd +
				sectionEnd * static_cast<float>(currentSection);

			newPosition.m128_f32[3] = 1.0f; // make sure w is 1 for correct transformation
			window->SetCameraLocation(newPosition);
		}

		static BodyInterface& bodyInterface = m_physicsSystem.GetBodyInterface();

		// TODO: update camera/player location and handle collision
		//static float constantDeltaTime = 1.0f / 60.0f;
	    //m_physicsSystem.Update(constantDeltaTime, 1, m_tempAllocator_p, m_jobSystem_p);

		//bodyInterface.SetPosition(m_camPlayerBodyId, JPH::Vec3(newPosition.m128_f32[0], 0.0f, newPosition.m128_f32[2]), EActivation::Activate);
		const JPH::BroadPhaseQuery& broadPhaseQuery = m_physicsSystem.GetBroadPhaseQuery();
		broadPhaseQuery.CollideSphere(
			JPH::Vec3(newPosition.m128_f32[0], newPosition.m128_f32[1], newPosition.m128_f32[2]),
			0.5f,
			m_sphereCollisionCollector,
			m_broadPhaseLayerFilter,
			m_defaultObjectLayerFilter);

		// update physics
		JPH::Vec3 raycastStart;
		JPH::Vec3 raycastDirection;
		if (window->IsRaycastRequested(raycastStart.mF32, raycastDirection.mF32))
		{
			// 1. Define the ray
			JPH::RRayCast ray;
			ray.mOrigin = raycastStart; // Start 10 units up
			ray.mDirection = raycastDirection * 100.0f; // Cast downwards for 100 units

			JPH::RayCastResult hit;

			const JPH::NarrowPhaseQuery& narrowPhaseQuery = m_physicsSystem.GetNarrowPhaseQuery();

			if (m_physicsSystem.GetNarrowPhaseQuery().CastRay(ray, hit))
			{
				// Hit detected
				JPH::Vec3 hitPosition = ray.GetPointOnRay(hit.mFraction);
				bodyInterface.RemoveBody(hit.mBodyID);
				bodyInterface.DestroyBody(hit.mBodyID);
				m_physicsBodiesSet.erase(hit.mBodyID);
				UIManager::Get().SetHitResult(hitPosition.mF32);
			}
		}
	}

	m_renderer_p->Render(*window);
}

void Application::SwitchToDemoWindow()
{
	m_pendingSwitchToDemoWindow = true;
}

void Application::SwitchToMainWindow()
{
	m_pendingSwitchToMainWindow = true;
}

bool Application::PendingWindowSwitchCheck()
{
	if (m_pendingSwitchToMainWindow)
	{
		m_pendingSwitchToMainWindow = false;
		gs_activeWindow = m_mainWindow;
		int returnValue = ShowCursor(true);

		// main window should always exist
		if (m_demoWindow)
		{
			m_demoWindow->Hide();
		}
		DestroyPhysicsBodies();
		ResetTimer();
		m_mainWindow->ResetCamera();
		m_mainWindow->Show();
		ClipCursor(nullptr);
		m_gameState = GameState::EditorScene;

		return false;
	}
	else if (m_pendingSwitchToDemoWindow)
	{
		m_pendingSwitchToDemoWindow = false;
		if (m_demoWindow == nullptr)
		{
			int adjustedWidth = static_cast<int>(1024 * m_dpiScale);
			int adjustedHeight = static_cast<int>(768 * m_dpiScale);
			m_demoWindow = std::make_shared<BearWindow>(L"Demo Window", adjustedWidth, adjustedHeight, true, true, m_frameTimeInSeconds);
			m_demoWindow->Initialize(WINDOW_CLASS_NAME, m_hInstance);
		}
		else
		{
			gs_activeWindow = m_demoWindow;
		}

		gs_activeWindow = m_demoWindow;
		AddPhysicsBodies();
		m_mainWindow->Hide();
		ResetTimer();
		m_mainWindow->ResetCamera();
		m_demoWindow->Show();
		int returnValue = ShowCursor(false);
		m_gameState = GameState::DemoStart;

		return false;
	}

	// render should continue
	return true;
}

void Application::InitializeJoltPhysics()
{
	RegisterDefaultAllocator();

	// Install trace and assert callbacks
	Trace = TraceImpl;
	JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl);

	Factory::sInstance = new Factory();
	// +		inFMT	0x00007ff77de1cf80 "Version mismatch, make sure you compile the client code with the same Jolt version and compiler definitions!"
	RegisterTypes();
	m_tempAllocator_p = new TempAllocatorImpl(10 * 1024 * 1024);
	m_jobSystem_p = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

	const uint cMaxBodies = 1024;
	const uint cNumBodyMutexes = 0;
	const uint cMaxBodyPairs = 1024;
	const uint cMaxContactConstraints = 1024;

	// Now we can create the actual physics system.
	m_physicsSystem.Init(
		cMaxBodies,
		cNumBodyMutexes,
		cMaxBodyPairs,
		cMaxContactConstraints,
		m_broadPhaseLayerInterface,
		m_objectVsBroadPhaseLayerFilter,
		m_objectVsObjectLayerFilter);

	m_physicsSystem.SetBodyActivationListener(&m_bodyActivationListener);

	m_physicsSystem.SetContactListener(&m_contactListener);

	m_physicsSystem.OptimizeBroadPhase();
#if defined(_DEBUG)
	//initializePhysicsBody_DEBUG();
#endif
}

bool Application::Tick(float& out_frameTime)
{
	// return value true means a tick is here
	// false means no
	// out_frameTime is only valid when return value is true

	m_windowClock.Tick();

	double deltaSeconds = m_windowClock.GetDeltaSeconds();
	m_totalTime += deltaSeconds;
	m_timeSinceLastTick += deltaSeconds;

	if (m_timeSinceLastTick > m_frameTimeInSeconds)
	{
		out_frameTime = static_cast<float>(m_timeSinceLastTick);
		m_timeSinceLastTick = 0.0;
		return true;
	}

	return false;
}

void Application::AddPhysicsBodies()
{
	// Get entity list
	const std::vector<Instance*>& instanceList = MeshManager::Get().GetInstanceList();

	static BodyInterface& bodyInterface = m_physicsSystem.GetBodyInterface();

	XMFLOAT4 position;
	XMFLOAT4 rotQuaternion;
	XMFLOAT4 scale;

	BodyCreationSettings bodySettings;

	for (auto in_p : instanceList)
	{
		Instance& instance = *in_p;

		JoltBodyShape bodyShape = instance.GetBodyShape();
		XMStoreFloat4(&position, instance.GetPosition());
		XMStoreFloat4(&rotQuaternion, instance.GetRotQuaternion());
		XMStoreFloat4(&scale, instance.GetScale());

		switch (bodyShape)
		{
		case JoltBodyShape::Sphere:
			bodySettings =
				BodyCreationSettings(
					new SphereShape(scale.x), // radius
					RVec3(position.x, position.y, position.z), // position
					Quat(rotQuaternion.x, rotQuaternion.y, rotQuaternion.z, rotQuaternion.w), // rotation
					EMotionType::Static,
					Layers::NON_MOVING);
			m_physicsBodiesSet.insert(bodyInterface.CreateAndAddBody(bodySettings, EActivation::Activate));
			break;
		case JoltBodyShape::Cube:
			bodySettings =
				BodyCreationSettings(
					new BoxShape(Vec3(scale.x, scale.y, scale.z)), // half extents
					RVec3(position.x, position.y, position.z), // position
					Quat(rotQuaternion.x, rotQuaternion.y, rotQuaternion.z, rotQuaternion.w), // rotation
					EMotionType::Static,
					Layers::NON_MOVING);
			m_physicsBodiesSet.insert(bodyInterface.CreateAndAddBody(bodySettings, EActivation::Activate));
			break;
		default:
			break;
		}
	}

	// need a separate one for camera/player
	bodySettings =
		BodyCreationSettings(
			new SphereShape(1.0f), // radius
			RVec3(0.0f, 0.0f, 0.0f), // position, starting position of camera/player is (0, 0, 0)
			Quat(0.0f, 0.0f, 0.0f, 1.0f), // rotation, theta is 0 -> cos(theta/2) = 1, sin(theta/2) = 0
			EMotionType::Dynamic,
			Layers::MOVING);
	m_camPlayerBodyId = bodyInterface.CreateAndAddBody(bodySettings, EActivation::Activate);
}

void Application::DestroyPhysicsBodies()
{
	static BodyInterface& bodyInterface = m_physicsSystem.GetBodyInterface();

	for (BodyID body : m_physicsBodiesSet)
	{
		bodyInterface.RemoveBody(body);
		bodyInterface.DestroyBody(body);
	}

	bodyInterface.RemoveBody(m_camPlayerBodyId);
	bodyInterface.DestroyBody(m_camPlayerBodyId);

	m_physicsBodiesSet.clear();
}

void Application::LoadBezierCurve()
{
	// the file path is fixed
	static const std::string filePath = "saved_maps\\bezier_control_points.txt";

	std::ifstream inFile(filePath);
	if (!inFile)
	{
		OutputDebugStringA("Failed to open bezier control points file.\n");
		return;
	}

	while (!inFile.eof())
	{
		std::string line;
		std::getline(inFile, line);
		if (line.empty())
		{
			continue;
		}
		std::istringstream iss(line);
		float x, y;
		if (!(iss >> x >> y))
		{
			continue;
		}

		// in NDC, y is up and xOz is the plane
		m_bezierCurvePoints.push_back(XMVectorSet(x, 0.0f, y, 0.0f));
	}

	inFile.close();

	if (m_bezierCurvePoints.size() < 2 || m_bezierCurvePoints.size() % 2 != 0)
	{
		m_bezierCurvePoints.clear();
	}

	m_numOfCurveSections = m_bezierCurvePoints.size() / 2;
}