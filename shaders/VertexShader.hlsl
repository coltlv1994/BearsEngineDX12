#define MAX_LIGHTS 16

// WARNING: upon any changes to this struct, remember to modify accordingly in Helpers.h
struct ModelViewProjection
{
    // Combined Model-View-Projection matrix
    matrix MVP;
    // Transpose of Inverse of the Model matrix for normal transformation
    // i.e., t_i_model = transpose(inverse(Model))
    matrix t_i_model;
};

struct MaterialConstants
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Roughness;
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

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);
ConstantBuffer<MaterialConstants> MaterialCB : register(b1);
ConstantBuffer<LightConstants> LightCB : register(b2);

struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float2 TexCoord : TEXCOORD;
    float4 LightColor : LIGHTCOLOR;
    float4 Position : SV_Position;
};

// Move light calculations to vertex shader for performance optimization
float CalcAttenuation(float d, float2 falloff)
{
    // Linear falloff.
    return saturate((falloff.y - d) / (falloff.y - falloff.x));
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye)
{
    
    const float m = (1.0f - MaterialCB.Roughness) * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(MaterialCB.FresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor * roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (MaterialCB.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
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

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    float3 normal = normalize(mul((float3x3) ModelViewProjectionCB.t_i_model, IN.Normal));
    OUT.TexCoord = IN.TexCoord;
    
    float3 ToEye = normalize(LightCB.CameraPosition.xyz - OUT.Position.xyz);
    
    float4 ambientLight = LightCB.AmbientLightStrength * MaterialCB.DiffuseAlbedo * LightCB.AmbientLightColor;
    
    float4 lighting = ComputeLighting(OUT.Position.xyz,
                                      normal,
                                      ToEye,
                                      float3(1.0f, 1.0f, 1.0f)); // No shadows for now
    
    OUT.LightColor = ambientLight + lighting;

    return OUT;
}