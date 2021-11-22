#include "header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    if (pos.x > 798 || pos.x < 1 || pos.y > 598 || pos.y < 1)
    {
        return;
    }
    
    //float rho[4] = { 0, 0, 0, 0 };
    //float2 u[4] = { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };
    float rho0 = 0;
    float2 u0 = 0;
    [unroll]
    for (uint i = 0; i < 9; i++)
    {
        rho0 += f_in[uint3(pos, i)];
        u0 += f_in[uint3(pos, i)] * c[i];
        
        //rho[0] += f_in[uint3(pos + int2(1, 0), i)];
        //rho[1] += f_in[uint3(pos + int2(0, 1), i)];
        //rho[2] += f_in[uint3(pos + int2(-1, 0), i)];
        //rho[3] += f_in[uint3(pos + int2(0, -1), i)];
        
        
        //u[0] += f_in[uint3(pos + int2(1, 0), i)] * c[i];
        //u[1] += f_in[uint3(pos + int2(0, 1), i)] * c[i];
        //u[2] += f_in[uint3(pos + int2(-1, 0), i)] * c[i];
        //u[3] += f_in[uint3(pos + int2(0, -1), i)] * c[i];
    }
    
    //u[0] /= rho[0] + 1e-20f;
    //u[1] /= rho[1] + 1e-20f;
    //u[2] /= rho[2] + 1e-20f;
    //u[3] /= rho[3] + 1e-20f;
    //float du = (u[0].x - u[2].x) * 0.5f;
    //float dv = (u[1].x - u[3].x) * 0.5f;
    //float w = du - dv;
    
    float2 v = u0 / rho0 + 1e-20f;
    float c = length(v) * 10+0.01f;
    //float3 a = { c, c + w * 50, c - w * 50 };
    float3 a = c;
    
    outTex[pos] = lerp(saturate(float4(a, 1.f)), float4(0.18f, 0.18f, 0.18f, 1.f), saturate((inTex[pos]).r * 2));
}