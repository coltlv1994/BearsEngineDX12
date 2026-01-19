/*
 *  Copyright(c) 2017 Jeremiah van Oosten
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files(the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions :
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

/**
 *  @file Helpers.h
 *  @date August 28, 2017
 *  @author Jeremiah van Oosten
 *
 *  @brief Helper functions.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT

#include <DirectXMath.h>
using namespace DirectX;

#include <exception>

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

#define STR1(x) #x
#define STR(x) STR1(x)
#define WSTR1(x) L##x
#define WSTR(x) WSTR1(x)
#define NAME_D3D12_OBJECT(x) x->SetName( WSTR(__FILE__ "(" STR(__LINE__) "): " L#x) )
#define MAX_LIGHTS 16

static UINT CalcConstantBufferByteSize(UINT byteSize)
{
    // Constant buffers must be a multiple of the minimum hardware
    // allocation size (usually 256 bytes).  So round up to nearest
    // multiple of 256.  We do this by adding 255 and then masking off
    // the lower 2 bytes which store all bits < 256.
    // Example: Suppose byteSize = 300.
    // (300 + 255) & ~255
    // 555 & ~255
    // 0x022B & ~0x00ff
    // 0x022B & 0xff00
    // 0x0200
    // 512
    return (byteSize + 255) & ~255;
}

// Vertex data for a colored cube.
struct VertexPosColor
{
    XMFLOAT3 Position;
    // normals are currently read from texture map
    //XMFLOAT3 Normal;
    XMFLOAT2 TexCoord;
};

// input structure for the vertex shader.
// WARNING: must match the structure defined in the shader.
struct VertexShaderInput
{
    XMMATRIX mvpMatrix;
};

// used for material constant buffer view
// NOTE: must match the structure defined in the shader.
struct MaterialConstants
{
    XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = 0.5f;
};

struct DirectionalLight
{
    float Strength = 0.5f;
	XMFLOAT3 Direction = XMFLOAT3(1.0f, 0.0f, 0.0f);

    XMFLOAT4 Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
};

struct PointLight
{
    float Strength = 0.5f;
    XMFLOAT3 Position = XMFLOAT3(0.0f, 0.0f, -10.0f);

    XMFLOAT4 Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    XMFLOAT2 Falloff = XMFLOAT2(1.0f, 1.0f); // start, end
    float Padding[2] = {0.0f, 0.0f};
};

struct SpotLight
{
    float Strength = 0.5f;
	XMFLOAT3 Position = XMFLOAT3(0.0f, 0.0f, -10.0f);

    XMFLOAT4 Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    XMFLOAT3 Direction = XMFLOAT3(1.0f, 0.0f, 0.0f);
    float SpotPower = 1.0f;

    XMFLOAT2 Falloff = XMFLOAT2(1.0f, 1.0f); // start, end
    float Padding[2] = { 0.0f, 0.0f };
};

struct LightConstants
{
    XMFLOAT4 CameraPosition = XMFLOAT4(0.0f, 0.0f, -10.0f, 0.0f);
    XMFLOAT4 AmbientLightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    float AmbientLightStrength = 0.2f;
    uint32_t NumOfDirectionalLights = 0;
    uint32_t NumOfPointLights = 0;
    uint32_t NumOfSpotLights = 0;

	DirectionalLight DirectionalLights[MAX_LIGHTS];
	PointLight PointLights[MAX_LIGHTS];
	SpotLight SpotLights[MAX_LIGHTS];
};

const static DirectionalLight defaultDL;

const static PointLight defaultPL;

const static SpotLight defaultSL;