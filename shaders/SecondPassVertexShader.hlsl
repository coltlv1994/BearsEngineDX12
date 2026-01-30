struct SecondPassVS_IN
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct SecondPassVS_OUT
{
    float2 texcoord : TEXCOORD;
    float4 position : SV_POSITION;
};

SecondPassVS_OUT main(SecondPassVS_IN IN)
{
    SecondPassVS_OUT OUT;
    OUT.position = float4(IN.position, 1.0f);
    OUT.texcoord = IN.texcoord;
    return OUT;
}