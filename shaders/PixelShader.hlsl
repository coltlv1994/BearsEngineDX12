struct PixelShaderInput
{
	float2 TexCoord    : TEXCOORD;
    // Normal vector is normalized in vertex shader already
    float3 Normal      : NORMAL;
    float4 Color       : COLOR;
};

Texture2D earthTexture : register(t0);
SamplerState Sampler : register(s0);

float4 main( PixelShaderInput IN ) : SV_Target
{
    // * is component-wise multiplication, dot is inner product
    return earthTexture.Sample(Sampler, IN.TexCoord) * IN.Color;
}