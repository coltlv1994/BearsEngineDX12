struct FPPS_IN
{
    float2 TexCoord : TEXCOORD;
    float3 Normal : NORMAL;
};

struct FPPS_OUT
{
    float3 albedo : SV_TARGET0;
    float3 normal : SV_TARGET1;
    float4 specgloss : SV_TARGET2;
    
    // from texture sampling
    float4 color : SV_TARGET3; 
};

Texture2D earthTexture : register(t0);
Texture2D specGlossTexture : register(t1);
Texture2D diffuseTexture : register(t2);

SamplerState Sampler : register(s0);

FPPS_OUT main(FPPS_IN IN) : SV_Target
{
    FPPS_OUT OUT;
   
    // * is component-wise multiplication, dot is inner product
    OUT.albedo = diffuseTexture.Sample(Sampler, IN.TexCoord).rgb;
    OUT.specgloss = specGlossTexture.Sample(Sampler, IN.TexCoord);
    OUT.normal = IN.Normal;
    OUT.color = earthTexture.Sample(Sampler, IN.TexCoord);
    
    return OUT;
}