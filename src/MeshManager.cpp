#include <MeshManager.h>
#include <chrono>
#include <thread>
#include <UIManager.h>
#include <EntityInstance.h>
#include <Application.h>

// UIManager singleton instance
static MeshManager* gs_pSingleton = nullptr;

MeshManager& MeshManager::Get()
{
	if (gs_pSingleton == nullptr)
	{
		gs_pSingleton = new MeshManager();
	}
	return *gs_pSingleton;
}

void MeshManager::Destroy()
{
	delete gs_pSingleton;
}

MeshManager::~MeshManager()
{
	ClearMeshes();
}

bool MeshManager::AddMesh(const wchar_t* meshName, Shader* p_shader_p, const wchar_t* texturePath)
{
	wchar_t meshFullPath[128];
	swprintf_s(meshFullPath, L"meshes\\%s.obj", meshName);

	auto result = m_meshes.find(meshName);
	if (result != m_meshes.end())
	{
		// Mesh with the same name already exists
		return false;
	}
	else
	{
		Mesh* newMesh = new Mesh();
		bool createResult = false;
		createResult = newMesh->Initialize(meshFullPath);

		if (createResult)
		{
			m_meshes[meshName] = newMesh;
			newMesh->UseShader(p_shader_p);
			return true;
		}
		else
		{
			return false;
		}
	}
}

Mesh* MeshManager::GetMesh(const std::wstring& meshName)
{
	auto result = m_meshes.find(meshName);
	if (result != m_meshes.end())
	{
		// Mesh with the same name already exists
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

bool MeshManager::RemoveMesh(const std::wstring& meshName)
{
	auto result = m_meshes.find(meshName);
	if (result != m_meshes.end())
	{
		// Mesh with the same name already exists
		delete result->second;
		m_meshes.erase(result->first);
	}
	else
	{
		return false;
	}
}

void MeshManager::ClearMeshes()
{
	for (auto mesh_p : m_meshes)
	{
		delete mesh_p.second;
	}

	m_meshes.clear();
}

std::wstring MeshManager::_generateMeshName(const std::wstring& meshPath)
{
	size_t foundBackslash = meshPath.find_last_of(L"\\");
	std::wstring filename = meshPath.substr(foundBackslash + 1);
	size_t foundDot = filename.find_first_of(L".");
	std::wstring fileNameWithoutDot = filename.substr(0, foundDot);
	return fileNameWithoutDot;
}

void MeshManager::RenderAllMeshes(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle)
{
	for (auto meshClass : m_meshes)
	{
		meshClass.second->RenderInstances(p_commandList, p_vpMatrix, textureHandle);
	}
}

void MeshManager::Listen()
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

void MeshManager::_processMessage(Message& msg)
{
	// message data is mesh name in char*
	size_t dataSize = msg.GetSize();
	if (dataSize == 0 && msg.type != MSG_TYPE_CLEAN_MESHES)
	{
		return;
	}

	switch (msg.type)
	{
	case MSG_TYPE_LOAD_MESH:
	{
		char* meshName = (char*)msg.GetData();
		wchar_t* meshNameWChar = new wchar_t[dataSize];
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, meshNameWChar, dataSize, meshName, _TRUNCATE);

		Mesh* mesh_p = GetMesh(std::wstring(meshNameWChar));
		if (mesh_p)
		{
			// Mesh already exists, send back load success message
			_sendMeshLoadSuccessMessage(meshName, convertedChars);
			delete[] meshNameWChar;
			break;
		}

		// TODO: use different shader and texture if specified in message
		// texturePath is now fixed in Mesh constructor
		if (AddMesh(meshNameWChar, m_defaultShader_p, nullptr))
		{
			// Successfully added mesh
			// Send back load success message
			_sendMeshLoadSuccessMessage(meshName, convertedChars);
		}
		else
		{
			// Failed to add mesh, send error message
			_sendMeshLoadFailedMessage(meshName, convertedChars);

		}
		delete[] meshNameWChar;

		break;
	}
	case MSG_TYPE_CREATE_INSTANCE:
	{
		char* meshName = (char*)msg.GetData();
		wchar_t* meshNameWChar = new wchar_t[dataSize];
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, meshNameWChar, dataSize, meshName, _TRUNCATE);

		Mesh* mesh_p = GetMesh(std::wstring(meshNameWChar));

		if (mesh_p)
		{
			Instance* createdInstance = mesh_p->AddInstance();
			if (createdInstance != nullptr)
			{
				// Successfully added instance
				// Send back instance reply message
				createdInstance->SetMeshClassName(meshNameWChar);

				_sendInstanceReplyMessage(createdInstance);
			}
			else
			{
				// Failed to add instance, send error message
				_sendInstanceFailedMessage(meshName, convertedChars);
			}
		}
		delete[] meshNameWChar;
		break;
	}
	case MSG_TYPE_RELOAD_MESH:
	{
		char* buffer = new char[dataSize];
		memcpy(buffer, msg.GetData(), dataSize);
		ReloadInfo& reloadInfo = *(ReloadInfo*)buffer;

		wchar_t meshNameWChar[128];
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, meshNameWChar, 128, reloadInfo.meshName, _TRUNCATE);

		// try get, then create, if fail, return
		Mesh* mesh_p = GetMesh(std::wstring(meshNameWChar));
		bool result = true;
		if (!mesh_p)
		{
			result = AddMesh(meshNameWChar, m_defaultShader_p, nullptr);
			if (!result)
			{
				// Failed to add mesh, send error message
				_sendMeshLoadFailedMessage(reloadInfo.meshName, convertedChars);
				delete[] buffer;
				break;
			}
			mesh_p = GetMesh(std::wstring(meshNameWChar));
		}
		_sendMeshLoadSuccessMessage(reloadInfo.meshName, convertedChars);

		InstanceInfo* instanceInfos_p = &reloadInfo.instanceInfos;
		for (size_t i = 0; i < reloadInfo.numOfInstances; ++i)
		{
			Instance* instance_p = mesh_p->AddInstance();
			if (instance_p)
			{
				instance_p->SetPosition(instanceInfos_p[i].position[0], instanceInfos_p[i].position[1], instanceInfos_p[i].position[2]);
				// rotation is in radians already
				instance_p->SetRotation(XMVectorSet(instanceInfos_p[i].rotation[0], instanceInfos_p[i].rotation[1], instanceInfos_p[i].rotation[2], 0.0f));
				instance_p->SetScale(XMVectorSet(instanceInfos_p[i].scale[0], instanceInfos_p[i].scale[1], instanceInfos_p[i].scale[2], 0.0f));
				instance_p->SetMeshClassName(meshNameWChar);

				_sendInstanceReplyMessage(instance_p);
			}
			else
			{
				// Failed to add instance, send error message
				_sendInstanceFailedMessage(reloadInfo.meshName, convertedChars);
			}
		}

		// TODO: clean up unused mesh
		break;
	}
	case MSG_TYPE_CLEAN_MESHES:
	{
		// Clean all meshes
		CleanForLoad();
		break;
	}
	default:
		break;
	}

	msg.Release();
}

