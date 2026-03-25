// WARNING: upon any changes to this struct, remember to modify accordingly in Helpers.h
struct ModelViewProjection
{
    // Combined Model-View-Projection matrix
    matrix MVP;
    // Transpose of Inverse of the Model matrix for normal transformation
    // i.e., t_i_model = transpose(inverse(Model))
    matrix t_i_model;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 Pos : POSITION; // for pixel shader
    float2 TexCoord : TEXCOORD;
    float3x3 tbnMatrix : TBNMATRIX;
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    OUT.Pos = OUT.Position;
    
    OUT.TexCoord = IN.TexCoord;
    
    // construct tbn matrix
    float3x3 tiM = (float3x3) ModelViewProjectionCB.t_i_model;
    
    float3 N = normalize(mul(tiM, IN.Normal));
    float3 T = normalize(mul(tiM, IN.Tangent));
    float3 B = cross(N, T);
    
    OUT.tbnMatrix = float3x3(T, B, N);

    return OUT;
}