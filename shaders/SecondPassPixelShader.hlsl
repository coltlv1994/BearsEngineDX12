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

ConstantBuffer<LightConstants> LightCB : register(b0);

Texture2D gAlbedoTexture : register(t0);
Texture2D gSpecularGlossTexture : register(t1);
Texture2D gNormalTexture : register(t2);
Texture2D gDepth : register(t3);

SamplerState Sampler : register(s0);

float4 main(FPPS_IN IN) : SV_TARGET
{
	
    return gAlbedoTexture.Sample(Sampler, IN.TexCoord);
}