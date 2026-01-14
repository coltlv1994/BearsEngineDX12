#define MaxLights 16

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

// TODO: define another struct for lighting parameters

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction; // directional/spot light only
    float FalloffEnd; // point/spot light only
    float3 Position; // point light only
    float SpotPower; // spot light only
};

struct LightConstants
{
    float4 CameraPosition;
    float4 AmbientLightColor;
    float AmbientLightStrength;
    uint NumOfDirectionalLights;
    uint NumOfPointLights;
    uint NumOfSpotLights;
    
    Light Lights[MaxLights];
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
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    OUT.Normal = normalize(mul((float3x3) ModelViewProjectionCB.t_i_model, IN.Normal));
    OUT.Color = LightCB.AmbientLightColor; // just pass ambient light color for now
    OUT.TexCoord = IN.TexCoord;

    return OUT;
}