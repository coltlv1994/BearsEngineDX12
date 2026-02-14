#include <MeshManager.h>
#include <chrono>
#include <thread>
#include <UIManager.h>
#include <Application.h>
#include <CommandQueue.h>

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

bool MeshManager::AddMesh(const std::string& meshName)
{
	auto result = m_meshes.find(meshName);
	if (result != m_meshes.end())
	{
		// Mesh with the same name already exists
		return false;
	}
	else
	{
		Mesh* newMesh = new Mesh();

		wchar_t meshNameWChar[128];
		mbstowcs_s(nullptr, meshNameWChar, 128, meshName.c_str(), _TRUNCATE);

		wchar_t meshFullPath[128];
		swprintf_s(meshFullPath, L"meshes\\%s.obj", meshNameWChar);

		bool createResult = false;
		createResult = newMesh->Initialize(meshFullPath);

		if (createResult)
		{
			m_meshes[meshName] = newMesh;
			newMesh->SetMeshClassName(meshName);
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool MeshManager::RemoveMesh(const std::string& meshName)
{
	auto result = m_meshes.find(meshName);
	if (result != m_meshes.end())
	{
		// Mesh with the same name already exists
		delete result->second;
		m_meshes.erase(result->first);
		return true;
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

	switch (msg.type)
	{
	case MSG_TYPE_LOAD_MESH:
	{
		std::string meshName = std::string((char*)msg.GetData());

		Mesh* mesh_p = GetMeshByName(meshName);
		if (mesh_p)
		{
			// Mesh already exists, send back load success message
			_sendMeshLoadSuccessMessage(meshName);
			break;
		}

		// TODO: use different shader and texture if specified in message
		// texturePath is now fixed in Mesh constructor
		if (AddMesh(meshName))
		{
			// Successfully added mesh
			// Send back load success message
			_sendMeshLoadSuccessMessage(meshName);
		}
		else
		{
			// Failed to add mesh, send error message
			_sendMeshLoadFailedMessage(meshName);
		}
		break;
	}
	case MSG_TYPE_CREATE_INSTANCE:
	{
		std::string instanceName = "instance_" + std::to_string(m_createdInstanceCount);

		Instance* createdInstance = new Instance(instanceName, m_textureMap["default_white"]);
		m_instanceList.push_back(createdInstance);
		_sendInstanceReplyMessage(createdInstance);

		m_createdInstanceCount += 1;

		break;
	}
	case MSG_TYPE_RELOAD_MESH:
	{
		char* buffer = new char[dataSize];
		memcpy(buffer, msg.GetData(), dataSize);
		ReloadInfo& reloadInfo = *(ReloadInfo*)buffer;

		std::string meshName(reloadInfo.meshName);

		// try get, then create, if fail, return
		Mesh* mesh_p = nullptr;
		if (meshName != "null_object")
		{
			mesh_p = GetMeshByName(meshName);
			if (!mesh_p)
			{
				if (!AddMesh(meshName))
				{
					// Failed to add mesh, send error message
					_sendMeshLoadFailedMessage(meshName);
					delete[] buffer;
					break;
				}
				mesh_p = GetMeshByName(meshName);
			}
			_sendMeshLoadSuccessMessage(meshName);
		}

		InstanceInfo* instanceInfos_p = &reloadInfo.instanceInfos;
		m_createdInstanceCount += reloadInfo.numOfInstances;
		for (size_t i = 0; i < reloadInfo.numOfInstances; ++i)
		{
			std::string instanceName = instanceInfos_p[i].instanceName;
			Texture* texture_p = GetTextureByName(std::string(instanceInfos_p[i].textureName));
			Instance* instance_p = new Instance(instanceName, texture_p, mesh_p);
			if (instance_p)
			{
				instance_p->SetPosition(instanceInfos_p[i].position[0], instanceInfos_p[i].position[1], instanceInfos_p[i].position[2]);
				// rotation is in radians already
				instance_p->SetRotation(XMVectorSet(instanceInfos_p[i].rotation[0], instanceInfos_p[i].rotation[1], instanceInfos_p[i].rotation[2], 0.0f));
				instance_p->SetScale(XMVectorSet(instanceInfos_p[i].scale[0], instanceInfos_p[i].scale[1], instanceInfos_p[i].scale[2], 0.0f));

				m_instanceList.push_back(instance_p);
				_sendInstanceReplyMessage(instance_p);
			}
			else
			{
				// Failed to add instance, send error message
				_sendInstanceFailedMessage(meshName);
			}
		}

		// TODO: clean up unused mesh
		delete[] buffer;
		break;
	}
	case MSG_TYPE_LOAD_TEXTURE:
	{
		std::string textureName = std::string((char*)msg.GetData());
		bool result = false;
		// check if texture already loaded

		auto texture = m_textureMap.find(textureName);
		if (texture != m_textureMap.end())
		{
			// texture with the same name already exists
			result = false;
		}
		else
		{
			result = ReadAndUploadTexture(textureName.c_str());
		}

		// send back texture load message
		Message* msgReply = new Message();
		if (result)
		{
			msgReply->type = MSG_TYPE_TEXTURE_SUCCESS;
		}
		else
		{
			// failed to load texture, send back default texture name
			msgReply->type = MSG_TYPE_TEXTURE_FAILED;
		}
		msgReply->SetData(textureName.c_str(), textureName.length() + 1); // SetData is always copying
		UIManager::Get().ReceiveMessage(msgReply);
		break;
	}
	case MSG_TYPE_REMOVE_INSTANCE:
	{
		// data is instance pointer
		if (dataSize != POINTER_SIZE)
		{
			// invalid data size
			break;
		}
		Instance* instanceToRemove = *(Instance**)(msg.GetData());
		auto iter = std::find(m_instanceList.begin(), m_instanceList.end(), instanceToRemove);
		if (iter != m_instanceList.end())
		{
			delete* iter;
			m_instanceList.erase(iter);
		}
		break;
	}
	case MSG_TYPE_CLEAN_MESHES:
	{
		// Clean all meshes
		CleanForLoad();
		break;
	}
	case MSG_TYPE_CPU_MEMORY_INFO:
	{
		// data is two size_t values: available memory and total memory
		if (dataSize != sizeof(uint64_t) * 2)
		{
			// invalid data size
			break;
		}
		char* msgData = (char*)msg.GetData();
		memcpy_s(m_memInfo, sizeof(uint64_t) * 2, msgData, sizeof(uint64_t) * 2);
		break;
	}
	case MSG_TYPE_MODIFY_LIGHT:
	{
		// data is LightData struct
		if (dataSize != sizeof(LightConstants))
		{
			// invalid data size
			break;
		}
		LightConstants* lightData = (LightConstants*)(msg.GetData());
		m_lightManager_p->CopyData(lightData);
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

void MeshManager::_sendMeshLoadSuccessMessage(const std::string& meshName)
{
	Message* msgReply = new Message();
	msgReply->type = MSG_TYPE_LOAD_SUCCESS;
	msgReply->SetData(meshName.c_str(), meshName.length() + 1); // SetData is always copying
	UIManager::Get().ReceiveMessage(msgReply);
}

void MeshManager::_sendInstanceReplyMessage(Instance* createdInstance)
{
	std::string instanceName = createdInstance->GetName();
	const char* instanceNameCStr = instanceName.c_str();
	const size_t instanceNameLength = instanceName.length();

	size_t dataLength = instanceName.length() + 1 + POINTER_SIZE; // one for null terminator

	unsigned char* replyDataBuffer = new unsigned char[dataLength];
	memcpy(replyDataBuffer, instanceNameCStr, instanceNameLength + 1);
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
	for (Instance* instance_p : m_instanceList)
	{
		delete instance_p;
	}
	m_instanceList.clear();
	m_createdInstanceCount = 0;

	// clean all textures
	// for now don't touch textures to avoid messing around SRV heap
	//for (auto texturePair : m_textureMap)
	//{
	//	delete texturePair.second;
	//}

	//while (m_meshes.begin() != m_meshes.end())
	//{
	//	auto meshIter = m_meshes.begin();
	//	delete meshIter->second;
	//	m_meshes.erase(meshIter);
	//}
}

void MeshManager::_sendMeshLoadFailedMessage(const std::string& meshName)
{
	Message* msgReply = new Message();
	msgReply->type = MSG_TYPE_MESH_LOAD_FAILED;
	msgReply->SetData(meshName.c_str(), meshName.length() + 1); // SetData is always copying
	UIManager::Get().ReceiveMessage(msgReply);
}

void MeshManager::_sendInstanceFailedMessage(const std::string& meshName)
{
	Message* msgReply = new Message();
	msgReply->type = MSG_TYPE_INSTANCE_FAILED;
	msgReply->SetData(meshName.c_str(), meshName.length() + 1); // SetData is always copying
	UIManager::Get().ReceiveMessage(msgReply);
}

void MeshManager::CreateDefaultTexture()
{
	m_textureMap["default_white"] = new Texture("default_white", 0);
}

bool MeshManager::ReadAndUploadTexture(const char* textureName)
{
	Texture* newTexture = new Texture(textureName, m_textureMap.size());

	m_textureMap[textureName] = newTexture;

	return true;
}

Texture* MeshManager::GetTextureByName(const std::string& textureName)
{
	if (auto textureIter = m_textureMap.find(textureName); textureIter != m_textureMap.end())
	{
		return textureIter->second;
	}
	else
	{
		// get default texture
		return m_textureMap["default_white"];
	}
}

Mesh* MeshManager::GetMeshByName(const std::string& meshName)
{
	if (auto meshIter = m_meshes.find(meshName); meshIter != m_meshes.end())
	{
		return meshIter->second;
	}
	else
	{
		return nullptr;
	}
}

void MeshManager::InitializeLightManager(XMFLOAT4& p_mainCameraLocation)
{
	if (!m_lightManager_p)
	{
		m_lightManager_p = new LightManager(p_mainCameraLocation);
	}
	else
	{
		m_lightManager_p->UpdateCameraPosition(p_mainCameraLocation);
	}
}

void MeshManager::SetSamplerIndex(unsigned int p_samplerIndex)
{
	m_selectedSampler = p_samplerIndex;
}