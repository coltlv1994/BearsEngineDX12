#include <UIManager.h>
#include <exception>
#include <vector>
#include <MeshManager.h>

#include <iostream>
#include <fstream>

// UIManager singleton instance
static UIManager* gs_pSingleton = nullptr;

constexpr float PI_DIV_180 = 0.01745329; // PI / 180.0f

// copy-paste, no idea
static ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;

// persistent datas
static char camTable[3][4][128] =
{ {"MainCam", "X", "Y","Z"},
	{"Position", "0.0", "0.0", "-10.0"},
	{"Rotation", "0.0", "0.0", "0.0"} };
static char camTableID[3][4][128] =
{ {"0,0", "0,1", "0,2","0,3"},
	{"1,0", "##1,1", "##1,2","##1,3"},
	{"2,0", "##2,1", "##2,2","##2,3"} };
static float cameraFOV = 90.0f;

static char instanceTable[4][4][128] =
{ {"Instance", "IX", "IY","IZ"},
	{"Position", "##IPX", "##IPY", "##IPZ"},
	{"Rotation", "##IRX", "##IRY", "##IRZ"},
	{"Scale", "##ISX", "##ISY", "##ISZ"} };
static float camParam[2][3] = { {0.0f, 0.0f, -10.0f}, {0.0f, 0.0f, 0.0f} }; // position, rotation
static float instanceParam[3][3] = { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} , {1.0f, 1.0f, 1.0f} }; // position, rotation in degree, scale

static ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody;

static char meshObjectToLoad[128] = "";
static int selectedMeshIndex = 0;
static int selectedInstanceIndex = -1;

static char mapNameToLoad[128] = "default";

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
	io.WantCaptureKeyboard = true;
	io.WantCaptureMouse = true;
	io.WantTextInput = true;
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

	//ImGui::ShowDemoWindow();
}

