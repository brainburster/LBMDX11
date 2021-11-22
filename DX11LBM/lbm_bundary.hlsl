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
        [unroll(9)]
        for (uint i = 0; i < 9; i++)
        {
            //f_in[uint3(pos, i)] = f_in[uint3(pos + uint2(-1, 0), i)];
            //f_in[uint3(pos, i)] = f_in[uint3(pos + uint2(-2, 0), i)] * 2 - f_in[uint3(pos + uint2(-1, 0), i)];
            f_in[uint3(pos, i)] = f_in[uint3(pos + uint2(-1, 0), i)] * 1.5f - f_in[uint3(pos + uint2(-2, 0), i)] * 0.5f; 
        }
    }
    else
    {
        float f[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        uint i = 0;
        float rho = 0;
        float2 u = { 0.0f, 0.0f };
        //
        [unroll(9)]
        for (i = 0; i < 9; i++)
        {
            f[i] = f_out[uint3(pos, i)];
        }
    
        u = float2(0.02f, 0);
        rho = 1 / (1 - u.x) * (f[3] + f[4] + f[5] + (f[6] + f[7] + f[8]) * 2 + 1e-20f);
        
        const float usqr = 1.5 * dot(u, u);
        float eq[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        
        
        [unroll(9)]
        for (i = 0; i < 9; i++)
        {
            float cu = 3.f * dot(u, c[i]);
            eq[i] = rho * w[i] * (1.f + cu + 0.5f * cu * cu - usqr);
        }
    
        f[0] = eq[0] + f[6] - eq[6];
        f[1] = eq[1] + f[7] - eq[7];
        f[2] = eq[2] + f[8] - eq[8];
        
                
        [unroll(9)]
        for (i = 0; i < 9; i++)
        {
            f_in[uint3(pos, i)] = f[i];
        }
    }
}