#include <MeshManager.h>
#include <chrono>
#include <thread>
#include <UIManager.h>
#include <EntityInstance.h>

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
		if (texturePath == nullptr)
		{
			createResult = newMesh->Initialize(meshFullPath);
		}
		else
		{
			createResult = newMesh->Initialize(meshFullPath, texturePath);
		}

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

void MeshManager::RenderAllMeshes(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix)
{
	for (auto meshClass : m_meshes)
	{
		meshClass.second->RenderInstances(p_commandList, p_vpMatrix);
	}
}

void MeshManager::AddOneInstanceToMesh_DEBUG()
{
	for (auto& item : m_meshes)
	{
		if (item.second->AddInstance())
		{
			return;
		}
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
	if (dataSize == 0)
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

		// TODO: use different shader and texture if specified in message
		// texturePath is now fixed in Mesh constructor
		if (AddMesh(meshNameWChar, m_defaultShader_p, nullptr))
		{
			// Successfully loaded mesh
			// Send back success message
			Message* msgReply = new Message();
			msgReply->type = MSG_TYPE_LOAD_SUCCESS;
			msgReply->SetData(meshName, dataSize); // SetData is always copying
			UIManager::Get().ReceiveMessage(msgReply);
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
		}
		delete[] meshNameWChar;
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