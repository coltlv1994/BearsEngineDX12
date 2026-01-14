#pragma once
#include <string>

#include <Helpers.h>
#include <Application.h>
#include <d3dx12.h>

class Material
{
public:
		Material(const std::string& name)
			: m_name(name)
		{
			m_materialCBSize = CalcConstantBufferByteSize(sizeof(MaterialConstants));

			auto device = Application::Get().GetDevice();
			static CD3DX12_HEAP_PROPERTIES heap_upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			static CD3DX12_RESOURCE_DESC buffer_upload = CD3DX12_RESOURCE_DESC::Buffer(m_materialCBSize);

			// upload and map
			ThrowIfFailed(device->CreateCommittedResource(
				&heap_upload,
				D3D12_HEAP_FLAG_NONE,
				&buffer_upload,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_uploadBuffer)));

			ThrowIfFailed(m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData)));

			// initialize the data
			MaterialConstants defaultData;
			memcpy(m_mappedData, &defaultData, sizeof(MaterialConstants));
		}

		~Material()
		{
			if (m_uploadBuffer)
			{
				m_uploadBuffer->Unmap(0, nullptr);
				m_mappedData = nullptr;
			}
		}
		const std::string& GetName() const { return m_name; }

		MaterialConstants& GetMaterialConstants()
		{
			return m_materialConstants;
		}

		void SetDiffuseAlbedo(const DirectX::XMFLOAT4& diffuseAlbedo)
		{
			m_materialConstants.DiffuseAlbedo = diffuseAlbedo;
		}

		void SetFresnelR0(const DirectX::XMFLOAT3& fresnelR0)
		{
			m_materialConstants.FresnelR0 = fresnelR0;
		}

		void SetRoughness(float roughness)
		{
			m_materialConstants.Roughness = roughness;
		}

		void CopyData()
		{
			memcpy(m_mappedData, &m_materialConstants, sizeof(MaterialConstants));
		}

private:
	std::string m_name;

	MaterialConstants m_materialConstants;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer;
	unsigned char* m_mappedData = nullptr;

	UINT m_materialCBSize = 0;
};