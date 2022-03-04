#include "header.hlsli"

[numthreads(1, 599, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos ={
        DTid.x ? 799 : 0,
        DTid.y
    };
    
    if (pos.x)
    {
        f_out[uint3(pos, 6)] = f_out[uint3(pos + uint2(-1, 0), 6)] * 1.5f - f_out[uint3(pos + uint2(-2, 0), 6)] * 0.5f;
        f_out[uint3(pos, 7)] = f_out[uint3(pos + uint2(-1, 0), 7)] * 1.5f - f_out[uint3(pos + uint2(-2, 0), 7)] * 0.5f;
        f_out[uint3(pos, 8)] = f_out[uint3(pos + uint2(-1, 0), 8)] * 1.5f - f_out[uint3(pos + uint2(-2, 0), 8)] * 0.5f;
    }
    else
    {
        uint2 pos2 = pos + uint2(1, 0);
        uint i = 0;
        float rho = 0;
        float2 u = { 0.0f, 0.0f };
        //
        [unroll(9)]
        for (i = 0; i < 9; i++)
        {
            float f_i = f_out[uint3(pos2, i)];
            rho += f_i;
            u += f_i * c[i];
        }
        u /= rho + 1e-20f;
        float a = u.x < 0.02f ? 1e-6f : 0;
        f_out[uint3(pos, 0)] = f_out[uint3(pos2, 0)] + a;
        f_out[uint3(pos, 1)] = f_out[uint3(pos2, 1)] + a;
        f_out[uint3(pos, 2)] = f_out[uint3(pos2, 2)] + a;
        
    //    float f[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    //    uint i = 0;
    //    float rho = 0;
    //    float2 u = { 0.0f, 0.0f };
    //    //
    //    [unroll(9)]
    //    for (i = 0; i < 9; i++)
    //    {
    //        f[i] = f_out[uint3(pos, i)];
    //    }
    
    //    u = float2(0.02f, 0);
    //    rho = 1 / (1 - u.x) * (f[3] + f[4] + f[5] + (f[6] + f[7] + f[8]) * 2 + 1e-20f);
        
    //    const float usqr = 1.5 * dot(u, u);
    //    float eq[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        
        
    //    [unroll(9)]
    //    for (i = 0; i < 9; i++)
    //    {
    //        float cu = 3.f * dot(u, c[i]);
    //        eq[i] = rho * w[i] * (1.f + cu + 0.5f * cu * cu - usqr);
    //    }
    
    //    f[0] = eq[0] + f[6] - eq[6];
    //    f[1] = eq[1] + f[7] - eq[7];
    //    f[2] = eq[2] + f[8] - eq[8];
        
                
    //    [unroll(9)]
    //    for (i = 0; i < 9; i++)
    //    {
    //        f_in[uint3(pos, i)] = f[i];
    //    }
    }
}