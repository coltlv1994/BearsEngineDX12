#pragma once
#include <Shader.h>
#include <vector>
#include <Helpers.h>
#include <map>
#include <EntityInstance.h>

using namespace DirectX;

// Global function definition

class Mesh
{
public:
	~Mesh();
	bool Initialize(const wchar_t* p_objFilePath, const wchar_t* p_textureFilePath = L"textures\\2k_earth_daymap.jpg");
	void LoadOBJFile(const wchar_t* p_objFilePath);
	void UseShader(Shader* shader_p);
	void LoadDataToGPU();
	void ReadFromBinaryFile(const wchar_t* p_binFilePath);
	void WriteToBinaryFile(const wchar_t* p_binFilePath);
	void SetMeshClassName(const std::wstring& meshClassName);
	const std::wstring& GetMeshClassName();

	// Render work
	void PopulateCommandList(ComPtr<ID3D12GraphicsCommandList2> p_commandList);

	void RenderInstances(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix);

	// Instance management
	Instance* AddInstance();
	bool RemoveInstance(Instance* instance_p);
	void ClearInstances();

private:
	std::wstring m_meshClassName;
	std::vector<float> m_vertices;
	std::vector<float> m_normals;
	std::vector<float> m_texcoords;

	std::vector<uint32_t> m_triangles;
	std::vector<uint32_t> m_triangleNormalIndex;
	std::vector<uint32_t> m_triangleTexcoordIndex;

	std::vector<VertexPosColor> combinedBuffer;

	std::vector<Instance*> m_instances;

	UINT m_triangleCount = 0;

	// Vertex buffer for the mesh.
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	// Index buffer for the mesh.
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	// Depth buffer.
	ComPtr<ID3D12Resource> m_depthBuffer;
	// Descriptor heap for depth buffer.
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

	// Texture resources
	ComPtr<ID3D12Resource> m_texture;

	Shader* m_shader_p;

	const wchar_t* m_textureFilePath = nullptr;

	// Create a GPU buffer.
	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	unsigned char* ReadAndUploadTexture();
};