// Receive message from other systems
void MeshManager::ReceiveMessage(Message* msg)
{
	m_messageQueue.PushMessage(msg);
}

void MeshManager::_sendMeshLoadSuccessMessage(const char* meshName, size_t nameLength)
{
	Message* msgReply = new Message();
	msgReply->type = MSG_TYPE_LOAD_SUCCESS;
	msgReply->SetData(meshName, nameLength); // SetData is always copying
	UIManager::Get().ReceiveMessage(msgReply);
}

void MeshManager::_sendInstanceReplyMessage(Instance* createdInstance)
{
	std::wstring instanceNameWStr = createdInstance->GetName();
	char instanceName[128];
	size_t instanceNameLength = instanceNameWStr.size();
	wcstombs_s(nullptr, instanceName, 128, instanceNameWStr.c_str(), 128);

	size_t dataLength = instanceNameLength + 1 + POINTER_SIZE; // one for null terminator

	unsigned char* replyDataBuffer = new unsigned char[dataLength];
	memcpy(replyDataBuffer, instanceName, instanceNameLength + 1);
	memcpy(replyDataBuffer + instanceNameLength + 1, &createdInstance, POINTER_SIZE);

	Message* msgReply = new Message();
	msgReply->type = MSG_TYPE_INSTANCE_REPLY;
	msgReply->SetData(replyDataBuffer, dataLength); // SetData is always copying
	UIManager::Get().ReceiveMessage(msgReply);
	delete[] replyDataBuffer;
}

void MeshManager::CleanForLoad()
{
	// clean command queue
	Application::Get().Flush();

	// clean all instances
	for (auto& item : m_meshes)
	{
		item.second->ClearInstances();
	}

	//while (m_meshes.begin() != m_meshes.end())
	//{
	//	auto meshIter = m_meshes.begin();
	//	delete meshIter->second;
	//	m_meshes.erase(meshIter);
	//}
}

void MeshManager::_sendMeshLoadFailedMessage(const char* meshName, size_t nameLength)
{
	Message* msgReply = new Message();
	msgReply->type = MSG_TYPE_MESH_LOAD_FAILED;
	msgReply->SetData(meshName, nameLength); // SetData is always copying
	UIManager::Get().ReceiveMessage(msgReply);
}

void MeshManager::_sendInstanceFailedMessage(const char* meshName, size_t nameLength)
{
	Message* msgReply = new Message();
	msgReply->type = MSG_TYPE_INSTANCE_FAILED;
	msgReply->SetData(meshName, nameLength); // SetData is always copying
	UIManager::Get().ReceiveMessage(msgReply);
}