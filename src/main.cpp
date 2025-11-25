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

	Shader meshShader = Shader(L"x64\\Debug\\VertexShader.cso", L"x64\\Debug\\PixelShader.cso", inputLayout, inputLayoutCount);
	Mesh* mesh = new Mesh(L"meshes\\FinalBaseMesh.obj");

	mesh->UseShader(&meshShader);

	mainApp.AddEntity(mesh);

	mainApp.Run();

	return 0;
}