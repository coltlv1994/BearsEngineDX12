struct FPPS_IN
{
    float2 TexCoord : TEXCOORD;
};

Texture2D gAlbedoTexture : register(t0);
Texture2D gSpecularGlossTexture : register(t1);
Texture2D gNormalTexture : register(t2);
Texture2D gDepth : register(t3);

SamplerState Sampler : register(s0);

float4 main(FPPS_IN IN) : SV_TARGET
{
    //float depthValue = gDepth.Sample(Sampler, IN.TexCoord).r;
	
    return gAlbedoTexture.Sample(Sampler, IN.TexCoord);
}