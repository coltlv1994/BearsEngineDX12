Texture2D gAlbedoTexture : register(t0);
Texture2D gNormalTexture : register(t1);
Texture2D gSpecularGlossTexture : register(t2);
Texture2D gDepth : register(t3);

SamplerState Sampler : register(s0);

float4 main() : SV_TARGET
{
	
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}