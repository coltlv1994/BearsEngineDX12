#include <EntityManager.h>

#include <Application.h>

#include <vector>

EntityManager::~EntityManager()
{
	for (Mesh* mesh : m_entities)
	{
		delete mesh;
	}
}

void EntityManager::AddEntity(Mesh* entity)
{
	m_entities.push_back(entity);
}

void EntityManager::AddEntity(const wchar_t* p_objFilePath)
{
	//std::vector<Mesh*> m_entities;

	Mesh* newMesh = new Mesh(p_objFilePath);
	AddEntity(newMesh);
}

void EntityManager::Render(std::vector<ComPtr<ID3D12GraphicsCommandList2>>& p_commandLists)
{
	for (auto& en : m_entities)
	{
		p_commandLists.push_back(en->PopulateCommandList());
	}

	//ComPtr<ID3D12Resource> currentBackBuffer = app.GetCurrentBackBuffer();
	//ComPtr<ID3D12CommandQueue> commandQueue = app.GetCommandQueue();

	//for (auto& commandList : commandLists)
	//{
	//	CD3DX12_RESOURCE_BARRIER barrier =
	//		CD3DX12_RESOURCE_BARRIER::Transition(
	//			currentBackBuffer.Get(),
	//		    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	//	commandList->ResourceBarrier(1, &barrier);

	//	commandList->Close();

	//	ID3D12CommandAllocator* commandAllocator;
	//	UINT dataSize = sizeof(commandAllocator);
	//	ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

	//	ID3D12CommandList* const ppCommandLists[] = {
	//		commandList.Get()
	//	};

	//	commandQueue->ExecuteCommandLists(1, ppCommandLists);
	//	uint64_t fenceValue = app.GetFenceValueForCurrentBackBuffer() + 1;
	//	app.SignalCommandQueue(fenceValue);

	//	m_CommandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
	//	m_CommandListQueue.push(commandList);

	//	// The ownership of the command allocator has been transferred to the ComPtr
	//	// in the command allocator queue. It is safe to release the reference 
	//	// in this temporary COM pointer here.
	//	commandAllocator->Release();

	//	app.SetFenceValueArrayForCurrentBackBuffer(fenceValue);

	//	currentBackBufferIndex = m_pWindow->Present();

	//	commandQueue->WaitForFenceValue(m_FenceValues[currentBackBufferIndex]);
	//}
}