#define MAX_LIGHTS 128

struct PixelShaderInput
{
    float4 Pos : POSITION; // for pixel shader
    float2 TexCoord : TEXCOORD;
    float3x3 tbnMatrix : TBNMATRIX;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

struct DirectionalLight
{
    float Strength;
    float3 Direction;
    
    float4 Color;
};

struct PointLight
{
    float Strength;
    float3 Position;
    
    float4 Color;
    
    float2 Falloff; // start, end
    float2 Padding;
};

struct SpotLight
{
    float Strength;
    float3 Position;
    
    float4 Color;
    
    float3 Direction;
    float SpotPower;
    
    float2 Falloff; // start, end
    float2 Padding;
};

struct LightConstants
{
    float4 CameraPosition;
    float4 AmbientLightColor;
    float AmbientLightStrength;
    uint NumOfDirectionalLights;
    uint NumOfPointLights;
    uint NumOfSpotLights;
    
    DirectionalLight DirectionalLights[MAX_LIGHTS];
    PointLight PointLights[MAX_LIGHTS];
    SpotLight SpotLights[MAX_LIGHTS];
};

struct MaterialConstants
{
    float4 DiffuseAlbedo;
    float Roughness;
};

static MaterialConstants MAT;

ConstantBuffer<LightConstants> LightCB : register(b1);

Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D specularTexture : register(t2);

SamplerState Sampler : register(s0);

// Move light calculations to vertex shader for performance optimization
float CalcAttenuation(float d, float2 falloff)
{
    // Linear falloff.
    return saturate((falloff.y - d) / (falloff.y - falloff.x));
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye)
{
    
    const float m = (1.0f - MAT.Roughness) * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    float specAlbedo = roughnessFactor / (roughnessFactor + 1.0f);

    return (MAT.DiffuseAlbedo.rgb + roughnessFactor) * lightStrength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(DirectionalLight L, float3 normal, float3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = -L.Direction;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for point lights.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(PointLight L, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if (d > L.Falloff.y)
        return 0.0f;

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.Falloff);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for spot lights.
//---------------------------------------------------------------------------------------
float3 ComputeSpotLight(SpotLight L, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if (d > L.Falloff.y)
        return 0.0f;

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.Falloff);
    lightStrength *= att;

    // Scale by spotlight
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye);
}

float4 ComputeLighting(float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor)
{
    float3 result = 0.0f;
    
    for (uint i = 0; i < LightCB.NumOfDirectionalLights; i++)
    {
        result += shadowFactor[i] * ComputeDirectionalLight(LightCB.DirectionalLights[i], normal, toEye);
    }
    
    for (uint i = 0; i < LightCB.NumOfPointLights; i++)
    {
        result += shadowFactor[i] * ComputePointLight(LightCB.PointLights[i], pos, normal, toEye);
    }
    
    for (uint i = 0; i < LightCB.NumOfSpotLights; i++)
    {
        result += shadowFactor[i] * ComputeSpotLight(LightCB.SpotLights[i], pos, normal, toEye);
    }

    return float4(result, 0.0f);
}

PixelShaderOutput main(PixelShaderInput IN) : SV_Target
{
    PixelShaderOutput OUT;
    
    float3 n_sample = normalTexture.Sample(Sampler, IN.TexCoord).xyz;
    n_sample = n_sample * 2.0f - 1.0f;
    float3 newNormal = float4(normalize(mul(IN.tbnMatrix, n_sample)), 0.0f);
    
    MAT.DiffuseAlbedo = diffuseTexture.Sample(Sampler, IN.TexCoord);
    MAT.Roughness = specularTexture.Sample(Sampler, IN.TexCoord).x;
    
    float3 toEye = normalize(LightCB.CameraPosition.xyz - IN.Pos.xyz);
    
    float4 ambientLight = LightCB.AmbientLightStrength * LightCB.AmbientLightColor * dot(newNormal, newNormal);
    
    float4 lighting = ComputeLighting(IN.Pos.xyz,
                                      newNormal,
                                      toEye,
                                      float3(1.0f, 1.0f, 1.0f)); // No shadows for now
    
    OUT.color = (lighting + ambientLight) * MAT.DiffuseAlbedo;
    
    return OUT;

}