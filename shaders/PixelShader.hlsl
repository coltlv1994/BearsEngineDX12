struct PixelShaderInput
{
	float2 TexCoord    : TEXCOORD;
    float4 LightColor : LIGHTCOLOR;
};

Texture2D earthTexture : register(t0);
SamplerState Sampler : register(s0);

float4 main( PixelShaderInput IN ) : SV_Target
{
    // * is component-wise multiplication, dot is inner product
    return earthTexture.Sample(Sampler, IN.TexCoord) * IN.LightColor;
}