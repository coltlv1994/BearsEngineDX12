#include "Application.h"
#include "Shader.h"
#include "Mesh.h"
#include "EntityManager.h"

// Main function
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	Application& mainApp = Application::GetInstance(hInstance, L"BearsEngine in DX12", 1280, 720, true);

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	size_t inputLayoutCount = 1;

	Shader meshShader = Shader(L"shaders\\VertexShader.cso", L"shaders\\PixelShader.cso", inputLayout, inputLayoutCount);
	Mesh* mesh = new Mesh(L"meshes\\FinalBaseMesh.obj");

	mesh->UseShader(&meshShader);

	// Update the view matrix.
	const XMVECTOR eyePosition = XMVectorSet(0, 0, -5, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	XMMATRIX viewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
	mesh->SetViewMatrix(viewMatrix);

	// Update the projection matrix.
	int height, width;
	mainApp.GetWidthAndHeight(width, height);
	float aspectRatio = width / static_cast<float>(height);
	XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), aspectRatio, 0.1f, 100.0f);
	mesh->SetProjectionMatrix(projectionMatrix);
	mesh->SetFOV(90.0f);

	mainApp.AddEntity(mesh);

	mainApp.Run();

	return 0;
}