#include <Mesh.h>
#include <Application.h>
#include <Helpers.h>
#include <CommandQueue.h>
#include <regex>
#include <openssl/sha.h>
#include <fstream>
#include <cstdlib>

#include <DirectXMath.h>
using namespace DirectX;

#include <sstream>

bool Mesh::Initialize(const wchar_t* p_objFilePath)
{
	// check if file exists
	std::ifstream objFile(p_objFilePath);
	if (!objFile.is_open())
	{
		// open failed
		return false;
	}

	// read file
	LoadOBJFile(p_objFilePath);

	return true;
}

void Mesh::SetMeshClassName(const std::string& meshClassName)
{
	m_meshClassName = meshClassName;
}

const std::string& Mesh::GetMeshClassName()
{
	return m_meshClassName;
}

void Mesh::LoadOBJFile(const wchar_t* p_objFilePath)
{
	// check if binary file exists
	std::wstring binFilePathStr(p_objFilePath);
	binFilePathStr += L".bin";
	std::ifstream binFile(binFilePathStr, std::ios::binary | std::ios::in);
	if (binFile.is_open())
	{
		// binary file exists, read from it
		binFile.close();
		ReadFromBinaryFile(binFilePathStr.c_str());
	}
	else
	{
		// binary file does not exist, parse the obj file
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
		// buffers for faces
		int v[128];
		int vt[128];
		int vn[128];
		int noOfVerticesInFace = 0;

		while (std::getline(objFile, line))
		{
			if (line.size() < 2)
			{
				// ill-formatted line, next
				continue;
			}

			std::sregex_token_iterator iter(line.begin(), line.end(), delimiter, -1);
			std::sregex_token_iterator end;

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
					// OBJ file starts at bottom-left, but DX12 starts at top-left
					m_texcoords.push_back(1.0f - tv);
					break;
				case 'n':
					// read vertex normal
					float nx, ny, nz;
					//// "vn " has three characters, including the space
					sscanf_s(&line.c_str()[3], "%f %f %f", &nx, &ny, &nz);
					m_normals.push_back(nx);
					m_normals.push_back(ny);
					m_normals.push_back(nz);
					break;
				default: // ill-formatted line, ignore
					break;
				}
				break;
			case 'f':
				// read faces
				// currently only work with polygons without vt, i.e. we expect "xxxx/yyyy/zzzz" and only three vertices per line
				noOfVerticesInFace = 0;
				while (iter != end)
				{
					std::string vertexDef = *iter;
					if (vertexDef.size() > 0 && vertexDef[0] != 'f')
					{
						sscanf_s(vertexDef.c_str(), "%d/%d/%d", &v[noOfVerticesInFace], &vt[noOfVerticesInFace], &vn[noOfVerticesInFace]);
						noOfVerticesInFace++;
					}
					++iter;
				}
				if (noOfVerticesInFace < 3 || noOfVerticesInFace > 128)
				{
					// invalid face definition, ignore
					break;
				}
				else
				{
					//break them into triangles
					int startIndex = 0;
					int currentIndex = 1;
					int currentIndexPlusOne = 2;
					while (currentIndexPlusOne < noOfVerticesInFace)
					{
						m_triangles.push_back(v[startIndex] - 1);
						m_triangles.push_back(v[currentIndex] - 1);
						m_triangles.push_back(v[currentIndexPlusOne] - 1);
						m_triangleNormalIndex.push_back(vn[startIndex] - 1);
						m_triangleNormalIndex.push_back(vn[currentIndex] - 1);
						m_triangleNormalIndex.push_back(vn[currentIndexPlusOne] - 1);
						m_triangleTexcoordIndex.push_back(vt[startIndex] - 1);
						m_triangleTexcoordIndex.push_back(vt[currentIndex] - 1);
						m_triangleTexcoordIndex.push_back(vt[currentIndexPlusOne] - 1);
						currentIndex++;
						currentIndexPlusOne++;
					}
				}
				break;
			case 'm': // TODO
			case '#': // comments, ignore
			default:
				continue; // next line
			}
		}
		objFile.close();

		// integrity check
		size_t numOfVertices = m_vertices.size() / 3;
		size_t numOfNormals = m_normals.size() / 3;
		size_t numOfTexcoords = m_texcoords.size() / 2;
		unsigned int maxVertexIndex = *std::max_element(m_triangles.begin(), m_triangles.end());
		unsigned int maxNormalIndex = *std::max_element(m_triangleNormalIndex.begin(), m_triangleNormalIndex.end());
		unsigned int maxTexcoordIndex = *std::max_element(m_triangleTexcoordIndex.begin(), m_triangleTexcoordIndex.end());

		// combine buffers
		auto noOfVertices = m_vertices.size() / 3;
		auto noOfTriangles = m_triangles.size() / 3;
		const float* verticesData = m_vertices.data();
		const float* normalsData = m_normals.data();
		const float* texcoordsData = m_texcoords.data();

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

			XMFLOAT3 position1 = XMFLOAT3(&verticesData[vi1 * 3]);
			XMFLOAT3 position2 = XMFLOAT3(&verticesData[vi2 * 3]);
			XMFLOAT3 position3 = XMFLOAT3(&verticesData[vi3 * 3]);

			XMFLOAT3 normal1 = XMFLOAT3(&normalsData[vni1 * 3]);
			XMFLOAT3 normal2 = XMFLOAT3(&normalsData[vni2 * 3]);
			XMFLOAT3 normal3 = XMFLOAT3(&normalsData[vni3 * 3]);

			XMFLOAT2 texcoord1 = XMFLOAT2(&texcoordsData[vti1 * 2]);
			XMFLOAT2 texcoord2 = XMFLOAT2(&texcoordsData[vti2 * 2]);
			XMFLOAT2 texcoord3 = XMFLOAT2(&texcoordsData[vti3 * 2]);

			float deltaU1 = texcoord2.x - texcoord1.x;
			float deltaU2 = texcoord3.x - texcoord1.x;
			float deltaV1 = texcoord2.y - texcoord1.y;
			float deltaV2 = texcoord3.y - texcoord1.y;

			float invDetDelta = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

			XMVECTOR p1v = XMLoadFloat3(&position1);
			XMVECTOR p2v = XMLoadFloat3(&position2);
			XMVECTOR p3v = XMLoadFloat3(&position3);
			XMVECTOR e1 = p2v - p1v;
			XMVECTOR e2 = p3v - p1v;

			//XMVECTOR T_VECTOR = (deltaV2 * (p2v - p1v) - deltaV1 * (p3v - p1v)) * invDetDelta;
			XMVECTOR T_VECTOR = (deltaV2 * p2v - deltaV1 * p3v - (deltaV2 - deltaV1) * p1v) * invDetDelta;

			XMFLOAT3 tVector;
			XMStoreFloat3(&tVector, T_VECTOR);

			// Tangent will be calculated later
			combinedBuffer.push_back(FirstPassVertexData(
				position1, normal1, tVector, texcoord1));

			combinedBuffer.push_back(FirstPassVertexData(
				position2, normal2, tVector, texcoord2));

			combinedBuffer.push_back(FirstPassVertexData(
				position3, normal3, tVector, texcoord3));
		}

		m_triangles.clear();
		for (auto i = 0; i < combinedBuffer.size(); i++)
		{
			m_triangles.push_back(i);
		}

		m_vertices.clear();
		m_normals.clear();
		m_texcoords.clear();
		m_triangleNormalIndex.clear();
		m_triangleTexcoordIndex.clear();

		// write to binary file for future use
		WriteToBinaryFile(binFilePathStr.c_str());
	}

	LoadDataToGPU();
}

