struct PixelShaderInput
{
	float2 TexCoord    : TEXCOORD;
    // Normal vector is normalized in vertex shader already
    float3 Normal      : NORMAL;
};

Texture2D earthTexture : register(t0);
SamplerState Sampler : register(s0);

float4 main( PixelShaderInput IN ) : SV_Target
{
    return earthTexture.Sample(Sampler, IN.TexCoord);
}