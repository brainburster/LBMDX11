struct VsIn
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VsOut
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD;
};
