struct ModelViewProjection
{
    // Combined Model-View-Projection matrix
    // and transpose of inverse model matrix
    matrix MVP;
    matrix tiModel;
};

struct FirstPassVS_IN
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 TexCoord : TEXCOORD;
};

struct FirstPassVS_OUT
{
    float2 TexCoord : TEXCOORD;
    float3x3 tbnMatrix : TBNMATRIX;
    float4 Position : SV_Position;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);


FirstPassVS_OUT main(FirstPassVS_IN FPVS_IN)
{
    FirstPassVS_OUT OUT;
    
    OUT.TexCoord = FPVS_IN.TexCoord;
    // mul(matrix, vector) will assume the matrix is column-major and vector is column vector
    // HLSL is default to use column-major to load matrices
    // When loading MVP matrix into constant buffer, it will write by rows; but HLSL will interpret them
    // as columns here, which is effectively, transpose.
    // (MA * MB)^T = MB^T * MA^T
    // No additional transpose is needed.
    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(FPVS_IN.Position, 1.0f));
    
    // construct tbn matrix
    float3 N = normalize(mul(ModelViewProjectionCB.tiModel, float4(FPVS_IN.Normal, 0.0f)).xyz);
    float3 T = normalize(mul(ModelViewProjectionCB.tiModel, float4(FPVS_IN.Tangent, 0.0f)).xyz);
    float3 B = cross(N, T);
    
    OUT.tbnMatrix = float3x3(T, B, N);
    
    return OUT;
}