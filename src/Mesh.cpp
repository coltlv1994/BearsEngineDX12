#include <Mesh.h>
#include <Application.h>
#include <Helpers.h>
#include <CommandQueue.h>

#include <DirectXMath.h>
using namespace DirectX;

#include <sstream>
#include <fstream>

Mesh::Mesh(const wchar_t* p_objFilePath)
{
	// read file
	LoadOBJFile(p_objFilePath, m_vertices, m_normals, m_triangles, m_triangleNormalIndex);
}

void Mesh::SetModelMatrix(XMMATRIX& p_modelMatrix)
{
	m_modelMatrix = p_modelMatrix;
}

void Mesh::SetViewMatrix(XMMATRIX& p_viewMatrix)
{
	m_viewMatrix = p_viewMatrix;
}

void Mesh::SetProjectionMatrix(XMMATRIX& p_projectionMatrix)
{
	m_projectionMatrix = p_projectionMatrix;
}

void Mesh::SetFOV(float p_fov)
{
	m_fov = p_fov;
}

void LoadOBJFile(
	const wchar_t* p_objFilePath,
	std::vector<float>& p_vertices,
	std::vector<float>& p_normals,
	std::vector<uint32_t>& p_triangles,
	std::vector<uint32_t>& p_triangleNormalIndex)
{
	/*
	* TODO:
	* 1. Make an individual thread and avoid unnecessary wait on main thread.
	* 2. Handle more formats (use regex or other implementation since sscanf
	*    cannot work with variable number of input format.
	*/
	std::ifstream objFile(p_objFilePath);

	if (!objFile.is_open())
	{
		// open failed
		exit(1);
	}

	std::string line;

	while (std::getline(objFile, line))
	{
		if (line.size() < 2)
		{
			// ill-formatted line, next
			continue;
		}

		switch (line[0])
		{
		case 'v':
			// need determine next char
			switch (line[1])
			{
			case ' ':
				// read vertex
				float vx, vy, vz;
				sscanf_s(&line.c_str()[2], "%f %f %f", &vx, &vy, &vz);
				p_vertices.push_back(vx);
				p_vertices.push_back(vy);
				p_vertices.push_back(vz);
				break;
			case 't':
				// texture coordinate
				break;
			case 'n':
				// read vertex normal
				float nx, ny, nz;
				// "vn " has three characters, including the space 
				sscanf_s(&line.c_str()[3], "%f %f %f", &nx, &ny, &nz);
				p_normals.push_back(nx);
				p_normals.push_back(ny);
				p_normals.push_back(nz);
				break;
			}
			break;
		case 'f':
			// read faces
			// currently only work with polygons without vt, i.e. we expect "xxxx//yyyy" and only four vertices per line
			int v1, v2, v3, v4, vn1, vn2, vn3, vn4;
			sscanf_s(&line.c_str()[2], "%d//%d %d//%d %d//%d %d//%d", &v1, &v2, &v3, &v4, &vn1, &vn2, &vn3, &vn4);
			// break down to two triangles
			// obj file uses counter-clockwise winding order and we will rotate to clockwise for D3D12
			p_triangles.push_back(v1 - 1);
			p_triangles.push_back(v4 - 1);
			p_triangles.push_back(v3 - 1);
			p_triangles.push_back(v1 - 1);
			p_triangles.push_back(v3 - 1);
			p_triangles.push_back(v2 - 1);
			p_triangleNormalIndex.push_back(v1 - 1);
			p_triangleNormalIndex.push_back(v4 - 1);
			p_triangleNormalIndex.push_back(v3 - 1);
			p_triangleNormalIndex.push_back(v1 - 1);
			p_triangleNormalIndex.push_back(v3 - 1);
			p_triangleNormalIndex.push_back(v2 - 1);
			break;
		case 'm': // TODO
		case '#': // comments, ignore
		default:
			continue; // next line
		}
	}
	objFile.close();
}

void Mesh::UseShader(Shader* shader_p)
{
	m_shader_p = shader_p;
}

void Mesh::LoadDataToGPU()
{
	// combine buffers
	auto noOfVertices = m_vertices.size() / 3;
	for (auto i = 0; i < noOfVertices; i++)
	{
		combinedBuffer.push_back(VertexPosColor({m_vertices[i * 3], m_vertices[i * 3 + 1] , m_vertices[i * 3+2] }, {0.0f, 0.0f, 0.0f}, {0.0f , 0.0f}));
	}

	// prepare upload
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	// Upload vertex buffer data.
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	UpdateBufferResource(commandList,
		&m_vertexBuffer, &intermediateVertexBuffer,
		combinedBuffer.size(), sizeof(VertexPosColor), combinedBuffer.data());

	// Create the vertex buffer view.
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = combinedBuffer.size() * sizeof(VertexPosColor);
	m_vertexBufferView.StrideInBytes = sizeof(VertexPosColor);

	// Upload index buffer data.
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	UpdateBufferResource(commandList,
		&m_indexBuffer, &intermediateIndexBuffer,
		m_triangles.size(), sizeof(uint32_t), m_triangles.data());

	// Create index buffer view.
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_indexBufferView.SizeInBytes = m_triangles.size() * sizeof(uint32_t);

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);
}

void Mesh::UpdateBufferResource(
	ComPtr<ID3D12GraphicsCommandList2> commandList,
	ID3D12Resource** pDestinationResource,
	ID3D12Resource** pIntermediateResource,
	size_t numElements, size_t elementSize, const void* bufferData,
	D3D12_RESOURCE_FLAGS flags)
{
	auto device = Application::Get().GetDevice();

	size_t bufferSize = numElements * elementSize;

	static CD3DX12_HEAP_PROPERTIES heap_default = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	static CD3DX12_HEAP_PROPERTIES heap_upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC buffer_default = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
	CD3DX12_RESOURCE_DESC buffer_upload = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	// Create a committed resource for the GPU resource in a default heap.
	ThrowIfFailed(device->CreateCommittedResource(
		&heap_default,
		D3D12_HEAP_FLAG_NONE,
		&buffer_default,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pDestinationResource)));

	// Create an committed resource for the upload.
	if (bufferData)
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&heap_upload,
			D3D12_HEAP_FLAG_NONE,
			&buffer_upload,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pIntermediateResource)));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(commandList.Get(),
			*pDestinationResource, *pIntermediateResource,
			0, 0, 1, &subresourceData);
	}
}

void Mesh::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList2> p_commandList, float deltaTime)
{
	ComPtr<ID3D12RootSignature> rootSignature = m_shader_p->GetRootSigniture();
	ComPtr<ID3D12PipelineState> pipelineState = m_shader_p->GetPipelineState();

	// Pass as parameter
	//auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	//auto commandList = commandQueue->GetCommandList();
	// Clear the render targets.
	{
		// This part should be in main thread
	}

	p_commandList->SetPipelineState(pipelineState.Get());
	p_commandList->SetGraphicsRootSignature(rootSignature.Get());

	p_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	p_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	p_commandList->IASetIndexBuffer(&m_indexBufferView);

	// Update the MVP matrix
	XMMATRIX mvpMatrix = XMMatrixMultiply(m_modelMatrix, m_viewMatrix);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, m_projectionMatrix);
	p_commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

	p_commandList->DrawIndexedInstanced(m_triangles.size(), 1, 0, 0, 0);

	// Present
	{
		// This part should be in main thread
	}
}