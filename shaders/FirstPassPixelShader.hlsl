struct FPPS_IN
{
    float2 TexCoord : TEXCOORD;
    float3x3 tbnMatrix : TBNMATRIX;
};

struct FPPS_OUT
{
    float4 albedo : SV_TARGET0;
    float specgloss : SV_TARGET1;
    float4 normal : SV_TARGET2;
};

Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D specularTexture : register(t2);

SamplerState Sampler : register(s0);

FPPS_OUT main(FPPS_IN IN)
{
    FPPS_OUT OUT;
   
    // * is component-wise multiplication, dot is inner product
    OUT.albedo = diffuseTexture.Sample(Sampler, IN.TexCoord);
    OUT.specgloss = specularTexture.Sample(Sampler, IN.TexCoord).x;
    
    float3 n_sample = normalTexture.Sample(Sampler, IN.TexCoord);
    OUT.normal = (normalize(mul(IN.tbnMatrix, n_sample * 2.0f - 1.0f)), 0.0f);
    
    return OUT;
}