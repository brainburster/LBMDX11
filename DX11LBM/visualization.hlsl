Texture2D<float4> inTex : register(t0);
RWTexture2D<float4> outTex : register(u0);
RWTexture2DArray<float> f_in : register(u1);
RWTexture2DArray<float> f_out : register(u2);

static const int2 v[9] = { { 1, 1 }, { 1, 0 }, { 1, -1 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const int2 index = DTid.xy;
    float rho = 0;
    float2 c = { 0, 0 };
    
    [unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        rho += f_in[uint3(index, i)];
        c += f_in[uint3(index, i)] * v[i];
    }
    
    c /= rho;
    float w = c.y - c.x;
    float3 a = 0.9f - w * 2.0f;
    outTex[index] = lerp(saturate(float4(a, 1.f)), float4(0.15f, 0.15f, 0.15f, 1.f), saturate((inTex[index]).r * 2));
}