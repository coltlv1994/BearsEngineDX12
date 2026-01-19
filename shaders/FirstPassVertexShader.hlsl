struct ModelViewProjection
{
    // Combined Model-View-Projection matrix
    matrix MVP;
};


struct FirstPassVS_IN
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct FirstPassVS_OUT
{
    float2 TexCoord : TEXCOORD;
    float4 Position : SV_Position;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);


FirstPassVS_OUT main(FirstPassVS_IN FPVS_IN)
{
    FirstPassVS_OUT OUT;
    
    OUT.TexCoord = FPVS_IN.TexCoord;
    OUT.Position = mul(float4(FPVS_IN.Position, 1.0f), ModelViewProjectionCB.MVP);
    
    return OUT;
}