void UIManager::CreateImGuiWindowContent()
{
	NewFrame();

	// all window content creation goes here
	// for demo purposes, we just show the demo window
	// Main body of the Demo window starts here.
	if (!ImGui::Begin("Control Panel"))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	// Window contents
	ImGui::InputText("mapName", mapNameToLoad, 128);
	if (ImGui::Button("Load Map"))
	{
		// load map
		_loadMap();
	}

	if (ImGui::Button("Save Map"))
	{
		// save map
		_saveMap();
	}

	if (ImGui::BeginTable("MainCam", 4, flags))
	{
		for (int row = 0; row < 3; row++)
		{
			ImGui::TableNextRow();
			for (int column = 0; column < 4; column++)
			{
				ImGui::TableSetColumnIndex(column);
				if (row == 0 || column == 0)
				{
					// header
					ImGui::Text(camTable[row][column]);
				}
				else
				{
					ImGui::SetNextItemWidth(-1.0f);
					ImGui::InputFloat(camTableID[row][column], &camParam[row - 1][column - 1], 0.1f, 1.0f, "%.3f");
				}
			}
		}
		ImGui::EndTable();
	}

	if (m_mainCamRef != nullptr)
	{
		// keyboard input for camera control
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantTextInput == false)
		{
			// if text input is not wanted, we can handle camera control
			if (ImGui::IsKeyPressed(ImGuiKey_W))
				camParam[0][2] += 0.1f; // move forward
			if (ImGui::IsKeyPressed(ImGuiKey_S))
				camParam[0][2] -= 0.1f; // move backward
			if (ImGui::IsKeyPressed(ImGuiKey_A))
				camParam[0][0] -= 0.1f; // move left
			if (ImGui::IsKeyPressed(ImGuiKey_D))
				camParam[0][0] += 0.1f; // move right
			if (ImGui::IsKeyPressed(ImGuiKey_Q))
				camParam[0][1] -= 0.1f; // move down
			if (ImGui::IsKeyPressed(ImGuiKey_E))
				camParam[0][1] += 0.1f; // move up

			//x - axis(pitch), then y - axis(yaw), and then z - axis(roll)
			if (ImGui::IsKeyPressed(ImGuiKey_U))
				camParam[1][0] += 0.1f; // PITCH UP
			if (ImGui::IsKeyPressed(ImGuiKey_J))
				camParam[1][0] -= 0.1f; // PITCH DOWN
			if (ImGui::IsKeyPressed(ImGuiKey_I))
				camParam[1][1] += 0.1f; // YAW UP
			if (ImGui::IsKeyPressed(ImGuiKey_K))
				camParam[1][1] -= 0.1f; // YAW DOWN
			if (ImGui::IsKeyPressed(ImGuiKey_O))
				camParam[1][2] += 0.1f; // ROLL UP
			if (ImGui::IsKeyPressed(ImGuiKey_L))
				camParam[1][2] -= 0.1f; // ROLL DOWN
		}

		m_mainCamRef->SetPosition(XMLoadFloat3((XMFLOAT3*)camParam[0]));
		_clampRotation(camParam[1]);
		m_mainCamRef->SetRotation(XMVectorSet(camParam[1][0], camParam[1][1], camParam[1][2], 0.0f) * PI_DIV_180);
	}

	// change main camera status via reference

	ImGui::Text("Meshes");
	ImGui::InputText("##MeshName", meshObjectToLoad, 128);
	if (ImGui::Button("Load"))
	{
		// send message to mesh manager
		Message* msg = new Message();
		msg->type = MSG_TYPE_LOAD_MESH;
		msg->SetData(meshObjectToLoad, strlen(meshObjectToLoad) + 1);
		MeshManager::Get().ReceiveMessage(msg);
	}

	// Populate mesh list
	if (listOfMeshes.size() > 0)
	{
		ImGui::Text("Loaded Meshes:");
		if (ImGui::BeginListBox("##listbox meshes"))
		{
			for (int n = 0; n < listOfMeshes.size(); n++)
			{
				const bool is_selected = (selectedMeshIndex == n);
				if (ImGui::Selectable(listOfMeshes[n].c_str(), is_selected))
					selectedMeshIndex = n;

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}
		if (ImGui::Button("Create Instance"))
		{
			// send message to mesh manager to create instance
			if (selectedMeshIndex >= 0 && selectedMeshIndex < listOfMeshes.size())
			{
				Message* msg = new Message();
				msg->type = MSG_TYPE_CREATE_INSTANCE;
				msg->SetData(listOfMeshes[selectedMeshIndex].c_str(), listOfMeshes[selectedMeshIndex].size() + 1);
				MeshManager::Get().ReceiveMessage(msg);
			}
		}
	}

	if (instanceMap.size() > 0)
	{
		ImGui::Text("Instances:");
		if (ImGui::BeginListBox("##listbox instances"))
		{
			int idx = 0;
			for (const auto& pair : instanceMap)
			{
				const bool is_selected = (selectedInstanceIndex == idx);
				if (ImGui::Selectable(pair.first.c_str(), is_selected))
					selectedInstanceIndex = idx;
				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
				idx++;
			}
			ImGui::EndListBox();
		}

		if (selectedInstanceIndex >= 0 && selectedInstanceIndex < instanceMap.size())
		{
			auto it = instanceMap.begin();
			std::advance(it, selectedInstanceIndex);
			Instance* selectedInstance = it->second;
			ImGui::Text("Selected Instance Transform:");
			XMVECTOR pos = selectedInstance->GetPosition();
			XMStoreFloat3((XMFLOAT3*)instanceParam[0], pos);
			XMVECTOR rot = selectedInstance->GetRotation() / PI_DIV_180;
			XMStoreFloat3((XMFLOAT3*)instanceParam[1], rot);
			XMVECTOR scale = selectedInstance->GetScale();
			XMStoreFloat3((XMFLOAT3*)instanceParam[2], scale);

			if (ImGui::BeginTable("Instance", 4, flags))
			{
				for (int row = 0; row < 4; row++)
				{
					ImGui::TableNextRow();
					for (int column = 0; column < 4; column++)
					{
						ImGui::TableSetColumnIndex(column);
						if (row == 0 || column == 0)
						{
							// header
							ImGui::Text(instanceTable[row][column]);
						}
						else
						{
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::InputFloat(instanceTable[row][column], &instanceParam[row - 1][column - 1], 0.1f, 1.0f, "%.3f");
						}
					}
				}
				ImGui::EndTable();
			}

			selectedInstance->SetPosition(instanceParam[0][0], instanceParam[0][1], instanceParam[0][2]);
			_clampRotation(instanceParam[1]);
			selectedInstance->SetRotation(XMVectorSet(instanceParam[1][0], instanceParam[1][1], instanceParam[1][2], 0.0f) * PI_DIV_180);
			_clampScale(instanceParam[2]);
			selectedInstance->SetScale(XMVectorSet(instanceParam[2][0], instanceParam[2][1], instanceParam[2][2], 0.0f));
		}
	}

	ImGui::End();

	// Render window content
	ImGui::Render();
}

