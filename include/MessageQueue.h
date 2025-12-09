#pragma once
#include <queue>
#include <mutex>
#include <iostream>

enum MessageType : uint32_t
{
	MSG_TYPE_NONE = 0x0,
	MSG_TYPE_LOAD_MESH = 0x1, // content is mesh name, for example, sphere
	MSG_TYPE_LOAD_SUCCESS = 0x2, // reply from MeshManager to UIManager, content is mesh name
	MSG_TYPE_CREATE_INSTANCE = 0x3, // request to create instance of a mesh, content is mesh name
	MSG_TYPE_INSTANCE_REPLY = 0x4, // reply from MeshManager to UIManager, content is instance name + pointer
	MSG_TYPE_RELOAD_MESH = 0x5, // request to reload mesh with new instance data, content is ReloadInfo struct
	MSG_TYPE_CLEAN_MESHES = 0x6, // request to clean all meshes, no content
	MSG_TYPE_MESH_LOAD_FAILED = 0x7, // reply from MeshManager to UIManager, content is mesh name
	MSG_TYPE_INSTANCE_FAILED = 0x8, // reply from MeshManager to UIManager, content is mesh name
};

constexpr size_t POINTER_SIZE = sizeof(void*);

struct InstanceInfo
{
	float position[3];
	float rotation[3]; // in degrees
	float scale[3];
};

struct ReloadInfo
{
	char meshName[128];
	size_t numOfInstances;
	InstanceInfo instanceInfos; // this can have variable length/size but lease one is required
};

class Message
{
public:
	MessageType type = MSG_TYPE_NONE;

	Message() = default;

	Message& operator=(const Message& other)
	{
		type = other.type;
		data = other.data;
		size = other.size;
		return *this;
	}

	~Message()
	{
		// DEBUG
		std::cout << "DELETED MESSAGE" << std::endl;
	}

	size_t GetSize() const { return size; }

	size_t SetData(const void* inData, size_t inSize)
	{
		delete[] data;
		data = new unsigned char[inSize];
		if (!data)
		{
			size = 0;
			return 0;
			// Could use some error handling here.
		}
		std::memcpy(data, inData, inSize);
		size = inSize;
		return size;
	}

	void* GetData()
	{
		return data;
	}

	void Release()
	{
		if (data)
		{
			delete[] data;
		}
		data = nullptr;
		size = 0;
	}
private:
	unsigned char* data = nullptr;
	size_t size = 0;
};

class MessageQueue
{
public:
	MessageQueue() = default;

	~MessageQueue()
	{
	}

	void PushMessage(Message* message)
	{
		LockMutex();
		// critical region
		m_messageQueue.push(message);
		UnlockMutex();
	}

	bool PopMessage(Message& message)
	{
		if (!m_messageQueue.empty())
		{
			// critical region
			LockMutex();
			message = *(m_messageQueue.front());

			delete m_messageQueue.front();
			m_messageQueue.pop();

			UnlockMutex();
			return true;
		}

		return false;
	}

	void LockMutex()
	{
		m_mutex.lock();
	}

	void UnlockMutex()
	{
		m_mutex.unlock();
	}

private:
	std::queue<Message*> m_messageQueue;
	std::mutex m_mutex;
};