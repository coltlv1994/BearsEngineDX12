#include <Mesh.h>
#include <Application.h>

Mesh::Mesh(const wchar_t* p_verticesPath, const wchar_t* p_indicesPath)
{
	m_vertices.clear();
	m_indices.clear();

	_readFromFile(p_verticesPath, p_indicesPath);
}

void Mesh::Render()
{
	// Populate command queue
	Application& app = Application::GetInstance();

	if (!m_shader_p)
	{
		// Shader class is set

	}
}

void Mesh::UseShader(Shader* p_shader_p)
{
	// clear previous root signiture

	// set new shader
	m_shader_p = p_shader_p;
}

Shader& Mesh::GetShaderObjectByRef()
{
    return *m_shader_p;
}


void Mesh::_readFromFile(const wchar_t* verticesPath, const wchar_t* indicesPath)
{

}