#define MAX_LIGHTS 16

struct FPPS_IN
{
    float2 TexCoord : TEXCOORD;
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

struct SecondPassRootConstants
{
    matrix invSPV; // screen * inv(P) * inv(V)
};

ConstantBuffer<LightConstants> LightCB : register(b0);
ConstantBuffer<SecondPassRootConstants> SPRC : register(b1);

Texture2D gAlbedoTexture : register(t0);
Texture2D gSpecularTexture : register(t1);
Texture2D gNormalTexture : register(t2);
Texture2D gDepth : register(t3);

SamplerState Sampler : register(s0);

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

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, float specular)
{
    
    const float m = specular * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;

    float3 specAlbedo = roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    roughnessFactor = roughnessFactor / (roughnessFactor + 1.0f);

    return specAlbedo * lightStrength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(DirectionalLight L, float3 normal, float3 toEye, float specular)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = -L.Direction;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, specular);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for point lights.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(PointLight L, float3 pos, float3 normal, float3 toEye, float specular)
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

    return BlinnPhong(lightStrength, lightVec, normal, toEye, specular);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for spot lights.
//---------------------------------------------------------------------------------------
float3 ComputeSpotLight(SpotLight L, float3 pos, float3 normal, float3 toEye, float specular)
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

    return BlinnPhong(lightStrength, lightVec, normal, toEye, specular);
}

float4 ComputeLighting(float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor, float specular)
{
    float3 result = 0.0f;
    
    for (uint i = 0; i < LightCB.NumOfDirectionalLights; i++)
    {
        result += shadowFactor[i] * ComputeDirectionalLight(LightCB.DirectionalLights[i], normal, toEye, specular);
    }
    
    for (uint i = 0; i < LightCB.NumOfPointLights; i++)
    {
        result += shadowFactor[i] * ComputePointLight(LightCB.PointLights[i], pos, normal, toEye, specular);
    }
    
    for (uint i = 0; i < LightCB.NumOfSpotLights; i++)
    {
        result += shadowFactor[i] * ComputeSpotLight(LightCB.SpotLights[i], pos, normal, toEye, specular);
    }

    return float4(result, 0.0f);
}


float4 main(FPPS_IN IN) : SV_TARGET
{
    float4 albedo = gAlbedoTexture.Sample(Sampler, IN.TexCoord); // final color
    
    // reconstruct world position from depth
    float z = gDepth.Sample(Sampler, IN.TexCoord).x;
    float4 projectedPosition = float4(IN.TexCoord.xy, z, 1.0f);
    float4 worldPosition = mul(SPRC.invSPV, projectedPosition);
    worldPosition /= worldPosition.w;
    
    // normal mapping
    // the texture has value of [0, 1] and now it has to converted to [-1, 1]
    float3 normal = normalize(gNormalTexture.Sample(Sampler, IN.TexCoord).xyz * 2.0f - 1.0f);
    
    // specular value
    float specular = gSpecularTexture.Sample(Sampler, IN.TexCoord).x;
    
    float3 toEye = normalize(LightCB.CameraPosition.xyz - worldPosition.xyz);
    
    float4 ambientLight = LightCB.AmbientLightStrength * albedo * LightCB.AmbientLightColor;
    
    float4 lighting = ComputeLighting(worldPosition.xyz,
                                      normal,
                                      toEye,
                                      float3(1.0f, 1.0f, 1.0f), // No shadows for now
                                      specular);
    
    float4 LightColor = ambientLight + lighting;
    
    return LightColor;
}