#pragma once
#include <queue>

enum MessageType : uint32_t
{
	MSG_TYPE_NONE = 0x0,
	MSG_TYPE_REQUIRE = 0x1,
	MSG_TYPE_REPLY = 0x2,
};

class Message
{
public:
	MessageType type = MSG_TYPE_NONE;

	Message() = default;

	~Message()
	{
		delete[] data;
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

	void GetData(void* outData, size_t outSize) const
	{
		if (outSize < size)
		{
			return;
			// Could use some error handling here.
		}

		std::memcpy(outData, data, size);
	}

	void Release()
	{
		delete[] data;
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
		while (!m_messageQueue.empty())
		{
			Message& msg = m_messageQueue.front();
			msg.Release();
			m_messageQueue.pop();
		}
	}

	void PushMessage(const Message& message)
	{
		m_messageQueue.push(message);
	}

	bool PopMessage(Message& message)
	{
		if (m_messageQueue.empty())
		{
			return false;
		}
		message = m_messageQueue.front();
		m_messageQueue.pop();
		return true;
	}
private:
	std::queue<Message> m_messageQueue;
};