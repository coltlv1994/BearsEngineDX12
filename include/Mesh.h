#pragma once
#include <CommonHeaders.h>
#include <Shader.h>
#include <Renderable.h>
#include <vector>

class Mesh : public Renderable
{
public:
	Mesh(const wchar_t* verticesPath, const wchar_t* indicesPath);
	void Render() override;
	void UseShader(Shader* shader_p);
	Shader& GetShaderObjectByRef();
private:
	std::vector<float> m_vertices;
	std::vector<uint16_t> m_indices;

	Shader* m_shader_p;

	void _readFromFile(const wchar_t* verticesPath, const wchar_t* indicesPath);
};
