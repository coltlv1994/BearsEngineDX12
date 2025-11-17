#include "Application.h"

// Main function
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	Application& mainApp = Application::GetInstance(hInstance, L"BearsEngine in DX12", 1280, 720, true);

	mainApp.Run();

	return 0;
}