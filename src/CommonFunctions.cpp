#include <CommonHeaders.h>
#include <sstream>
#include <fstream>
#include <string>
#include <regex>

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