#include <Mesh.h>
#include <Application.h>
#include <Helpers.h>
#include <CommandQueue.h>
#include <regex>

#include <DirectXMath.h>
using namespace DirectX;

#include <sstream>
#include <fstream>

// DEBUG INPUT
static VertexPosColor g_Vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f)}, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 2.0f), XMFLOAT2(0.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) }, // 5
	{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) }  // 7
};

static uint32_t g_Indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

Mesh::Mesh(const wchar_t* p_objFilePath)
{
	// read file
	LoadOBJFile(p_objFilePath);
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

void Mesh::LoadOBJFile(const wchar_t* p_objFilePath)
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
	std::regex delimiter(" ");

	while (std::getline(objFile, line))
	{
		if (line.size() < 2)
		{
			// ill-formatted line, next
			continue;
		}

		std::sregex_token_iterator iter(line.begin(), line.end(), delimiter, -1);
		std::sregex_token_iterator end;
		int v[4];
		int vt[4];
		int vn[4];
		int i = 0;

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
				m_vertices.push_back(vx);
				m_vertices.push_back(vy);
				m_vertices.push_back(vz);
				break;
			case 't':
				// texture coordinate
				float tu, tv;
				// "vt " has three characters, including the space
				sscanf_s(&line.c_str()[3], "%f %f", &tu, &tv);
				m_texcoords.push_back(tu);
				m_texcoords.push_back(tv);
				break;
			case 'n':
				// read vertex normal
				float nx, ny, nz;
				// "vn " has three characters, including the space 
				sscanf_s(&line.c_str()[3], "%f %f %f", &nx, &ny, &nz);
				m_normals.push_back(nx);
				m_normals.push_back(ny);
				m_normals.push_back(nz);
				break;
			}
			break;
		case 'f':
			// read faces
			// currently only work with polygons without vt, i.e. we expect "xxxx/yyyy/zzzz" and only three vertices per line
			i = 0;
			while (iter != end)
			{
				std::string vertexDef = *iter;
				if (vertexDef.size() > 0 && vertexDef[0] != 'f')
				{
					sscanf_s(vertexDef.c_str(), "%d/%d/%d", &v[i], &vt[i], &vn[i]);
					i++;
				}
				++iter;
			}
			if (i == 3)
			{
				// triangle
				m_triangles.push_back(v[0] - 1);
				m_triangles.push_back(v[1] - 1);
				m_triangles.push_back(v[2] - 1);
				m_triangleNormalIndex.push_back(vn[0] - 1);
				m_triangleNormalIndex.push_back(vn[1] - 1);
				m_triangleNormalIndex.push_back(vn[2] - 1);
				m_triangleTexcoordIndex.push_back(vt[0] - 1);
				m_triangleTexcoordIndex.push_back(vt[1] - 1);
				m_triangleTexcoordIndex.push_back(vt[2] - 1);
				break;
			}
			else if (i == 4)
			{
				// quad, need to split into two triangles
				m_triangles.push_back(v[0] - 1);
				m_triangles.push_back(v[1] - 1);
				m_triangles.push_back(v[2] - 1);
				m_triangleNormalIndex.push_back(vn[0] - 1);
				m_triangleNormalIndex.push_back(vn[1] - 1);
				m_triangleNormalIndex.push_back(vn[2] - 1);
				m_triangleTexcoordIndex.push_back(vt[0] - 1);
				m_triangleTexcoordIndex.push_back(vt[1] - 1);
				m_triangleTexcoordIndex.push_back(vt[2] - 1);
				m_triangles.push_back(v[0] - 1);
				m_triangles.push_back(v[2] - 1);
				m_triangles.push_back(v[3] - 1);
				m_triangleNormalIndex.push_back(vn[0] - 1);
				m_triangleNormalIndex.push_back(vn[2] - 1);
				m_triangleNormalIndex.push_back(vn[3] - 1);
				m_triangleTexcoordIndex.push_back(vt[0] - 1);
				m_triangleTexcoordIndex.push_back(vt[2] - 1);
				m_triangleTexcoordIndex.push_back(vt[3] - 1);
				break;
			}
			else
			{
				// unsupported polygon, ignore
			}
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
	auto noOfTriangles = m_triangles.size() / 3;
	for (auto i = 0; i < noOfTriangles; i++)
	{
		// position, normal, texcoord
		auto vi1 = m_triangles[i * 3];
		auto vi2 = m_triangles[i * 3 + 1];
		auto vi3 = m_triangles[i * 3 + 2];

		auto vni1 = m_triangleNormalIndex[i * 3];
		auto vni2 = m_triangleNormalIndex[i * 3 + 1];
		auto vni3 = m_triangleNormalIndex[i * 3 + 2];

		auto vti1 = m_triangleTexcoordIndex[i * 3];
		auto vti2 = m_triangleTexcoordIndex[i * 3 + 1];
		auto vti3 = m_triangleTexcoordIndex[i * 3 + 2];

		combinedBuffer.push_back(VertexPosColor(
			{ m_vertices[vi1 * 3], m_vertices[vi1 * 3 + 1] , m_vertices[vi1 * 3 + 2] },
			{ m_normals[vni1 * 3], m_normals[vni1 * 3 + 1] , m_normals[vni1 * 3 + 2] },
			{ m_texcoords[vti1 * 2], m_texcoords[vti1 * 2 + 1] }));

		combinedBuffer.push_back(VertexPosColor(
			{ m_vertices[vi2 * 3], m_vertices[vi2 * 3 + 1] , m_vertices[vi2 * 3 + 2] },
			{ m_normals[vni2 * 3], m_normals[vni2 * 3 + 1] , m_normals[vni2 * 3 + 2] },
			{ m_texcoords[vti2 * 2], m_texcoords[vti2 * 2 + 1] }));

		combinedBuffer.push_back(VertexPosColor(
			{ m_vertices[vi3 * 3], m_vertices[vi3 * 3 + 1] , m_vertices[vi3 * 3 + 2] },
			{ m_normals[vni3 * 3], m_normals[vni3 * 3 + 1] , m_normals[vni3 * 3 + 2] },
			{ m_texcoords[vti3 * 2], m_texcoords[vti3 * 2 + 1] }));
	}

	m_triangles.clear();
	for (auto i = 0; i < combinedBuffer.size(); i++)
	{
		m_triangles.push_back(i);
	}

	// Simple debug cube
	//combinedBuffer.insert(combinedBuffer.end(), std::begin(g_Vertices), std::end(g_Vertices));
	//m_triangles.clear();
	//m_triangles.insert(m_triangles.end(), std::begin(g_Indicies), std::end(g_Indicies));

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
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = m_triangles.size() * sizeof(uint32_t);

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

	// record number of triangles before release
	m_triangleCount = static_cast<UINT>(m_triangles.size());

	// release on CPU memory
	m_vertices.clear();
	m_normals.clear();
	m_texcoords.clear();

	m_triangles.clear();
	m_triangleNormalIndex.clear();
	m_triangleTexcoordIndex.clear();

	combinedBuffer.clear();
	
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

	p_commandList->SetPipelineState(pipelineState.Get());
	p_commandList->SetGraphicsRootSignature(rootSignature.Get());

	p_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	p_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	p_commandList->IASetIndexBuffer(&m_indexBufferView);

	// Update the MVP matrix
	XMMATRIX mvpMatrix = XMMatrixMultiply(m_modelMatrix, m_viewMatrix);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, m_projectionMatrix);
	p_commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

	p_commandList->DrawIndexedInstanced(m_triangleCount, 1, 0, 0, 0);
}