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
	Mesh(const wchar_t* p_objFilePath);
	Mesh(const wchar_t* p_objFilePath, const wchar_t* p_textureFilePath);
	~Mesh();
	void LoadOBJFile(const wchar_t* p_objFilePath);
	void UseShader(Shader* shader_p);
	void LoadDataToGPU();
	void ReadFromBinaryFile(const wchar_t* p_binFilePath);
	void WriteToBinaryFile(const wchar_t* p_binFilePath);
	void SetMeshClassName(const std::wstring& meshClassName);
	const std::wstring& GetMeshClassName();

	void SetModelMatrix(XMMATRIX& p_modelMatrix);
	void SetViewMatrix(XMMATRIX& p_viewMatrix);
	void SetProjectionMatrix(XMMATRIX& p_projectionMatrix);
	void SetFOV(float p_fov);

	// Render work
	void PopulateCommandList(ComPtr<ID3D12GraphicsCommandList2> p_commandList);

	void RenderInstances(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix);

	// Instance management
	bool AddInstance();
	bool RemoveInstance(const std::wstring& instanceName);
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

	// 3D matrices
	/*
	* (Explanation keeps here for reference)
	* The "model" matrix is one that takes your object from it's "local" space,
	  and positions it in the "world" you are viewing. Generally it just
	  modifies it's position, rotation and scale.

	  The "view" matrix is the position and orientation of your camera that views
	  the world. The inverse of this is used to take objects that are in the world,
	  and move everything such that the camera is at the origin, looking down
	  the Z (or -Z depending on convention) axis. This is done so that the the next steps are easier and simpler.

	  The "projection" matrix is a bit special, in that it is no longer
	  a simple rotate/translate, but also scales objects based on distance,
	  and "pulls in" objects so that everything in the frustum is contained inside a unit-cube of space.
	*/
	float m_fov;
	XMMATRIX m_modelMatrix = XMMatrixIdentity(); // position, rotaion, scale in *local* space
	XMMATRIX m_viewMatrix = XMMatrixIdentity(); // view and projection matrices can be set in Camera class later
	XMMATRIX m_projectionMatrix = XMMatrixIdentity();

	const wchar_t* m_textureFilePath = nullptr;

	// Create a GPU buffer.
	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	unsigned char* ReadAndUploadTexture();
};
