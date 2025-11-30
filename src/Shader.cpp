#include <Shader.h>
#include <Helpers.h>
#include <cstring>
#include <DirectXMath.h>
using namespace DirectX;

#include <Application.h>

Shader::Shader(const wchar_t* p_vsPath, const wchar_t* p_psPath)
{
	ThrowIfFailed(D3DReadFileToBlob(p_vsPath, &m_vertexShaderBlob));
	ThrowIfFailed(D3DReadFileToBlob(p_psPath, &m_pixelShaderBlob));
}

Shader::~Shader()
{
}
