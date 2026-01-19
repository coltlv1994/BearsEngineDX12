struct FPPS_IN
{
    float2 TexCoord : TEXCOORD;
};

struct FPPS_OUT
{
    float4 albedo : SV_TARGET0;
    float specgloss : SV_TARGET1;
    float4 normal : SV_TARGET2;
};

Texture2D diffuseTexture : register(t0);
Texture2D specularTexture : register(t1);
Texture2D normalTexture : register(t2);

SamplerState Sampler : register(s0);

FPPS_OUT main(FPPS_IN IN)
{
    FPPS_OUT OUT;
   
    // * is component-wise multiplication, dot is inner product
    OUT.albedo = diffuseTexture.Sample(Sampler, IN.TexCoord);
    OUT.specgloss = specularTexture.Sample(Sampler, IN.TexCoord).x;
    OUT.normal = normalTexture.Sample(Sampler, IN.TexCoord);
    
    return OUT;
}