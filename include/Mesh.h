#pragma once
#include <CommonHeaders.h>
#include <Shader.h>
#include <Renderable.h>
#include <vector>

using namespace DirectX;

class Mesh
{
public:
	Mesh(const wchar_t* p_objFilePath);
	void UseShader(Shader* shader_p);
	Shader& GetShaderObjectByRef();
	ComPtr<ID3D12GraphicsCommandList2> PopulateCommandList();
	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	void SetModelMatrix(XMMATRIX& p_modelMatrix);
	void SetViewMatrix(XMMATRIX& p_viewMatrix);
	void SetProjectionMatrix(XMMATRIX& p_projectionMatrix);
	void SetFOV(float p_fov);

	// DEBUG
	void PopulateCommandList(ComPtr<ID3D12GraphicsCommandList2> p_commandList);

private:
	std::vector<float> m_vertices;
	std::vector<float> m_normals;
	std::vector<uint32_t> m_triangles;
	std::vector<uint32_t> m_triangleNormalIndex;
	
	ComPtr<ID3D12GraphicsCommandList2> m_commandList;
	// Vertex buffer for the mesh.
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	// Index buffer for the mesh.
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	ComPtr<ID3D12Device2> m_device; // save all these
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;

	// Depth buffer.
	ComPtr<ID3D12Resource> m_depthBuffer;
	// Descriptor heap for depth buffer.
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

	Shader* m_shader_p;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

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

	void _updateBufferResources();
	void _resizeDepthBuffer(int width, int height);
};
