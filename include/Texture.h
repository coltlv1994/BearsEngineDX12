#pragma once
#include <string>

#include <Helpers.h>
#include <Application.h>
#include <CommandQueue.h>
#include <d3d12.h>
#include <d3dx12.h>

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

class Texture
{
public:
	Texture(const std::string& name, unsigned int p_textureIndex, ComPtr<ID3D12DescriptorHeap> p_srvHeap)
	{
		m_name = name;
		m_SRVHeap = p_srvHeap;
		m_textureIndex = p_textureIndex;

		// load textures
		_initialize();
	}

	~Texture()
	{
	}

	const std::string& GetName() const { return m_name; }

	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHeapStart()
	{
		static UINT srvSize = Application::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		static D3D12_GPU_DESCRIPTOR_HANDLE srvBaseHandle = m_SRVHeap->GetGPUDescriptorHandleForHeapStart();

		// initial offset should be 11
		// 1 for imgui, 3 RTV-SRV per 3 backbuffers, 1 for depth buffer SRV
		// 1 + 3 * 3 + 1 = 11
		UINT offset = m_textureIndex * 3 + 11;

		return CD3DX12_GPU_DESCRIPTOR_HANDLE(srvBaseHandle, offset, srvSize);
	}

private:
	std::string m_name;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SRVHeap; // passed from mesh manager

	unsigned int m_textureIndex = 0; // to calculate the offset in SRV heap

	void _initialize();
	bool _loadTexture(const std::string& textureFilePath, UINT p_srvHeapIndex);
};