void Mesh::LoadDataToGPU()
{
	// prepare upload
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	// Upload vertex buffer data.
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	UpdateBufferResource(commandList,
		&m_vertexBuffer, &intermediateVertexBuffer,
		combinedBuffer.size(), sizeof(FirstPassVertexData), combinedBuffer.data());

	// Create the vertex buffer view.
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = combinedBuffer.size() * sizeof(FirstPassVertexData);
	m_vertexBufferView.StrideInBytes = sizeof(FirstPassVertexData);

	// Upload index buffer data.
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	UpdateBufferResource(commandList,
		&m_indexBuffer, &intermediateIndexBuffer,
		m_triangles.size(), sizeof(uint32_t), m_triangles.data());

	// Create index buffer view.
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = m_triangles.size() * sizeof(uint32_t);

	//unsigned char* data = ReadAndUploadTexture();

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

	// record number of triangles before release
	m_triangleCount = static_cast<UINT>(m_triangles.size());

	// release on CPU memory
	m_triangles.clear();
	combinedBuffer.clear();
	//stbi_image_free(data);
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

void Mesh::ReadFromBinaryFile(const wchar_t* p_binFilePath)
{
	// need to read index list and combined buffer from binary file
	// binary filename should be sha1(objfilepath) + ".bin"
	std::ifstream binFile(p_binFilePath, std::ios::binary | std::ios::in);
	if (!binFile.is_open())
	{
		// open failed
		exit(1);
	}
	std::string line;
	std::getline(binFile, line);
	size_t vertexCount = 0;
	sscanf_s(line.c_str(), "vertices: %zu", &vertexCount);
	combinedBuffer.resize(vertexCount);
	binFile.read(reinterpret_cast<char*>(combinedBuffer.data()), vertexCount * sizeof(FirstPassVertexData));
	std::getline(binFile, line); // read the newline
	std::getline(binFile, line);
	size_t indexCount = 0;
	sscanf_s(line.c_str(), "indices: %zu", &indexCount);
	m_triangles.resize(indexCount);
	binFile.read(reinterpret_cast<char*>(m_triangles.data()), indexCount * sizeof(uint32_t));
	binFile.close();
}

void Mesh::WriteToBinaryFile(const wchar_t* p_binFilePath)
{
	// need to write index list and combined buffer to binary file
	// binary filename should be sha1(objfilepath) + ".bin"
	std::ofstream binFile(p_binFilePath, std::ios::binary | std::ios::out);

	if (!binFile.is_open())
	{
		// open failed
		exit(1);
	}

	binFile << "vertices: " << combinedBuffer.size() << "\n";
	binFile.write(reinterpret_cast<const char*>(combinedBuffer.data()), combinedBuffer.size() * sizeof(FirstPassVertexData));
	binFile << "\nindices: " << m_triangles.size() << "\n";
	binFile.write(reinterpret_cast<const char*>(m_triangles.data()), m_triangles.size() * sizeof(uint32_t));
	binFile.close();
}

void Mesh::RenderInstance(ComPtr<ID3D12GraphicsCommandList2> p_commandList)
{
	p_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	p_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	p_commandList->IASetIndexBuffer(&m_indexBufferView);
	p_commandList->DrawIndexedInstanced(m_triangleCount, 1, 0, 0, 0);
}