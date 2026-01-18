struct ModelViewProjection
{
    // Combined Model-View-Projection matrix
    matrix MVP;
    // Transpose of Inverse of the Model matrix for normal transformation
    // i.e., t_i_model = transpose(inverse(Model))
    matrix t_i_model;
};


struct FirstPassVS_IN
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct FirstPassVS_OUT
{
    float2 TexCoord : TEXCOORD;
    float3 Normal : NORMAL;
    float4 Position : SV_Position;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);


FirstPassVS_OUT main(FirstPassVS_IN FPVS_IN)
{
    FirstPassVS_OUT OUT;
    
    OUT.TexCoord = FPVS_IN.TexCoord;
    OUT.Normal = normalize(mul((float3x3) ModelViewProjectionCB.t_i_model, FPVS_IN.Normal));
    OUT.Position = mul(float4(FPVS_IN.Position, 1.0f), ModelViewProjectionCB.MVP);
    
    return OUT;
}