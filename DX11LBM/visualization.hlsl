Texture2D<float4> inTex : register(t0);
RWTexture2D<float4> outTex : register(u0);
RWTexture2DArray<float> f_in : register(u1);
RWTexture2DArray<float> f_out : register(u2);

static const int2 v[9] = { { 1, 1 }, { 1, 0 }, { 1, -1 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const int2 index = DTid.xy;
    
    if (index.x < 1 || index.x > 798 || index.y < 1 || index.y > 598)
    {
        return;
    }
    
    float rho[4] = { 0, 0, 0, 0 };
    float2 u[4] = { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };
    
    [unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        rho[0] += f_in[uint3(index + int2(1, 0), i)];
        rho[1] += f_in[uint3(index + int2(0, 1), i)];
        rho[2] += f_in[uint3(index + int2(-1, 0), i)];
        rho[3] += f_in[uint3(index + int2(0, -1), i)];
        
        u[0] += f_in[uint3(index + int2(1, 0), i)] * v[i];
        u[1] += f_in[uint3(index + int2(0, 1), i)] * v[i];
        u[2] += f_in[uint3(index + int2(-1, 0), i)] * v[i];
        u[3] += f_in[uint3(index + int2(0, -1), i)] * v[i];
    }
    
    u[0] /= rho[0];
    u[1] /= rho[1];
    u[2] /= rho[2];
    u[3] /= rho[3];
    float du = (u[0].x - u[2].x) * 0.5f;
    float dv = (u[1].x - u[3].x) * 0.5f;
    float w = du - dv;
    
    float3 a = { 0.8f - abs(w) * 20, 0.8f - du * 16, 0.8f - dv * 16 };
    //float3 a = 0.8f - abs(w) * 20;
    
    outTex[index] = lerp(saturate(float4(a, 1.f)), float4(0.18f, 0.18f, 0.18f, 1.f), saturate((inTex[index]).r * 2));
}