void UIManager::Draw(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}

void UIManager::StartListeningThread()
{
	std::thread listenerThread(&UIManager::_listen, this);
	listenerThread.detach();
}

void UIManager::ReceiveMessage(Message* msg)
{
	m_messageQueue.PushMessage(msg);
}

void UIManager::_listen()
{
	while (true)
	{
		Message msg;
		if (m_messageQueue.PopMessage(msg))
		{
			// Process message
			_processMessage(msg);
			continue;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Avoid busy waiting
	}
}

void UIManager::_processMessage(Message& msg)
{
	switch (msg.type)
	{
	case MSG_TYPE_LOAD_SUCCESS:
	{
		// add mesh name to list
		size_t dataSize = msg.GetSize();
		if (dataSize != 0)
		{
			char* msgData = (char*)msg.GetData();
			std::string meshNameStr(msgData);
			listOfMeshes.push_back(meshNameStr);
		}
		break;
	}
	case MSG_TYPE_INSTANCE_REPLY:
	{
		// add instance to list
		size_t dataSize = msg.GetSize();
		if (dataSize < 1 + POINTER_SIZE)
		{
			// ill-formated message, ignore
			break;
		}
		else
		{
			char* msgData = (char*)msg.GetData();
			size_t nameLength = dataSize - POINTER_SIZE;
			std::string name = std::string((char*)msgData, nameLength);
			Instance* instancePtr = *(Instance**)(msgData + nameLength);
			instanceMap[std::string((char*)msgData, nameLength)] = instancePtr;
		}
		break;
	}
	default:
		break;
	}
	msg.Release();
}

void UIManager::_saveMap()
{
	char mapFileName[256];
	sprintf_s(mapFileName, 256, "saved_maps\\%s.bmap", mapNameToLoad);

	std::ofstream mapFile(mapFileName, std::ios::binary | std::ios::out);

	if (!mapFile.is_open())
	{
		std::cerr << "Failed to open map file for saving: " << mapFileName << std::endl;
		return;
	}

	size_t sizeOfCameraClass = sizeof(Camera);
	mapFile << "camera,";
	char* cameraData = new char[sizeOfCameraClass];
	memcpy(cameraData, m_mainCamRef, sizeOfCameraClass);
	mapFile.write(cameraData, sizeOfCameraClass);
	delete[] cameraData;

	// Save instances
	//std::vector<std::string> listOfMeshes;
	//std::map<std::string, Instance*> instanceMap;

	std::map<std::string, std::vector<Instance*>> instancesByMesh;

	for (const auto meshName : listOfMeshes)
	{
		instancesByMesh[meshName] = std::vector<Instance*>();
	}

	for (const auto& pair : instanceMap)
	{
		Instance* instance = pair.second;
		const wchar_t* meshClassNameWChar = instance->GetMeshClassName();
		size_t convertedChars = 0;
		char meshClassNameChar[128];
		wcstombs_s(&convertedChars, meshClassNameChar, 128, meshClassNameWChar, _TRUNCATE);
		std::string meshClassNameStr(meshClassNameChar);
		instancesByMesh[meshClassNameStr].push_back(instance);
	}

	for (const auto& meshPair : instancesByMesh)
	{
		mapFile << "mesh,";
		const std::string& meshName = meshPair.first;
		const std::vector<Instance*>& instances = meshPair.second;

		size_t totalRequiredSize = sizeof(ReloadInfo);
		if (instances.size() > 1)
		{
			totalRequiredSize += sizeof(InstanceInfo) * (instances.size() - 1);
		} // one is already included in ReloadInfo

		char* buffer = new char[totalRequiredSize];
		ReloadInfo& reloadInfo = *(ReloadInfo*)buffer;

		strcpy_s(reloadInfo.meshName, meshName.c_str());
		reloadInfo.numOfInstances = instances.size();

		// To the start of potential array
		InstanceInfo* instanceInfo_ptr = &reloadInfo.instanceInfos;

		for (size_t i = 0; i < reloadInfo.numOfInstances; i++)
		{
			Instance* instance = instances[i];

			XMVECTOR pos = instance->GetPosition();
			XMStoreFloat3((XMFLOAT3*)instanceInfo_ptr[i].position, pos);
			XMVECTOR rot = instance->GetRotation(); // use radians to avoid calculation in mesh manager
			XMStoreFloat3((XMFLOAT3*)instanceInfo_ptr[i].rotation, rot);
			XMVECTOR scale = instance->GetScale();
			XMStoreFloat3((XMFLOAT3*)instanceInfo_ptr[i].scale, scale);
		}

		mapFile.write(reinterpret_cast<char*>(&reloadInfo), totalRequiredSize);
	}

	mapFile.close();
}

bool UIManager::_loadMap()
{
	char mapFileName[256];
	sprintf_s(mapFileName, 256, "saved_maps\\%s.bmap", mapNameToLoad);

	std::ifstream mapFile(mapFileName, std::ios::binary | std::ios::ate);

	if (!mapFile.is_open())
	{
		std::cerr << "Failed to open map file for saving: " << mapFileName << std::endl;
		return false;
	}

	// read until end of file
	std::streamsize size = mapFile.tellg();
	mapFile.seekg(0, std::ios::beg);

	char* mapData = new char[size];
	if (!mapFile.read(mapData, size))
	{
		std::cerr << "Failed to read map file: " << mapFileName << std::endl;
		delete[] mapData;
		return false;
	}

	if (size < sizeof(Camera) || m_mainCamRef == nullptr)
	{
		std::cerr << "Map file too small to contain camera data." << std::endl;
		delete[] mapData;
		mapFile.close();
		return false;
	}

	// send message to MesnhManager to clear current meshes
	// UIManager does not own these pointers
	listOfMeshes.clear();
	instanceMap.clear();
	Message* message = new Message();
	message->type = MSG_TYPE_CLEAN_MESHES;
	MeshManager::Get().ReceiveMessage(message);

	// Camera class has fixed size
	size_t offset = 7; // "camera," take 7 bytes
	Camera tempCam;
	memcpy_s(&tempCam, sizeof(Camera), mapData + offset, sizeof(Camera));
	*m_mainCamRef = tempCam;
	XMVECTOR mainCamRot = m_mainCamRef->GetRotation() / PI_DIV_180; // to degrees
	XMStoreFloat3((XMFLOAT3*)camParam[0], m_mainCamRef->GetPosition());
	XMStoreFloat3((XMFLOAT3*)camParam[1], mainCamRot);

	offset += sizeof(Camera) + 5; // "mesh," take 5 bytes

	while (offset < size)
	{
		// copy data to buffer
		ReloadInfo& reloadInfo = *(ReloadInfo*)(mapData + offset);
		size_t messageSize = sizeof(ReloadInfo);
		if (reloadInfo.numOfInstances > 1)
		{
			messageSize += sizeof(InstanceInfo) * (reloadInfo.numOfInstances - 1);
		}
		
		Message* message = new Message();
		message->type = MSG_TYPE_RELOAD_MESH;
		message->SetData(&reloadInfo, messageSize);
		MeshManager::Get().ReceiveMessage(message);
		offset += messageSize + 5;// "mesh," take 5 bytes
	}
	
	mapFile.close();

	return true;
}

void UIManager::_clampRotation(float* rotation_p)
{
	float& rotX = rotation_p[0];
	float& rotY = rotation_p[1];
	float& rotZ = rotation_p[2];

	// Clamp rotation values to -180 to 180 degrees
	if (rotX < -180.0f) rotX += 360.0f;
	else if (rotX > 180.0f) rotX -= 360.0f;
	if (rotY <= -90.0f) rotY = -89.9f;
	else if (rotY >= 90.0f) rotY = 89.9f;
	if (rotZ < -180.0f) rotZ += 360.0f;
	else if (rotZ > 180.0f) rotZ -= 360.0f;
}

void UIManager::_clampScale(float* rotation_p)
{
	float& scaleX = rotation_p[0];
	float& scaleY = rotation_p[1];
	float& scaleZ = rotation_p[2];

	if (scaleX < 0.0f) scaleX = 0.0f;
	if (scaleY < 0.0f) scaleY = 0.0f;
	if (scaleZ < 0.0f) scaleZ = 0.0f;
}